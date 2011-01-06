// ****************************************************************************
//  compiler-unit.cpp                                               XLR project
// ****************************************************************************
//
//   File Description:
//
//     Information about a single compilation unit, i.e. the code generated
//     for a particular tree
//
//
//
//
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************
//
// The compilation unit is where most of the "action" happens, e.g. where
// the code generation happens for a given tree. It records all information
// that is transient, i.e. only exists during a given compilation phase
//
// In the following, we will consider a rewrite such as:
//    foo X:integer, Y -> bar X + Y
//
// Such a rewrite is transformed into a function with a prototype that
// depends on the arguments, i.e. something like:
//    retType foo(int X, Tree *Y);
//
// The actual retType is determined dynamically from the return type of bar.

#include "compiler-unit.h"
#include "compiler-parm.h"
#include "compiler-expred.h"
#include "errors.h"
#include "types.h"

#include <llvm/Analysis/Verifier.h>
#include <llvm/CallingConv.h>
#include "llvm/Constants.h"
#include "llvm/LLVMContext.h"
#include "llvm/DerivedTypes.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include <llvm/PassManager.h>
#include "llvm/Support/raw_ostream.h"
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/StandardPasses.h>
#include <llvm/System/DynamicLibrary.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Support/raw_ostream.h>

XL_BEGIN

using namespace llvm;

CompiledUnit::CompiledUnit(Compiler *compiler, Context *context)
// ----------------------------------------------------------------------------
//   CompiledUnit constructor
// ----------------------------------------------------------------------------
    : context(context), inference(NULL),
      compiler(compiler), llvm(compiler->llvm),
      code(NULL), data(NULL), function(NULL),
      allocabb(NULL), entrybb(NULL), exitbb(NULL), failbb(NULL),
      returned(NULL),
      value(), storage(), computed(), dataForm()
{}


CompiledUnit::~CompiledUnit()
// ----------------------------------------------------------------------------
//   Delete what we must...
// ----------------------------------------------------------------------------
{
    if (entrybb && exitbb)
    {
        // If entrybb is clear, we may be looking at a forward declaration
        // Otherwise, if exitbb was not cleared by Finalize(), this means we
        // failed to compile. Make sure LLVM cleans the function up
        function->eraseFromParent();
    }

    delete code;
    delete data;
}


Function *CompiledUnit::RewriteFunction(Rewrite *rewrite, TypeInference *inf)
// ----------------------------------------------------------------------------
//   Create a function for a tree rewrite
// ----------------------------------------------------------------------------
{
    // We must have verified the types before
    assert((inf && !inference) || !"RewriteFunction: bogus type check");
    inference = inf;

    Tree *source = rewrite->from;
    Tree *def = rewrite->to;
    IFTRACE(llvm)
        std::cerr << "CompiledUnit::RewriteFunction T" << (void *) source;

    // Extract parameters from source form
    ParameterList parameters(this);
    if (!source->Do(parameters))
        return NULL;

    // Create the function signature, one entry per parameter
    llvm_types signature;
    parameters.Signature(signature);
    llvm_type retTy;

    // Compute return type:
    // - If explicitly specified, use that (TODO: Check compatibility)
    // - For definitions, infer from definition
    // - For data forms, this is the type of the data form
    if (llvm_type specifiedRetTy = parameters.returned)
        retTy = specifiedRetTy;
    else if (def)
        retTy = ReturnType(def);
    else
        retTy = StructureType(signature);

    FunctionType *fnTy = FunctionType::get(retTy, signature, false);

    text label = "xl_eval_" + parameters.name;
    IFTRACE(labels)
        label += "[" + text(*source) + "]";

    return InitializeFunction(fnTy, parameters, label.c_str(), false);
}


Function *CompiledUnit::TopLevelFunction()
// ----------------------------------------------------------------------------
//   Create a function for a top-level program
// ----------------------------------------------------------------------------
{
    // We must have verified the types before
    assert(inference || !"TopLevelFunction called without type check");

    llvm_types signature;
    ParameterList parameters(this);
    llvm_type retTy = compiler->treePtrTy;
    FunctionType *fnTy = FunctionType::get(retTy, signature, false);
    return InitializeFunction(fnTy, parameters, "xl_program", true);
}


Function *CompiledUnit::InitializeFunction(FunctionType *fnTy,
                                           ParameterList &parameters,
                                           kstring label,
                                           bool global)
