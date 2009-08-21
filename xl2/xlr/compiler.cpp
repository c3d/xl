// ****************************************************************************
//  compiler.cpp                    (C) 1992-2009 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Just-in-time compilation of XL trees
// 
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
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "compiler.h"
#include "options.h"
#include "context.h"
#include "renderer.h"

#include <iostream>
#include <sstream>
#include <cstdarg>

#include <llvm/Analysis/Verifier.h>
#include <llvm/CallingConv.h>
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include <llvm/ModuleProvider.h>
#include <llvm/PassManager.h>
#include "llvm/Support/raw_ostream.h"
#include <llvm/Support/IRBuilder.h>
#include <llvm/System/DynamicLibrary.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

XL_BEGIN


// ============================================================================
// 
//    Compiler - Compiler environment
// 
// ============================================================================

using namespace llvm;

static void* unresolved_external(const std::string& name)
// ----------------------------------------------------------------------------
//   Resolve external names that dyld doesn't know about
// ----------------------------------------------------------------------------
// This is really just to print a fancy error message
{
    std::cout.flush();
    std::cerr << "Unable to resolve external: " << name << std::endl;
    assert(0);
    return 0;
}


Compiler::Compiler(kstring moduleName)
// ----------------------------------------------------------------------------
//   Initialize the various instances we may need
// ----------------------------------------------------------------------------
    : module(NULL), provider(NULL), runtime(NULL), optimizer(NULL),
      treeTy(NULL), treePtrTy(NULL), treePtrPtrTy(NULL),
      integerTreeTy(NULL), integerTreePtrTy(NULL),
      realTreeTy(NULL), realTreePtrTy(NULL),
      prefixTreeTy(NULL), prefixTreePtrTy(NULL),
      evalTy(NULL), evalFnTy(NULL),
      xl_evaluate(NULL), xl_same_text(NULL), xl_same_shape(NULL),
      xl_type_check(NULL),
      functions()
{
    // Thanks to Dr. Albert Graef (pure programming language) for inspiration

    // Create module where we will build the code
    module = new Module(moduleName);
    provider = new ExistingModuleProvider(module);

    // Select "fast JIT" if optimize level is 0, optimizing JIT otherwise
    if (command_line_options->optimize_level == 0)
        runtime = ExecutionEngine::create(provider, false, 0);
    else
        runtime = ExecutionEngine::create(provider);

    // Setup the optimizer - REVISIT: Adjust with optimization level
    optimizer = new FunctionPassManager(provider);
    {
        // Register target data structure layout info
        optimizer->add(new TargetData(*runtime->getTargetData()));

        // Promote allocas to registers.
        optimizer->add(createPromoteMemoryToRegisterPass());

        // Do simple "peephole" optimizations and bit-twiddling optimizations.
        optimizer->add(createInstructionCombiningPass());

        // Reassociate expressions.
        optimizer->add(createReassociatePass());

        // Eliminate common subexpressions.
        optimizer->add(createGVNPass());

        // Simplify the control flow graph (deleting unreachable blocks, etc).
        optimizer->add(createCFGSimplificationPass());
    }

    // Install a fallback mechanism to resolve references to the runtime, on
    // systems which do not allow the program to dlopen itself.
    runtime->InstallLazyFunctionCreator(unresolved_external);

    // Create the eval_fn type
    PATypeHolder structTreeTy = OpaqueType::get();      // struct Tree
    treePtrTy = PointerType::get(structTreeTy, 0);      // Tree *
    treePtrPtrTy = PointerType::get(treePtrTy, 0);      // Tree **
    std::vector<const Type *> evalParms;
    evalParms.push_back(treePtrTy);
    evalTy = FunctionType::get(treePtrTy, evalParms, false);
    evalFnTy = PointerType::get(evalTy, 0);

    // Create the Tree type
    std::vector<const Type *> treeElements;
    treeElements.push_back(LLVM_INTTYPE(ulong));        // tag
    treeElements.push_back(evalFnTy);			// code
    treeElements.push_back(treePtrTy);			// type
    treeTy = StructType::get(treeElements);		// struct Tree {}
    cast<OpaqueType>(structTreeTy.get())->refineAbstractTypeTo(treeTy);
    treeTy = cast<StructType> (structTreeTy.get());

    // Create the Integer type
    std::vector<const Type *> integerElements = treeElements;
    integerElements.push_back(LLVM_INTTYPE(longlong));  // value
    integerTreeTy = StructType::get(integerElements);   // struct Integer{}
    integerTreePtrTy = PointerType::get(integerTreeTy,0); // Integer *
#define INTEGER_VALUE_INDEX     3

    // Create the Real type
    std::vector<const Type *> realElements = treeElements;
    realElements.push_back(Type::DoubleTy);             // value
    realTreeTy = StructType::get(realElements);         // struct Real{}
    realTreePtrTy = PointerType::get(realTreeTy, 0);    // Real *
#define REAL_VALUE_INDEX        3

    // Create the Prefix type (which we also use for Infix and Block)
    std::vector<const Type *> prefixElements = treeElements;
    prefixElements.push_back(treePtrTy);                // Tree *
    prefixElements.push_back(treePtrTy);                // Tree *
    prefixTreeTy = StructType::get(prefixElements);     // struct Prefix {}
    prefixTreePtrTy = PointerType::get(prefixTreeTy, 0);// Prefix *
#define LEFT_VALUE_INDEX        3
#define RIGHT_VALUE_INDEX       4

    // Record the type names
    module->addTypeName("tree", treeTy);
    module->addTypeName("treeptr", treePtrTy);
    module->addTypeName("treeptrptr", treePtrPtrTy);
    module->addTypeName("integer", integerTreeTy);
    module->addTypeName("integerptr", integerTreePtrTy);
    module->addTypeName("real", realTreeTy);
    module->addTypeName("realptr", realTreePtrTy);
    module->addTypeName("eval", evalTy);
    module->addTypeName("evalfn", evalFnTy);
    module->addTypeName("prefix", prefixTreeTy);
    module->addTypeName("prefixptr", prefixTreePtrTy);

    // Create a reference to the evaluation function
    Type *charPtrTy = PointerType::get(LLVM_INTTYPE(char), 0);
    const Type *boolTy = Type::Int1Ty;
#define FN(x) #x, (void *) XL::x
    xl_evaluate = ExternFunction(FN(xl_evaluate),
                                 treePtrTy, 1, treePtrTy);
    xl_same_text = ExternFunction(FN(xl_same_text),
                                  boolTy, 2, treePtrTy, charPtrTy);
    xl_same_shape = ExternFunction(FN(xl_same_shape),
                                   boolTy, 2, treePtrTy, treePtrTy);
    xl_type_check = ExternFunction(FN(xl_type_check),
                                   boolTy, 2, treePtrTy, treePtrTy);
    xl_new_integer = ExternFunction(FN(xl_new_integer),
                                    treePtrTy, 1, LLVM_INTTYPE(longlong));
    xl_new_real = ExternFunction(FN(xl_new_real),
                                 treePtrTy, 1, Type::DoubleTy);
    xl_new_character = ExternFunction(FN(xl_new_character),
                                      treePtrTy, 1, charPtrTy);
    xl_new_text = ExternFunction(FN(xl_new_text),
                                 treePtrTy, 1, charPtrTy);
    xl_new_text = ExternFunction(FN(xl_new_xtext),
                                 treePtrTy, 3, charPtrTy, charPtrTy, charPtrTy);
}