// ----------------------------------------------------------------------------
//   Build the LLVM function, create entry points, ...
// ----------------------------------------------------------------------------
{
    assert (!function || !"LLVM function was already built");

    // Create function and save it in the CompiledUnit
    global = true;
    function = Function::Create(fnTy,
                                global
                                ? Function::ExternalLinkage
                                : Function::InternalLinkage,
                                label, compiler->module);
    IFTRACE(llvm)
        std::cerr << " new F" << function << "\n";

    // Create function entry point, where we will have all allocas
    allocabb = BasicBlock::Create(*llvm, "allocas", function);
    data = new IRBuilder<> (allocabb);

    // Create entry block for the function
    entrybb = BasicBlock::Create(*llvm, "entry", function);
    code = new IRBuilder<> (entrybb);

    // Associate the value for the input tree
    Function::arg_iterator args = function->arg_begin();
    llvm_type retTy = function->getReturnType();
    returned = data->CreateAlloca(retTy, 0, "result");

    // Associate the value for the additional arguments (read-only, no alloca)
    Parameters &plist = parameters.parameters;
    for (Parameters::iterator p = plist.begin(); p != plist.end(); p++)
    {
        Parameter &parm = *p;
        llvm_value inputArg = args++;
        parm.value = inputArg;
        value[parm.name] = inputArg;
    }

    // Create the exit basic block and return statement
    exitbb = BasicBlock::Create(*llvm, "exit", function);
    IRBuilder<> exitcode(exitbb);
    Value *retVal = exitcode.CreateLoad(returned, "retval");
    exitcode.CreateRet(retVal);

    // Return the newly created function
    return function;
}


bool CompiledUnit::TypeCheck(Tree *program)
// ----------------------------------------------------------------------------
//   Verify that the given program/expression is valid in current context
// ----------------------------------------------------------------------------
{
    TypeInference_p inferTypes = new TypeInference(context);
    bool result = inferTypes->TypeCheck(program);
    if (result)
        inference = inferTypes;
    return result;
}


llvm_value CompiledUnit::Compile(Tree *tree)
// ----------------------------------------------------------------------------
//    Compile a given tree
// ----------------------------------------------------------------------------
{
    assert (inference || !"Compile() called without type checking");
    CompileExpression cexpr(this);
    llvm_value result = tree->Do(cexpr);
    return result;
}


llvm_value CompiledUnit::Compile(Rewrite *rewrite, TypeInference *types)
// ----------------------------------------------------------------------------
//    Compile a given tree
// ----------------------------------------------------------------------------
{
    llvm_value result = value[rewrite->from];
    if (result == NULL)
    {
        Context_p rewriteContext = types->context;
        CompiledUnit rewriteUnit(compiler, rewriteContext);

        Function *function = rewriteUnit.RewriteFunction(rewrite, types);
        value[rewrite->from] = function;
        rewriteUnit.value[rewrite->from] = function;
        result = function;

        llvm_value returned = rewriteUnit.Compile(rewrite->to);
        if (!returned)
            return NULL;
        if (!rewriteUnit.Return(returned))
            return NULL;
        rewriteUnit.Finalize(false);
    }
    return result;
}


llvm_value CompiledUnit::Return(llvm_value value)
// ----------------------------------------------------------------------------
//   Return the given value, after appropriate boxing
// ----------------------------------------------------------------------------
{
    llvm_type retTy = function->getReturnType();
    value = Autobox(value, retTy);
    code->CreateStore(value, returned);
    return value;
}


eval_fn CompiledUnit::Finalize(bool createCode)
// ----------------------------------------------------------------------------
//   Finalize the build of the current function
// ----------------------------------------------------------------------------
{
    IFTRACE(llvm)
        std::cerr << "CompiledUnit Finalize F" << function;

    // Branch to the exit block from the last test we did
    code->CreateBr(exitbb);

    // Connect the "allocas" to the actual entry point
    data->CreateBr(entrybb);

    // Verify the function we built
    verifyFunction(*function);
    if (compiler->optimizer)
        compiler->optimizer->run(*function);

    IFTRACE(code)
    {
        function->print(errs());
    }

    void *result = NULL;
    if (createCode)
    {
        compiler->moduleOptimizer->run(*compiler->module);
        result = compiler->runtime->getPointerToFunction(function);
        IFTRACE(llvm)
            std::cerr << " C" << (void *) result << "\n";
    }

    exitbb = NULL;              // Tell destructor we were successful
    return (eval_fn) result;
}


Value *CompiledUnit::NeedStorage(Tree *tree)
// ----------------------------------------------------------------------------
//    Allocate storage for a given tree
// ----------------------------------------------------------------------------
{
    assert(inference || !"Storage() called without type check");

    Value *result = storage[tree];
    if (!result)
    {
        // Get the associated machine type
        llvm_type mtype = ExpressionMachineType(tree);

        // Create alloca to store the new form
        text label = "loc";
        IFTRACE(labels)
            label += "[" + text(*tree) + "]";
        const char *clabel = label.c_str();
        result = data->CreateAlloca(mtype, 0, clabel);
        storage[tree] = result;

        // If this started with a value or global, initialize on function entry
        if (value.count(tree))
            data->CreateStore(value[tree], result);
        else if (Value *global = compiler->TreeGlobal(tree))
            data->CreateStore(data->CreateLoad(global), result);
    }

    return result;
}


bool CompiledUnit::IsKnown(Tree *tree, uint which)
// ----------------------------------------------------------------------------
//   Check if the tree has a known local or global value
// ----------------------------------------------------------------------------
{
    if ((which & knowLocals) && storage.count(tree) > 0)
        return true;
    else if ((which & knowValues) && value.count(tree) > 0)
        return true;
    else if (which & knowGlobals)
        if (compiler->IsKnown(tree))
            return true;
    return false;
}