Function *Compiler::EnterBuiltin(text name,
                                 Tree *from, Tree *to,
                                 eval_fn code)
// ----------------------------------------------------------------------------
//   Declare a built-in function
// ----------------------------------------------------------------------------
//   The input is not technically an eval_fn, but has as many parameters as
//   there are variables in the form
{
    // Find the parameters
    tree_list parms;
    Context *context = Context::context;
    context->ParameterList (from, parms);

    // Create the LLVM function
    std::vector<const Type *> parmTypes;
    parmTypes.push_back(treePtrTy); // First arg is self
    for (tree_list::iterator p = parms.begin(); p != parms.end(); p++)
        parmTypes.push_back(treePtrTy);
    FunctionType *fnTy = FunctionType::get(treePtrTy, parmTypes, false);
    Function *result = Function::Create(fnTy, Function::ExternalLinkage,
                                        name, module);

    // Record the runtime symbol address
    sys::DynamicLibrary::AddSymbol(name, (void*) code);

    // Associate the function with the tree form
    functions[to] = result;

    return result;    
}


Function *Compiler::ExternFunction(kstring name, void *address,
                                   const Type *retType, uint parmCount, ...)
// ----------------------------------------------------------------------------
//   Return a Function for some given external symbol
// ----------------------------------------------------------------------------
{
    va_list va;
    va_start (va, parmCount);
    std::vector<const Type *> parms;
    for (uint i = 0; i < parmCount; i++)
    {
        Type *ty = va_arg(va, Type *);
        parms.push_back(ty);
    }
    FunctionType *fnType = FunctionType::get(retType, parms, false);
    Function *result = Function::Create(fnType, Function::ExternalLinkage,
                                        name, module);
    sys::DynamicLibrary::AddSymbol(name, address);
    return result;
}


Value *Compiler::EnterGlobal(Name *name, Name **address)
// ----------------------------------------------------------------------------
//   Enter a global variable in the symbol table
// ----------------------------------------------------------------------------
{
    Constant *null = ConstantPointerNull::get(treePtrTy);
    bool isConstant = false;
    GlobalValue *result = new GlobalVariable (treePtrTy, isConstant,
                                              GlobalVariable::ExternalLinkage,
                                              null, name->value, module);
    runtime->addGlobalMapping(result, address);
    globals[name] = result;
    return result;
}


Value *Compiler::Known(Tree *tree)
// ----------------------------------------------------------------------------
//    Return the known global value of the tree if it exists
// ----------------------------------------------------------------------------
{
    Value *result = NULL;
    if (globals.count(tree) > 0)
        result = globals[tree];
    return result;
}



// ============================================================================
// 
//   CompiledUnit - A particular piece of code we generate for a tree
// 
// ============================================================================

CompiledUnit::CompiledUnit(Compiler *comp, Tree *src, tree_list parms)
// ----------------------------------------------------------------------------
//   CompiledUnit constructor
// ----------------------------------------------------------------------------
    : compiler(comp), source(src),
      builder(NULL), function(NULL),
      allocabb(NULL), entrybb(NULL), exitbb(NULL),
      invokebb(NULL), failbb(NULL), successbb(NULL)
{
    // If a compilation for that tree is alread in progress, fwd decl
    if (compiler->functions[src])
        return;

    // Create the instruction builder for this compiled unit
    builder = new IRBuilder<> ();

    // Create the function signature, one entry per parameter + one for source
    std::vector<const Type *> signature;
    Type *treeTy = compiler->treePtrTy;
    for (ulong p = 0; p <= parms.size(); p++)
        signature.push_back(treeTy);
    FunctionType *fnTy = FunctionType::get(treeTy, signature, false);
    function = Function::Create(fnTy, Function::InternalLinkage,
                                "xl_eval", compiler->module);

    // Save it in the compiler
    compiler->functions[src] = function;

    // Create function entry point, where we will have all allocas
    allocabb = BasicBlock::Create("allocas", function);

    // Create entry block for the function
    entrybb = BasicBlock::Create("entry", function);

    // Associate the value for the input tree
    Function::arg_iterator args = function->arg_begin();
    Value *inputArg = args++;
    builder->SetInsertPoint(allocabb);
    Value *result_storage = builder->CreateAlloca(treeTy, 0, "result");
    builder->CreateStore(inputArg, result_storage);
    alloca[src] = result_storage;

    // Associate the value for the additional arguments (read-only, no alloca)
    tree_list::iterator parm;
    ulong parmsCount = 0;
    for (parm = parms.begin(); parm != parms.end(); parm++)
    {
        inputArg = args++;
        value[*parm] = inputArg;
        parmsCount++;
    }

    // Create the exit basic block and return statement
    exitbb = BasicBlock::Create("exit", function);
    builder->SetInsertPoint(exitbb);
    Value *retVal = builder->CreateLoad(result_storage, "retval");
    builder->CreateRet(retVal);

    // Begin by inserting code at the top
    builder->SetInsertPoint(entrybb);

    // Record current entry/exit points for the current expression
    invokebb = entrybb;
    successbb = exitbb;
    failbb = NULL;
}