Value *CompiledUnit::Known(Tree *tree, uint which)
// ----------------------------------------------------------------------------
//   Return the known local or global value if any
// ----------------------------------------------------------------------------
{
    Value *result = NULL;
    if ((which & knowLocals) && storage.count(tree) > 0)
    {
        // Value is stored in a local variable
        result = code->CreateLoad(storage[tree], "loc");
    }
    else if ((which & knowValues) && value.count(tree) > 0)
    {
        // Immediate value of some sort, use that
        result = value[tree];
    }
    else if (which & knowGlobals)
    {
        // Check if this is a global
        result = compiler->TreeGlobal(tree);
        if (result)
        {
            text label = "glob";
            IFTRACE(labels)
                label += "[" + text(*tree) + "]";
            result = code->CreateLoad(result, label);
        }
    }
    return result;
}


Value *CompiledUnit::ConstantInteger(Integer *what)
// ----------------------------------------------------------------------------
//    Generate an Integer tree
// ----------------------------------------------------------------------------
{
    Value *result = Known(what, knowGlobals);
    if (!result)
    {
        result = compiler->EnterConstant(what);
        result = code->CreateLoad(result, "intk");
        if (storage.count(what))
            code->CreateStore(result, storage[what]);
    }
    return result;
}


Value *CompiledUnit::ConstantReal(Real *what)
// ----------------------------------------------------------------------------
//    Generate a Real tree
// ----------------------------------------------------------------------------
{
    Value *result = Known(what, knowGlobals);
    if (!result)
    {
        result = compiler->EnterConstant(what);
        result = code->CreateLoad(result, "realk");
        if (storage.count(what))
            code->CreateStore(result, storage[what]);
    }
    return result;
}


Value *CompiledUnit::ConstantText(Text *what)
// ----------------------------------------------------------------------------
//    Generate a Text tree
// ----------------------------------------------------------------------------
{
    Value *result = Known(what, knowGlobals);
    if (!result)
    {
        result = compiler->EnterConstant(what);
        result = code->CreateLoad(result, "textk");
        if (storage.count(what))
            code->CreateStore(result, storage[what]);
    }
    return result;
}


Value *CompiledUnit::ConstantTree(Tree *what)
// ----------------------------------------------------------------------------
//    Generate a constant tree
// ----------------------------------------------------------------------------
{
    Value *result = Known(what, knowGlobals);
    if (!result)
    {
        result = compiler->EnterConstant(what);
        result = data->CreateLoad(result, "treek");
    }
    return result;
}


Value *CompiledUnit::NeedLazy(Tree *subexpr, bool allocate)
// ----------------------------------------------------------------------------
//   Record that we need a 'computed' flag for lazy evaluation of the subexpr
// ----------------------------------------------------------------------------
{
    Value *result = computed[subexpr];
    if (!result && allocate)
    {
        text label = "computed";
        IFTRACE(labels)
            label += "[" + text(*subexpr) + "]";

        result = data->CreateAlloca(LLVM_BOOLTYPE, 0, label.c_str());
        Value *falseFlag = ConstantInt::get(LLVM_BOOLTYPE, 0);
        data->CreateStore(falseFlag, result);
        computed[subexpr] = result;
    }
    return result;
}


llvm::Value *CompiledUnit::MarkComputed(Tree *subexpr, Value *val)
// ----------------------------------------------------------------------------
//   Record that we computed that particular subexpression
// ----------------------------------------------------------------------------
{
    // Store the value we were given as the result
    if (val)
    {
        if (storage.count(subexpr) > 0)
            code->CreateStore(val, storage[subexpr]);
    }

    // Set the 'lazy' flag or lazy evaluation
    Value *result = NeedLazy(subexpr);
    code->CreateStore(code->getTrue(), result);

    // Return the test flag
    return result;
}


BasicBlock *CompiledUnit::BeginLazy(Tree *subexpr)
// ----------------------------------------------------------------------------
//    Begin lazy evaluation of a block of code
// ----------------------------------------------------------------------------
{
    text lskip = "skip";
    text lwork = "work";
    text llazy = "lazy";
    IFTRACE(labels)
    {
        text lbl = text("[") + text(*subexpr) + "]";
        lskip += lbl;
        lwork += lbl;
        llazy += lbl;
    }
    BasicBlock *skip = BasicBlock::Create(*llvm, lskip, function);
    BasicBlock *work = BasicBlock::Create(*llvm, lwork, function);

    Value *lazyFlagPtr = computed[subexpr];
    Value *lazyFlag = code->CreateLoad(lazyFlagPtr, llazy);
    code->CreateCondBr(lazyFlag, skip, work);

    code->SetInsertPoint(work);
    return skip;
}


void CompiledUnit::EndLazy(Tree *subexpr, llvm::BasicBlock *skip)
// ----------------------------------------------------------------------------
//   Finish lazy evaluation of a block of code
// ----------------------------------------------------------------------------
{
    (void) subexpr;
    code->CreateBr(skip);
    code->SetInsertPoint(skip);
}