CompiledUnit::~CompiledUnit()
// ----------------------------------------------------------------------------
//   Delete what we must...
// ----------------------------------------------------------------------------
{
    delete builder;
}    


Value *CompiledUnit::Known(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if the tree has a known local or global value
// ----------------------------------------------------------------------------
{
    Value *result = NULL;
    if (value.count(tree) > 0)
    {
        // Immediate value of some sort, use that
        result = value[tree];
    }
    else if (alloca.count(tree) > 0)
    {
        // Value is a variable
        result = builder->CreateLoad(alloca[tree], "known.alloca");
    }
    else
    {
        // Check if this is a global
        result = compiler->Known(tree);
        if (result)
            result = builder->CreateLoad(result, "known.global");
    }
    return result;
}


Value *CompiledUnit::ConstantInteger(Integer *what)
// ----------------------------------------------------------------------------
//    Generate a call to xl_new_integer to build an Integer tree
// ----------------------------------------------------------------------------
{
    Value *result = Known(what);
    if (!result)
    {
        Value *imm = ConstantInt::get(LLVM_INTTYPE(longlong), what->value);
        result = builder->CreateCall(compiler->xl_new_integer, imm);
        value[what] = result;
    }
    return result;
}


Value *CompiledUnit::ConstantReal(Real *what)
// ----------------------------------------------------------------------------
//    Generate a call to xl_new_real to build a Real tree
// ----------------------------------------------------------------------------
{
    Value *result = Known(what);
    if (!result)
    {
        Value *imm = ConstantFP::get(Type::DoubleTy, what->value);
        result = builder->CreateCall(compiler->xl_new_real, imm);
        value[what] = result;
    }
    return result;
}


Value *CompiledUnit::ConstantText(Text *what)
// ----------------------------------------------------------------------------
//    Generate a text with the same properaties as the input
// ----------------------------------------------------------------------------
{
    Value *result = Known(what);
    if (!result)
    {
        Constant *txtArray = ConstantArray::get(what->value);
        Value *txtPtr = builder->CreateConstGEP1_32(txtArray, 0);

        // Check if this is a normal text, "Foo"
        if (what->opening == Text::textQuote &&
            what->closing == Text::textQuote)
        {
            result = builder->CreateCall(compiler->xl_new_text, txtPtr);
        }
        // Check if this is is a normal character description, 'A'
        else if (what->opening == Text::charQuote &&
                 what->closing == Text::charQuote)
        {
            result = builder->CreateCall(compiler->xl_new_character, txtPtr);
        }
        else
        {
            Constant *openArray = ConstantArray::get(what->opening);
            Constant *closeArray = ConstantArray::get(what->opening);
            Value *openPtr = builder->CreateConstGEP1_32(openArray, 0);
            Value *closePtr = builder->CreateConstGEP1_32(closeArray, 0);
            result = builder->CreateCall3(compiler->xl_new_xtext,
                                          txtPtr, openPtr, closePtr);
        }
        value[what] = result;
    }
    return result;
}


llvm::Value *CompiledUnit::Invoke(Tree *subexpr, Tree *callee, tree_list args)
// ----------------------------------------------------------------------------
//    Generate a call with the given arguments
// ----------------------------------------------------------------------------
{
    // Check if the resulting form is a name or literal
    if (args.size() == 0)
        if (Value *known = Known(callee))
            return known;

    Function *toCall = compiler->functions[callee];
    assert(toCall);

    std::vector<Value *> argValues;
    tree_list::iterator a;
    for (a = args.begin(); a != args.end(); a++)
    {
        Tree *arg = *a;
        Value *value = Known(arg);
        assert(value);
        argValues.push_back(value);
    }

    Value *callVal = builder->CreateCall(toCall,
                                         argValues.begin(), argValues.end());

    // Store this as the result
    if (alloca.count(subexpr) > 0)
        builder->CreateStore(callVal, alloca[subexpr]);
    else
        value[subexpr] = callVal;

    return callVal;
}


eval_fn CompiledUnit::Finalize()
// ----------------------------------------------------------------------------
//   Finalize the build of the current function
// ----------------------------------------------------------------------------
{
    // Branch to the exit block from the last test we did
    builder->CreateBr(exitbb);

    // Connect the "allocas" to the actual entry point
    builder->SetInsertPoint(allocabb);
    builder->CreateBr(entrybb);

    // Verify the function we built
    verifyFunction(*function);
    if (compiler->optimizer)
        compiler->optimizer->run(*function);

    IFTRACE(code)
        function->print(std::cerr);

    void *result = compiler->runtime->getPointerToFunction(function);
    return (eval_fn) result;
}


BasicBlock *CompiledUnit::NeedTest()
// ----------------------------------------------------------------------------
//    Indicates that we need an exit basic block to jump to
// ----------------------------------------------------------------------------
{
    if (!failbb)
        failbb = BasicBlock::Create("fail", function);
    return failbb;
}


Value * CompiledUnit::NeedLazy(Tree *tree)
// ----------------------------------------------------------------------------
//    Indicates that we do lazy evaluation on given expression id
// ----------------------------------------------------------------------------
{
    // Check if we already have the cache variable
    Value *value = lazy[tree];
    if (value)
        return value;

    // Create some local "stack" variable
    std::ostringstream out;
    BasicBlock *current = builder->GetInsertBlock();
    builder->SetInsertPoint(allocabb); // Only valid location for alloca
    value = builder->CreateAlloca(compiler->treePtrTy, 0, "lazy");
    builder->SetInsertPoint(current);
    lazy[tree] = value;

    return value;
}


Value *CompiledUnit::Left(Tree *code)
// ----------------------------------------------------------------------------
//    Return the value for the left of the current tree
// ----------------------------------------------------------------------------
{
    // Check that the tree has the expected kind
    kind k = code->Kind();
    assert (k >= BLOCK);

    // Check if we already know the result, if so just return it
    Prefix *prefix = (Prefix *) code;
    Value *result = Known(prefix->left);
    if (result)
        return result;

    // Check that we already have a value for the given code
    Value *parent = Known(code);
    if (parent)
    {
        // WARNING: This relies on the layout of all nodes beginning the same
        Value *pptr = builder->CreateBitCast(parent, compiler->prefixTreePtrTy,
                                             "asprefix.left");
        result = builder->CreateConstGEP2_32(pptr, 0,
                                             LEFT_VALUE_INDEX, "leftptr");
        result = builder->CreateLoad(result, "left");
        assert(!value[prefix->left]);
        value[prefix->left] = result;
    }
    else
    {
        Context::context->Error("Internal: Using left of uncompiled '$1'",
                                code);
    }

    return result;
}


Value *CompiledUnit::Right(Tree *code)
// ----------------------------------------------------------------------------
//    Return the value for the right of the current tree
// ----------------------------------------------------------------------------
{
    // Check that the tree has the expected kind
    kind k = code->Kind();
    assert(k > BLOCK);

    // Check if we already known the result, if so just return it
    Prefix *prefix = (Prefix *) code;
    Value *result = Known(prefix->right);
    if (result)
        return result;

    // Check that we already have a value for the given code
    Value *parent = Known(code);
    if (parent)
    {
        // WARNING: This relies on the layout of all nodes beginning the same
        Value *pptr = builder->CreateBitCast(parent, compiler->prefixTreePtrTy,
                                             "asprefix.right");
        result = builder->CreateConstGEP2_32(pptr, 0,
                                             RIGHT_VALUE_INDEX, "rightptr");
        result = builder->CreateLoad(result, "right");
        assert(!value[prefix->right]);
        value[prefix->right] = result;
    }
    else
    {
        Context::context->Error("Internal: Using right of uncompiled '$1'",
                                code);
    }
    return result;
}


Value *CompiledUnit::LazyEvaluation(Tree *code)
// ----------------------------------------------------------------------------
//   Evaluate the given tree on demand
// ----------------------------------------------------------------------------
{
    // Check if we have saved the tree value
    Value *cache = NeedLazy(code);

    // Create the basic blocks 
    BasicBlock *doEval = BasicBlock::Create("doEval", function);
    BasicBlock *doneEval = BasicBlock::Create("doneEval", function);

    // Load the cached value, and test if it null
    Value *cached = builder->CreateLoad(cache, "cached");
    Value *testIfNull = builder->CreateIsNull(cached, "nulltest");
    builder->CreateCondBr(testIfNull, doEval, doneEval);

    // If we need to evaluate, call the xl_evaluate function and store result
    builder->SetInsertPoint(doEval);
    Value *treeValue = Known(code);
    if (!treeValue)
    {
        Context::context->Error("No value for '$1'", code);
        return NULL;
    }
    Value *evaluated = builder->CreateCall(compiler->xl_evaluate, treeValue,
                                           "evalcall");
    builder->CreateStore(evaluated, cache);
    builder->CreateBr(doneEval);

    // Load the cached value in the 'doneEval' basic block
    builder->SetInsertPoint(doneEval);
    cached = builder->CreateLoad(cache, "cached.done");
    invokebb = doneEval;

    return cached;
}


Value *CompiledUnit::EagerEvaluation(Tree *tree)
// ----------------------------------------------------------------------------
//   Evaluate the given tree immediately
// ----------------------------------------------------------------------------
{
    Value *treeValue = Known(tree);
    assert(treeValue);
    Value *evaluated = builder->CreateCall(compiler->xl_evaluate, treeValue);
    if (alloca.count(tree) > 0)
        builder->CreateStore(evaluated, alloca[tree]);
    return evaluated;
}


BasicBlock *CompiledUnit::TagTest(Tree *code, ulong tagValue)
// ----------------------------------------------------------------------------
//   Test if the input tree is an integer tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();

    // Check if the tag is INTEGER
    Value *treeValue = Known(code);
    if (!treeValue)
    {
        Context::context->Error("No value for '$1'", code);
        return NULL;
    }
    Value *tagPtr = builder->CreateConstGEP2_32(treeValue, 0, 0, "tagPtr");
    Value *tag = builder->CreateLoad(tagPtr, "tag");
    Value *mask = ConstantInt::get(tag->getType(), Tree::KINDMASK);
    Value *kind = builder->CreateAnd(tag, mask, "tagAndMask");
    Constant *refTag = ConstantInt::get(tag->getType(), tagValue);
    Value *isRightTag = builder->CreateICmpEQ(kind, refTag, "isRightTag");
    BasicBlock *isRightKindBB = BasicBlock::Create("isRightKind", function);
    builder->CreateCondBr(isRightTag, isRightKindBB, notGood);

    builder->SetInsertPoint(isRightKindBB);
    return isRightKindBB;
}


BasicBlock *CompiledUnit::IntegerTest(Tree *code, longlong value)
// ----------------------------------------------------------------------------
//   Test if the input tree is an integer tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();

    // Check if the tag is INTEGER
    BasicBlock *isIntegerBB = TagTest(code, INTEGER);
    if (!isIntegerBB)
        return isIntegerBB;

    // Check if the value is the same
    Value *treeValue = Known(code);
    assert(treeValue);
    treeValue = builder->CreateBitCast(treeValue, compiler->integerTreePtrTy);
    Value *valueFieldPtr = builder->CreateConstGEP2_32(treeValue, 0,
                                                       INTEGER_VALUE_INDEX);
    Value *tval = builder->CreateLoad(valueFieldPtr, "treeValue");
    Constant *rval = ConstantInt::get(tval->getType(), value, "refValue");
    Value *isGood = builder->CreateICmpEQ(tval, rval, "isGood");
    BasicBlock *isGoodBB = BasicBlock::Create("isGood", function);
    builder->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    builder->SetInsertPoint(isGoodBB);
    return isGoodBB;
}


BasicBlock *CompiledUnit::RealTest(Tree *code, double value)
// ----------------------------------------------------------------------------
//   Test if the input tree is a real tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();

    // Check if the tag is REAL
    BasicBlock *isRealBB = TagTest(code, REAL);
    if (!isRealBB)
        return isRealBB;

    // Check if the value is the same
    Value *treeValue = Known(code);
    assert(treeValue);
    treeValue = builder->CreateBitCast(treeValue, compiler->realTreePtrTy);
    Value *valueFieldPtr = builder->CreateConstGEP2_32(treeValue, 0,
                                                       REAL_VALUE_INDEX);
    Value *tval = builder->CreateLoad(valueFieldPtr, "treeValue");
    Constant *rval = ConstantFP::get(tval->getType(), value);
    Value *isGood = builder->CreateFCmpOEQ(tval, rval, "isGood");
    BasicBlock *isGoodBB = BasicBlock::Create("isGood", function);
    builder->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    builder->SetInsertPoint(isGoodBB);
    return isGoodBB;
}