llvm::Value *CompiledUnit::Invoke(Tree *subexpr, Tree *callee, TreeList args)
// ----------------------------------------------------------------------------
//    Generate a call with the given arguments
// ----------------------------------------------------------------------------
{
    // Check if the resulting form is a name or literal
    if (callee->IsConstant())
    {
        if (Value *known = Known(callee))
        {
            MarkComputed(subexpr, known);
            return known;
        }
        else
        {
            std::cerr << "No value for xl_identity tree " << callee << '\n';
        }
    }

    Function *toCall = compiler->TreeFunction(callee); assert(toCall);

    // Add the 'self' argument
    std::vector<Value *> argV;
    Value *defaultVal = ConstantTree(subexpr);
    argV.push_back(defaultVal);

    TreeList::iterator a;
    for (a = args.begin(); a != args.end(); a++)
    {
        Tree *arg = *a;
        Value *value = Known(arg);
        if (!value)
            value = ConstantTree(arg);
        argV.push_back(value);
    }

    Value *callVal = code->CreateCall(toCall, argV.begin(), argV.end());

    // Store the flags indicating that we computed the value
    MarkComputed(subexpr, callVal);

    return callVal;
}


BasicBlock *CompiledUnit::NeedTest()
// ----------------------------------------------------------------------------
//    Indicates that we need an exit basic block to jump to
// ----------------------------------------------------------------------------
{
    if (!failbb)
        failbb = BasicBlock::Create(*llvm, "fail", function);
    return failbb;
}


Value *CompiledUnit::Left(Tree *tree)
// ----------------------------------------------------------------------------
//    Return the value for the left of the current tree
// ----------------------------------------------------------------------------
{
    // Check that the tree has the expected kind
    assert (tree->Kind() >= BLOCK);

    // Check if we already know the result, if so just return it
    // HACK: The following code assumes Prefix, Infix and Postfix have the
    // same layout for their pointers.
    Prefix *prefix = (Prefix *) tree;
    Value *result = Known(prefix->left);
    if (result)
        return result;

    // Check that we already have a value for the given tree
    Value *parent = Known(tree);
    if (parent)
    {
        Value *ptr = NeedStorage(prefix->left);

        // WARNING: This relies on the layout of all nodes beginning the same
        Value *pptr = code->CreateBitCast(parent, compiler->prefixTreePtrTy,
                                          "pfxl");
        result = code->CreateConstGEP2_32(pptr, 0,
                                          LEFT_VALUE_INDEX, "lptr");
        result = code->CreateLoad(result, "left");
        code->CreateStore(result, ptr);
    }
    else
    {
        Ooops("Internal: Using left of uncompiled $1", tree);
    }

    return result;
}


Value *CompiledUnit::Right(Tree *tree)
// ----------------------------------------------------------------------------
//    Return the value for the right of the current tree
// ----------------------------------------------------------------------------
{
    // Check that the tree has the expected kind
    assert(tree->Kind() > BLOCK);

    // Check if we already known the result, if so just return it
    // HACK: The following code assumes Prefix, Infix and Postfix have the
    // same layout for their pointers.
    Prefix *prefix = (Prefix *) tree;
    Value *result = Known(prefix->right);
    if (result)
        return result;

    // Check that we already have a value for the given tree
    Value *parent = Known(tree);
    if (parent)
    {
        Value *ptr = NeedStorage(prefix->right);

        // WARNING: This relies on the layout of all nodes beginning the same
        Value *pptr = code->CreateBitCast(parent, compiler->prefixTreePtrTy,
                                          "pfxr");
        result = code->CreateConstGEP2_32(pptr, 0,
                                          RIGHT_VALUE_INDEX, "rptr");
        result = code->CreateLoad(result, "right");
        code->CreateStore(result, ptr);
    }
    else
    {
        Ooops("Internal: Using right of uncompiled $14", tree);
    }
    return result;
}


Value *CompiledUnit::Copy(Tree *source, Tree *dest, bool markDone)
// ----------------------------------------------------------------------------
//    Copy data from source to destination
// ----------------------------------------------------------------------------
{
    Value *result = Known(source); assert(result);
    Value *ptr = NeedStorage(dest); assert(ptr);
    code->CreateStore(result, ptr);

    if (markDone)
    {
        // Set the target flag to 'done'
        Value *doneFlag = NeedLazy(dest);
        Value *trueFlag = ConstantInt::get(LLVM_BOOLTYPE, 1);
        code->CreateStore(trueFlag, doneFlag);
    }
    else if (Value *oldDoneFlag = NeedLazy(source, false))
    {
        // Copy the flag from the source
        Value *newDoneFlag = NeedLazy(dest);
        Value *computed = code->CreateLoad(oldDoneFlag);
        code->CreateStore(computed, newDoneFlag);
    }

    return result;
}