BasicBlock *CompiledUnit::TextTest(Tree *code, text value)
// ----------------------------------------------------------------------------
//   Test if the input tree is a text tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();

    // Check if the tag is TEXT
    BasicBlock *isTextBB = TagTest(code, TEXT);
    if (!isTextBB)
        return isTextBB;

    // Check if the value is the same, call xl_same_text
    Value *treeValue = Known(code);
    assert(treeValue);
    Constant *refVal = ConstantArray::get(value);
    Value *refPtr = builder->CreateConstGEP1_32(refVal, 0);
    Value *isGood = builder->CreateCall2(compiler->xl_same_text,
                                         treeValue, refPtr);
    BasicBlock *isGoodBB = BasicBlock::Create("isGood", function);
    builder->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    builder->SetInsertPoint(isGoodBB);
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
    Value *isGood = builder->CreateCall2(compiler->xl_same_shape,
                                         leftVal, rightVal);
    BasicBlock *isGoodBB = BasicBlock::Create("isGood", function);
    builder->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    builder->SetInsertPoint(isGoodBB);
    return isGoodBB;
}


BasicBlock *CompiledUnit::TypeTest(Tree *value, Tree *type)
// ----------------------------------------------------------------------------
//   Test if the given value has the given type
// ----------------------------------------------------------------------------
{
    Value *valueVal = Known(value);
    Value *typeVal = Known(type);
    assert(valueVal);
    assert(typeVal);

    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();
    Value *isGood = builder->CreateCall2(compiler->xl_type_check,
                                         valueVal, typeVal);
    BasicBlock *isGoodBB = BasicBlock::Create("isGood", function);
    builder->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    builder->SetInsertPoint(isGoodBB);
    return isGoodBB;
}