Value *CompiledUnit::CallEvaluate(Tree *tree)
// ----------------------------------------------------------------------------
//   Call the evaluate function for the given tree
// ----------------------------------------------------------------------------
{
    Value *treeValue = Known(tree); assert(treeValue);
    if (dataForm.count(tree))
        return treeValue;

    Value *evaluated = code->CreateCall(compiler->xl_evaluate, treeValue);
    MarkComputed(tree, evaluated);
    return evaluated;
}


Value *CompiledUnit::CallNewBlock(Block *block)
// ----------------------------------------------------------------------------
//    Compile code generating the children of the block
// ----------------------------------------------------------------------------
{
    Value *blockValue = ConstantTree(block);
    Value *childValue = Known(block->child);
    Value *result = code->CreateCall2(compiler->xl_new_block,
                                      blockValue, childValue);
    MarkComputed(block, result);
    return result;
}


Value *CompiledUnit::CallNewPrefix(Prefix *prefix)
// ----------------------------------------------------------------------------
//    Compile code generating the children of a prefix
// ----------------------------------------------------------------------------
{
    Value *prefixValue = ConstantTree(prefix);
    Value *leftValue = Known(prefix->left);
    Value *rightValue = Known(prefix->right);
    Value *result = code->CreateCall3(compiler->xl_new_prefix,
                                      prefixValue, leftValue, rightValue);
    MarkComputed(prefix, result);
    return result;
}


Value *CompiledUnit::CallNewPostfix(Postfix *postfix)
// ----------------------------------------------------------------------------
//    Compile code generating the children of a postfix
// ----------------------------------------------------------------------------
{
    Value *postfixValue = ConstantTree(postfix);
    Value *leftValue = Known(postfix->left);
    Value *rightValue = Known(postfix->right);
    Value *result = code->CreateCall3(compiler->xl_new_postfix,
                                      postfixValue, leftValue, rightValue);
    MarkComputed(postfix, result);
    return result;
}


Value *CompiledUnit::CallNewInfix(Infix *infix)
// ----------------------------------------------------------------------------
//    Compile code generating the children of an infix
// ----------------------------------------------------------------------------
{
    Value *infixValue = ConstantTree(infix);
    Value *leftValue = Known(infix->left);
    Value *rightValue = Known(infix->right);
    Value *result = code->CreateCall3(compiler->xl_new_infix,
                                      infixValue, leftValue, rightValue);
    MarkComputed(infix, result);
    return result;
}


Value *CompiledUnit::CreateClosure(Tree *callee, TreeList &args)
// ----------------------------------------------------------------------------
//   Create a closure for an expression we want to evaluate later
// ----------------------------------------------------------------------------
{
    std::vector<Value *> argV;
    Value *calleeVal = Known(callee);
    if (!calleeVal)
        return NULL;
    Value *countVal = ConstantInt::get(LLVM_INTTYPE(uint), args.size());
    TreeList::iterator a;

    argV.push_back(calleeVal);
    argV.push_back(countVal);
    for (a = args.begin(); a != args.end(); a++)
    {
        Tree *value = *a;
        Value *llvmValue = Known(value); assert(llvmValue);
        argV.push_back(llvmValue);
    }

    Value *callVal = code->CreateCall(compiler->xl_new_closure,
                                      argV.begin(), argV.end());

    // Need to store result, but not mark it as evaluated
    NeedStorage(callee);
    code->CreateStore(callVal, storage[callee]);
    // MarkComputed(callee, callVal);

    return callVal;
}


Value *CompiledUnit::CallClosure(Tree *callee, uint ntrees)
// ----------------------------------------------------------------------------
//   Call a closure function with the given n trees
// ----------------------------------------------------------------------------
//   We build it with an indirect call so that we generate one closure call
//   subroutine per number of arguments only.
//   The input is a prefix of the form E X1 X2 X3 false, where E is the
//   expression to evaluate, and X1, X2, X3 are the arguments it needs.
//   The generated function takes the 'code' field of E, and calls it
//   using C conventions with arguments (E, X1, X2, X3).
{
    // Load left tree and get its code tag
    Type *treePtrTy = compiler->treePtrTy;
    Value *ptr = Known(callee); assert(ptr);
    Value *pfx = code->CreateBitCast(ptr,compiler->prefixTreePtrTy);
    Value *lf = code->CreateConstGEP2_32(pfx, 0, LEFT_VALUE_INDEX);
    Value *callTree = code->CreateLoad(lf);
#define CODE_INDEX 0
    Value *callCode = code->CreateConstGEP2_32(callTree, 0, CODE_INDEX);
    callCode = code->CreateLoad(callCode);

    // Build argument list
    std::vector<Value *> argV;
    std::vector<const Type *> signature;
    argV.push_back(callTree);     // Self is the original expression
    signature.push_back(treePtrTy);
    for (uint i = 0; i < ntrees; i++)
    {
        // WARNING: This relies on the layout of all nodes beginning the same
        Value *pfx = code->CreateBitCast(ptr,compiler->prefixTreePtrTy);
        Value *rt = code->CreateConstGEP2_32(pfx, 0, RIGHT_VALUE_INDEX);
        ptr = code->CreateLoad(rt);
        pfx = code->CreateBitCast(ptr,compiler->prefixTreePtrTy);
        Value *lf = code->CreateConstGEP2_32(pfx, 0, LEFT_VALUE_INDEX);
        Value *arg = code->CreateLoad(lf);
        argV.push_back(arg);
        signature.push_back(treePtrTy);
    }

    // Call the resulting function
    FunctionType *fnTy = FunctionType::get(treePtrTy, signature, false);
    PointerType *fnPtrTy = PointerType::get(fnTy, 0);
    Value *toCall = code->CreateBitCast(callCode, fnPtrTy);
    Value *callVal = code->CreateCall(toCall, argV.begin(), argV.end());

    // Store the flags indicating that we computed the value
    MarkComputed(callee, callVal);

    return callVal;

}


Value *CompiledUnit::CallFormError(Tree *what)
// ----------------------------------------------------------------------------
//   Report a type error trying to evaluate some argument
// ----------------------------------------------------------------------------
{
    Value *ptr = ConstantTree(what); assert(what);
    Value *callVal = code->CreateCall(compiler->xl_form_error, ptr);
    return callVal;
}


BasicBlock *CompiledUnit::TagTest(Tree *tree, ulong tagValue)
// ----------------------------------------------------------------------------
//   Test if the input tree is an integer tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();

    // Check if the tag is INTEGER
    Value *treeValue = Known(tree);
    if (!treeValue)
    {
        Ooops("No value for $1", tree);
        return NULL;
    }
    Value *tagPtr = code->CreateConstGEP2_32(treeValue, 0, 0, "tagPtr");
    Value *tag = code->CreateLoad(tagPtr, "tag");
    Value *mask = ConstantInt::get(tag->getType(), Tree::KINDMASK);
    Value *kind = code->CreateAnd(tag, mask, "tagAndMask");
    Constant *refTag = ConstantInt::get(tag->getType(), tagValue);
    Value *isRightTag = code->CreateICmpEQ(kind, refTag, "isRightTag");
    BasicBlock *isRightKindBB = BasicBlock::Create(*llvm,
                                                   "isRightKind", function);
    code->CreateCondBr(isRightTag, isRightKindBB, notGood);

    code->SetInsertPoint(isRightKindBB);
    return isRightKindBB;
}


BasicBlock *CompiledUnit::IntegerTest(Tree *tree, longlong value)
// ----------------------------------------------------------------------------
//   Test if the input tree is an integer tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();

    // Check if the tag is INTEGER
    BasicBlock *isIntegerBB = TagTest(tree, INTEGER);
    if (!isIntegerBB)
        return isIntegerBB;

    // Check if the value is the same
    Value *treeValue = Known(tree);
    assert(treeValue);
    treeValue = code->CreateBitCast(treeValue, compiler->integerTreePtrTy);
    Value *valueFieldPtr = code->CreateConstGEP2_32(treeValue, 0,
                                                    INTEGER_VALUE_INDEX);
    Value *tval = code->CreateLoad(valueFieldPtr, "treeValue");
    Constant *rval = ConstantInt::get(tval->getType(), value, "refValue");
    Value *isGood = code->CreateICmpEQ(tval, rval, "isGood");
    BasicBlock *isGoodBB = BasicBlock::Create(*llvm,
                                              "isGood", function);
    code->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code->SetInsertPoint(isGoodBB);
    return isGoodBB;
}


BasicBlock *CompiledUnit::RealTest(Tree *tree, double value)
// ----------------------------------------------------------------------------
//   Test if the input tree is a real tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();

    // Check if the tag is REAL
    BasicBlock *isRealBB = TagTest(tree, REAL);
    if (!isRealBB)
        return isRealBB;

    // Check if the value is the same
    Value *treeValue = Known(tree);
    assert(treeValue);
    treeValue = code->CreateBitCast(treeValue, compiler->realTreePtrTy);
    Value *valueFieldPtr = code->CreateConstGEP2_32(treeValue, 0,
                                                       REAL_VALUE_INDEX);
    Value *tval = code->CreateLoad(valueFieldPtr, "treeValue");
    Constant *rval = ConstantFP::get(tval->getType(), value);
    Value *isGood = code->CreateFCmpOEQ(tval, rval, "isGood");
    BasicBlock *isGoodBB = BasicBlock::Create(*llvm,
                                              "isGood", function);
    code->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code->SetInsertPoint(isGoodBB);
    return isGoodBB;
}


BasicBlock *CompiledUnit::TextTest(Tree *tree, text value)
// ----------------------------------------------------------------------------
//   Test if the input tree is a text tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();

    // Check if the tag is TEXT
    BasicBlock *isTextBB = TagTest(tree, TEXT);
    if (!isTextBB)
        return isTextBB;

    // Check if the value is the same, call xl_same_text
    Value *treeValue = Known(tree);
    assert(treeValue);
    GlobalVariable *global = compiler->TextConstant(value);
    Value *refPtr = code->CreateConstGEP2_32(global, 0, 0);
    Value *isGood = code->CreateCall2(compiler->xl_same_text,
                                      treeValue, refPtr);
    BasicBlock *isGoodBB = BasicBlock::Create(*llvm, "isGood", function);
    code->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code->SetInsertPoint(isGoodBB);
    return isGoodBB;
}