// ============================================================================
// 
//    Expression reduction
// 
// ============================================================================
//   An expression reduction typically compiles as:
//     if (cond1) if (cond2) if (cond3) invoke(T)
//   However, we may determine during compilation of if(cond2) that the
//   call is statically not valid. So we save the initial basic block,
//   and decide at the end to connect it or not. Let LLVM optimize branches
//   and dead code away...
//
//   Invariants:
//   - u.invokebb is where we generate the code
//   - u.successbb is where we jump if we find a valid candidate
//   - u.failbb is where we jump if the current form doesn't match


ExpressionReduction::ExpressionReduction(CompiledUnit &u, Tree *src)
// ----------------------------------------------------------------------------
//    Snapshot current basic blocks in the compiled unit
// ----------------------------------------------------------------------------
    : unit(u), source(src),
      invokebb(u.invokebb), failbb(u.failbb), successbb(u.successbb)
{
    BasicBlock *bb = u.builder->GetInsertBlock();

    // Create the end-of-expression point if we find a match
    u.successbb = BasicBlock::Create("success", u.function);
    u.failbb = NULL;

    // Try to build a somewhat descriptive label for the tree
    text label;
    switch (src->Kind())
    {
    case INTEGER:       label = "integer"; break;
    case REAL:          label = "real"; break;
    case TEXT:          label = "text"; break;
    case NAME:          label = "name." + src->AsName()->value; break;
    case BLOCK:         label = "block"; break;
    case PREFIX:        label = "prefix"; break;
    case POSTFIX:       label = "postfix"; break;
    case INFIX:         label = "infix"; break;
    default:            label = "unknown"; break;
    }

    // Create alloca to store the new form
    u.builder->SetInsertPoint(u.allocabb); // Only valid location for alloca
    const char *clabel = label.c_str();
    Value *storage = u.builder->CreateAlloca(u.compiler->treePtrTy, 0, clabel);

    // We should have an initial value for that tree from the source
    u.builder->SetInsertPoint(bb);
    Value *initval = u.Known(src);
    if (!initval)
        source = Context::context->Error("No initial value for '$1'", src);
    u.builder->CreateStore(initval, storage);
    u.alloca[src] = storage;
}