BasicBlock *CompiledUnit::ShapeTest(Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//   Test if the two given trees have the same shape
// ----------------------------------------------------------------------------
{
    Value *leftVal = Known(left);
    Value *rightVal = Known(right);
    assert(leftVal);
    assert(rightVal);
    if (leftVal == rightVal) // How unlikely?
        return NULL;

    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();
    Value *isGood = code->CreateCall2(compiler->xl_same_shape,
                                      leftVal, rightVal);
    BasicBlock *isGoodBB = BasicBlock::Create(*llvm, "isGood", function);
    code->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code->SetInsertPoint(isGoodBB);
    return isGoodBB;
}


BasicBlock *CompiledUnit::InfixMatchTest(Tree *actual, Infix *reference)
// ----------------------------------------------------------------------------
//   Test if the actual tree has the same shape as the given infix
// ----------------------------------------------------------------------------
{
    // Check that we know how to evaluate both
    Value *actualVal = Known(actual);           assert(actualVal);
    Value *refVal = NeedStorage(reference);     assert (refVal);

    // Extract the name of the reference
    Constant *refNameVal = ConstantArray::get(*llvm, reference->name);
    const Type *refNameTy = refNameVal->getType();
    GlobalVariable *gvar = new GlobalVariable(*compiler->module,refNameTy,true,
                                              GlobalValue::InternalLinkage,
                                              refNameVal, "infix_name");
    Value *refNamePtr = code->CreateConstGEP2_32(gvar, 0, 0);

    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();
    Value *afterExtract = code->CreateCall2(compiler->xl_infix_match_check,
                                            actualVal, refNamePtr);
    Constant *null = ConstantPointerNull::get(compiler->treePtrTy);
    Value *isGood = code->CreateICmpNE(afterExtract, null, "isGoodInfix");
    BasicBlock *isGoodBB = BasicBlock::Create(*llvm, "isGood", function);
    code->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code->SetInsertPoint(isGoodBB);

    // We are on the right path: extract left and right
    code->CreateStore(afterExtract, refVal);
    MarkComputed(reference, NULL);
    MarkComputed(reference->left, NULL);
    MarkComputed(reference->right, NULL);
    Left(reference);
    Right(reference);

    return isGoodBB;
}


BasicBlock *CompiledUnit::TypeTest(Tree *value, Tree *type)
// ----------------------------------------------------------------------------
//   Test if the given value has the given type
// ----------------------------------------------------------------------------
{
    Value *valueVal = Known(value);     assert(valueVal);
    Value *typeVal = Known(type);       assert(typeVal);

    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();
    Value *afterCast = code->CreateCall2(compiler->xl_type_check,
                                         valueVal, typeVal);
    Constant *null = ConstantPointerNull::get(compiler->treePtrTy);
    Value *isGood = code->CreateICmpNE(afterCast, null, "isGoodType");
    BasicBlock *isGoodBB = BasicBlock::Create(*llvm, "isGood", function);
    code->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value matched, we may have a type cast, remember it
    code->SetInsertPoint(isGoodBB);
    Value *ptr = NeedStorage(value);
    code->CreateStore(afterCast, ptr);

    return isGoodBB;
}


llvm_type CompiledUnit::ReturnType(Tree *form)
// ----------------------------------------------------------------------------
//   Compute the return type associated with the given form
// ----------------------------------------------------------------------------
{
    // Type inference gives us the return type for this form
    Tree *type = inference->Type(form);
    llvm_type mtype = compiler->MachineType(type);
    return mtype;
}


llvm_type CompiledUnit::StructureType(llvm_types &signature)
// ----------------------------------------------------------------------------
//   Compute the return type associated with the given data form
// ----------------------------------------------------------------------------
{
    StructType *stype = StructType::get(*llvm, signature);
    return stype;
}


llvm_type CompiledUnit::ExpressionMachineType(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the machine type associated with a given expression
// ----------------------------------------------------------------------------
{
    assert(inference || !"ExpressionMachineType without type check");

    Tree *type = inference->Type(expr);
    return compiler->MachineType(type);
}


llvm_type CompiledUnit::MachineType(Tree *type)
// ----------------------------------------------------------------------------
//   Return the machine type associated with a given type
// ----------------------------------------------------------------------------
{
    assert(inference || !"ExpressionMachineType without type check");

    type = inference->Base(type);
    return compiler->MachineType(type);
}


llvm_value CompiledUnit::Autobox(llvm_value value, llvm_type req)
// ----------------------------------------------------------------------------
//   Automatically box/unbox primitive types
// ----------------------------------------------------------------------------
//   Primitive values like integers can exist in two forms during execution:
//   - In boxed form, e.g. as a pointer to an instance of Integer
//   - In native form, e.g. as an integer
//   This function automatically converts from one to the other as necessary
{
    llvm_type  type   = value->getType();
    llvm_value result = value;
    Function * boxFn  = NULL;

    // Short circuit if we are already there
    if (req == type)
        return result;

    if (req == compiler->booleanTy)
    {
        assert (type == compiler->treePtrTy || type == compiler->nameTreePtrTy);
        Value *falsePtr = compiler->TreeGlobal(xl_false);
        result = code->CreateLoad(falsePtr, "xl_false");
        result = code->CreateICmpNE(value, result, "notFalse");
    }
    else if (req == compiler->integerTy)
    {
        assert (type == compiler->integerTreePtrTy);
        result = code->CreateConstGEP2_32(value,0, INTEGER_VALUE_INDEX, "ival");
    }
    else if (req == compiler->realTy)
    {
        assert(type == compiler->realTreePtrTy);
        result = code->CreateConstGEP2_32(value,0, REAL_VALUE_INDEX, "rval");
    }
    else if (req == compiler->characterTy)
    {
        assert(type == compiler->textTreePtrTy);
        result = code->CreateConstGEP2_32(result,0, TEXT_VALUE_INDEX, "tval");
        result = code->CreateConstGEP2_32(result, 0, 0, "cptrval");
        result = code->CreateConstGEP2_32(result, 0, 0, "charval");
    }
    else if (req == compiler->charPtrTy)
    {
        assert(type == compiler->textTreePtrTy);
        result = code->CreateConstGEP2_32(result,0, TEXT_VALUE_INDEX, "tval");
        result = code->CreateConstGEP2_32(result,0, TEXT_VALUE_INDEX,"cptrval");
    }
    else if (req == compiler->textTy)
    {
        assert (type == compiler->textTreePtrTy);
        result = code->CreateConstGEP2_32(result,0, TEXT_VALUE_INDEX, "tval");
    }
    else if (type == compiler->booleanTy)
    {
        assert(req == compiler->treePtrTy || req == compiler->nameTreePtrTy);

        // Insert code corresponding to value ? xl_true : xl_false
        BasicBlock *isTrue = BasicBlock::Create(*llvm, "isTrue", function);
        BasicBlock *isFalse = BasicBlock::Create(*llvm, "isFalse", function);
        BasicBlock *exit = BasicBlock::Create(*llvm, "booleanBoxed", function);
        code->CreateCondBr(value, isTrue, isFalse);

        // True block
        code->SetInsertPoint(isTrue);
        Value *truePtr = compiler->TreeGlobal(xl_true);
        result = code->CreateLoad(truePtr, "xl_true");
        code->CreateBr(exit);

        // False block
        code->SetInsertPoint(isFalse);
        Value *falsePtr = compiler->TreeGlobal(xl_false);
        result = code->CreateLoad(falsePtr, "xl_false");
        code->CreateBr(exit);

        // Now on shared exit block
        code->SetInsertPoint(exit);
    }
    else if (type == compiler->integerTy)
    {
        assert(req == compiler->treePtrTy || req == compiler->integerTreePtrTy);
        boxFn = compiler->xl_new_integer;
    }
    else if (type == compiler->realTy)
    {
        assert(req == compiler->treePtrTy || req == compiler->realTreePtrTy);
        boxFn = compiler->xl_new_real;
    }
    else if (type == compiler->characterTy)
    {
        assert(req == compiler->treePtrTy || req == compiler->textTreePtrTy);
        boxFn = compiler->xl_new_character;
    }
    else if (type == compiler->textTy)
    {
        assert(req == compiler->treePtrTy || req == compiler->textTreePtrTy);
        boxFn = compiler->xl_new_text;
    }
    else if (type == compiler->charPtrTy)
    {
        assert(req == compiler->treePtrTy || req == compiler->textTreePtrTy);
        boxFn = compiler->xl_new_ctext;
    }

    // If we need to invoke a boxing function, do it now
    if (boxFn)
    {
        result = code->CreateCall(boxFn, value);
        type = result->getType();
    }


    if (req == compiler->treePtrTy)
    {
        assert(type == compiler->integerTreePtrTy ||
               type == compiler->realTreePtrTy ||
               type == compiler->textTreePtrTy ||
               type == compiler->nameTreePtrTy ||
               type == compiler->blockTreePtrTy ||
               type == compiler->prefixTreePtrTy ||
               type == compiler->postfixTreePtrTy ||
               type == compiler->infixTreePtrTy);
        result = code->CreateBitCast(result, req);
    }

    // Return what we built if anything
    return result;
}


Value *CompiledUnit::Global(Tree *tree)
// ----------------------------------------------------------------------------
//   Return a global value if there is any
// ----------------------------------------------------------------------------
{
    // Check if this is a global
    Value *result = compiler->TreeGlobal(tree);
    if (result)
    {
        text label = "glob";
        IFTRACE(labels)
            label += "[" + text(*tree) + "]";
        result = code->CreateLoad(result, label);
    }
    return result;
}

XL_END