ExpressionReduction::~ExpressionReduction()
// ----------------------------------------------------------------------------
//   Destructor restores the prevous sucess and fail points
// ----------------------------------------------------------------------------
{
    CompiledUnit &u = unit;

    // If last expression was conditional, branch to 'success' from there
    // (in that case, we leave the input tree unchanged if all tests fail)
    if (u.failbb)
    {
        u.builder->SetInsertPoint(u.failbb);
        u.builder->CreateBr(u.successbb);
    }
    if (u.invokebb)
    {
        u.builder->SetInsertPoint(u.invokebb);
        u.builder->CreateBr(u.successbb);
    }

    // Make the curent success point the next invokation point,
    // restore previous fail and success points
    u.invokebb = u.successbb;
    u.failbb = failbb;
    u.successbb = successbb;

    // Resume at new invoke point
    u.builder->SetInsertPoint(u.invokebb);
}


void ExpressionReduction::NewForm ()
// ----------------------------------------------------------------------------
//    Indicate that we are testing a new form for evaluating the expression
// ----------------------------------------------------------------------------
{
    CompiledUnit &u = unit;
    // Save previous basic blocks in the compiled unit
    invokebb = u.invokebb;
    assert(invokebb || !"NewForm called after unconditional success");

    // Create entry / exit basic blocks for this expression
    u.invokebb = BasicBlock::Create("invoke", u.function);
    u.failbb = NULL;

    // Set the insertion point to the new invokation code
    u.builder->SetInsertPoint(u.invokebb);
}


void ExpressionReduction::Succeeded(void)
// ----------------------------------------------------------------------------
//   We successfully compiled a reduction for that expression
// ----------------------------------------------------------------------------
//   In that case, we connect the basic blocks to evaluate the expression
{
    CompiledUnit &u = unit;

    // Branch from current point (end of expression) to exit of evaluation
    u.builder->CreateBr(u.successbb);

    // Branch from saved invokation to this one
    u.builder->SetInsertPoint(invokebb);
    u.builder->CreateBr(u.invokebb);

    // If there were tests, we keep testing from that 'else' spot
    u.invokebb = u.failbb;
    u.failbb = NULL;
}


void ExpressionReduction::Failed()
// ----------------------------------------------------------------------------
//    We figured out statically that the current form doesn't apply
// ----------------------------------------------------------------------------
{
    CompiledUnit &u = unit;
    Value *drop = ConstantInt::get(LLVM_INTTYPE(int), 0);
    if (u.failbb)
    {
        u.builder->SetInsertPoint(u.failbb);
        u.builder->CreateAdd(drop, drop, "drop.failbb");
        u.builder->CreateUnreachable();
    }
    u.builder->SetInsertPoint(u.invokebb);
    u.builder->CreateSub(drop, drop, "drop.invokebb");
    u.builder->CreateUnreachable();

    u.failbb = NULL;
    u.invokebb = invokebb;
    u.builder->SetInsertPoint(invokebb);
}


XL_END


// ============================================================================
// 
//    Debug helpers
// 
// ============================================================================

void debugm(XL::value_map &m)
// ----------------------------------------------------------------------------
//   Dump a value map from the debugger
// ----------------------------------------------------------------------------
{
    XL::value_map::iterator i;
    for (i = m.begin(); i != m.end(); i++)
        std::cerr << "map[" << (*i).first << "]=" << *(*i).second << '\n';
}


void debugv(void *v)
// ----------------------------------------------------------------------------
//   Dump a value for the debugger
// ----------------------------------------------------------------------------
{
    llvm::Value *value = (llvm::Value *) v;
    std::cerr << *value << "\n";
}
