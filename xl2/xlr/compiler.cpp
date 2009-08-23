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
#include "runtime.h"

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
      evalTy(NULL), evalFnTy(NULL), symbolsPtrTy(NULL),
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

    // Create the Symbol pointer type
    PATypeHolder structSymbolsTy = OpaqueType::get();   // struct Symbols
    symbolsPtrTy = PointerType::get(structSymbolsTy, 0);// Symbols *

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
    treeElements.push_back(symbolsPtrTy);               // symbols
    treeElements.push_back(treePtrTy);			// type
    treeTy = StructType::get(treeElements);		// struct Tree {}
    cast<OpaqueType>(structTreeTy.get())->refineAbstractTypeTo(treeTy);
    treeTy = cast<StructType> (structTreeTy.get());

    // Create the Integer type
    std::vector<const Type *> integerElements = treeElements;
    integerElements.push_back(LLVM_INTTYPE(longlong));  // value
    integerTreeTy = StructType::get(integerElements);   // struct Integer{}
    integerTreePtrTy = PointerType::get(integerTreeTy,0); // Integer *
#define INTEGER_VALUE_INDEX     4

    // Create the Real type
    std::vector<const Type *> realElements = treeElements;
    realElements.push_back(Type::DoubleTy);             // value
    realTreeTy = StructType::get(realElements);         // struct Real{}
    realTreePtrTy = PointerType::get(realTreeTy, 0);    // Real *
#define REAL_VALUE_INDEX        4

    // Create the Prefix type (which we also use for Infix and Block)
    std::vector<const Type *> prefixElements = treeElements;
    prefixElements.push_back(treePtrTy);                // Tree *
    prefixElements.push_back(treePtrTy);                // Tree *
    prefixTreeTy = StructType::get(prefixElements);     // struct Prefix {}
    prefixTreePtrTy = PointerType::get(prefixTreeTy, 0);// Prefix *
#define LEFT_VALUE_INDEX        4
#define RIGHT_VALUE_INDEX       5

    // Record the type names
    module->addTypeName("tree", treeTy);
    module->addTypeName("treePtr", treePtrTy);
    module->addTypeName("treePtrPtr", treePtrPtrTy);
    module->addTypeName("integer", integerTreeTy);
    module->addTypeName("integerPtr", integerTreePtrTy);
    module->addTypeName("real", realTreeTy);
    module->addTypeName("realPtr", realTreePtrTy);
    module->addTypeName("eval", evalTy);
    module->addTypeName("evalFn", evalFnTy);
    module->addTypeName("prefix", prefixTreeTy);
    module->addTypeName("prefixPtr", prefixTreePtrTy);
    module->addTypeName("symbolsPtr", symbolsPtrTy);

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


Value *Compiler::EnterConstant(Tree *constant)
// ----------------------------------------------------------------------------
//   Enter a constant (i.e. an Integer, Real or Text) into global map
// ----------------------------------------------------------------------------
{
    bool isConstant = true;
    text name = "xlcst";
    switch(constant->Kind())
    {
    case INTEGER: name = "xlint";  break;
    case REAL:    name = "xlreal"; break;
    case TEXT:    name = "xltext"; break;
    default:                       break;
    }
    IFTRACE(treelabels)
        name += "[" + text(*constant) + "]";
    GlobalValue *result = new GlobalVariable (treePtrTy, isConstant,
                                              GlobalVariable::InternalLinkage,
                                              NULL, name, module);
    Tree **address = Context::context->AddGlobal(constant);
    runtime->addGlobalMapping(result, address);
    globals[constant] = result;
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
      code(NULL), data(NULL), function(NULL),
      allocabb(NULL), entrybb(NULL), exitbb(NULL), failbb(NULL)
{
    // If a compilation for that tree is alread in progress, fwd decl
    if (compiler->functions[src])
        return;

    // Create the function signature, one entry per parameter + one for source
    std::vector<const Type *> signature;
    Type *treeTy = compiler->treePtrTy;
    for (ulong p = 0; p <= parms.size(); p++)
        signature.push_back(treeTy);
    FunctionType *fnTy = FunctionType::get(treeTy, signature, false);
    text label = "xl_eval";
    IFTRACE(treelabels)
        label += "[" + text(*src) + "]";
    function = Function::Create(fnTy, Function::InternalLinkage,
                                label.c_str(), compiler->module);

    // Save it in the compiler
    compiler->functions[src] = function;

    // Create function entry point, where we will have all allocas
    allocabb = BasicBlock::Create("allocas", function);
    data = new IRBuilder<> (allocabb);

    // Create entry block for the function
    entrybb = BasicBlock::Create("entry", function);
    code = new IRBuilder<> (entrybb);

    // Associate the value for the input tree
    Function::arg_iterator args = function->arg_begin();
    Value *inputArg = args++;
    Value *result_storage = data->CreateAlloca(treeTy, 0, "result");
    data->CreateStore(inputArg, result_storage);
    storage[src] = result_storage;

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
    IRBuilder<> exitcode(exitbb);
    Value *retVal = exitcode.CreateLoad(result_storage, "retval");
    exitcode.CreateRet(retVal);

    // Record current entry/exit points for the current expression
    failbb = NULL;
}


CompiledUnit::~CompiledUnit()
// ----------------------------------------------------------------------------
//   Delete what we must...
// ----------------------------------------------------------------------------
{
    delete code;
    delete data;
}    


eval_fn CompiledUnit::Finalize()
// ----------------------------------------------------------------------------
//   Finalize the build of the current function
// ----------------------------------------------------------------------------
{
    // Branch to the exit block from the last test we did
    code->CreateBr(exitbb);

    // Connect the "allocas" to the actual entry point
    data->CreateBr(entrybb);

    // Verify the function we built
    verifyFunction(*function);
    if (compiler->optimizer)
        compiler->optimizer->run(*function);

    IFTRACE(code)
        function->print(std::cerr);

    void *result = compiler->runtime->getPointerToFunction(function);
    return (eval_fn) result;
}


Value *CompiledUnit::NeedStorage(Tree *tree)
// ----------------------------------------------------------------------------
//    Allocate storage for a given tree
// ----------------------------------------------------------------------------
{
    Value *result = storage[tree];
    if (!result)
    {
        // Create alloca to store the new form
        text label = "loc";
        IFTRACE(treelabels)
            label += "[" + text(*tree) + "]";
        const char *clabel = label.c_str();
        result = data->CreateAlloca(compiler->treePtrTy, 0, clabel);
        storage[tree] = result;
    }

    return result;
}


Value *CompiledUnit::Known(Tree *tree, uint which)
// ----------------------------------------------------------------------------
//   Check if the tree has a known local or global value
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
        result = compiler->Known(tree);
        if (result)
            result = code->CreateLoad(result, "glob");
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
        result = code->CreateLoad(result, "treek");
        if (storage.count(what))
            code->CreateStore(result, storage[what]);
    }
    return result;
}


Value *CompiledUnit::NeedLazy(Tree *subexpr)
// ----------------------------------------------------------------------------
//   Record that we need a 'computed' flag for lazy evaluation of the subexpr
// ----------------------------------------------------------------------------
{
    Value *result = computed[subexpr];
    if (!result)
    {
        text label = "computed";
        IFTRACE(treelabels)
            label += "[" + text(*subexpr) + "]";
        result = data->CreateAlloca(Type::Int1Ty, 0, label.c_str());
        Value *falseFlag = ConstantInt::get(Type::Int1Ty, 0);
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
    // Set the 'lazy' flag or lazy evaluation
    Value *result = NeedLazy(subexpr);
    Value *trueFlag = ConstantInt::get(Type::Int1Ty, 1);
    code->CreateStore(trueFlag, result);

    // Store the value we were given as the result
    if (val)
    {
        if (storage.count(subexpr) > 0)
            code->CreateStore(val, storage[subexpr]);
    }

    // Return the test flag
    return result;
}


BasicBlock *CompiledUnit::BeginLazy(Tree *subexpr)
// ----------------------------------------------------------------------------
//    Begin lazy evaluation of a block of code
// ----------------------------------------------------------------------------
{
    BasicBlock *skip = BasicBlock::Create("skip", function);
    BasicBlock *work = BasicBlock::Create("work", function);

    Value *lazyFlagPtr = NeedLazy(subexpr);
    Value *lazyFlag = code->CreateLoad(lazyFlagPtr, "lazy");
    code->CreateCondBr(lazyFlag, skip, work);

    code->SetInsertPoint(work);
    return skip;
}


void CompiledUnit::EndLazy(Tree *subexpr,
                           llvm::BasicBlock *skip)
// ----------------------------------------------------------------------------
//   Finish lazy evaluation of a block of code
// ----------------------------------------------------------------------------
{
    code->CreateBr(skip);
    code->SetInsertPoint(skip);
}


llvm::Value *CompiledUnit::Invoke(Tree *subexpr, Tree *callee, tree_list args)
// ----------------------------------------------------------------------------
//    Generate a call with the given arguments
// ----------------------------------------------------------------------------
{
    // Check if the resulting form is a name or literal
    kind k = callee->Kind();
    if (k == INTEGER || k == REAL || k == TEXT)
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

    Function *toCall = compiler->functions[callee]; assert(toCall);

    // Add the 'self' argument
    std::vector<Value *> argV;
    Value *defaultVal = ConstantTree(subexpr);
    argV.push_back(defaultVal);

    tree_list::iterator a;
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
        failbb = BasicBlock::Create("fail", function);
    return failbb;
}


Value *CompiledUnit::Left(Tree *tree)
// ----------------------------------------------------------------------------
//    Return the value for the left of the current tree
// ----------------------------------------------------------------------------
{
    // Check that the tree has the expected kind
    kind k = tree->Kind();
    assert (k >= BLOCK);

    // Check if we already know the result, if so just return it
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
        Context::context->Error("Internal: Using left of uncompiled '$1'",
                                tree);
    }

    return result;
}


Value *CompiledUnit::Right(Tree *tree)
// ----------------------------------------------------------------------------
//    Return the value for the right of the current tree
// ----------------------------------------------------------------------------
{
    // Check that the tree has the expected kind
    kind k = tree->Kind();
    assert(k > BLOCK);

    // Check if we already known the result, if so just return it
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
        Context::context->Error("Internal: Using right of uncompiled '$1'",
                                tree);
    }
    return result;
}


Value *CompiledUnit::Copy(Tree *source, Tree *dest)
// ----------------------------------------------------------------------------
//    Copy data from source to destination
// ----------------------------------------------------------------------------
{
    Value *result = Known(source); assert(result);
    Value *ptr = NeedStorage(dest); assert(ptr);
    code->CreateStore(result, ptr);

    Value *doneFlag = NeedLazy(dest);
    Value *trueFlag = ConstantInt::get(Type::Int1Ty, 1);
    code->CreateStore(trueFlag, doneFlag);

    return result;
}


Value *CompiledUnit::CallEvaluate(Tree *tree)
// ----------------------------------------------------------------------------
//   Call the evaluate function for the given tree
// ----------------------------------------------------------------------------
{
    Value *treeValue = Known(tree);
    assert(treeValue);
    Value *evaluated = code->CreateCall(compiler->xl_evaluate, treeValue);
    MarkComputed(tree, evaluated);
    return evaluated;
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
        Context::context->Error("No value for '$1'", tree);
        return NULL;
    }
    Value *tagPtr = code->CreateConstGEP2_32(treeValue, 0, 0, "tagPtr");
    Value *tag = code->CreateLoad(tagPtr, "tag");
    Value *mask = ConstantInt::get(tag->getType(), Tree::KINDMASK);
    Value *kind = code->CreateAnd(tag, mask, "tagAndMask");
    Constant *refTag = ConstantInt::get(tag->getType(), tagValue);
    Value *isRightTag = code->CreateICmpEQ(kind, refTag, "isRightTag");
    BasicBlock *isRightKindBB = BasicBlock::Create("isRightKind", function);
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
    BasicBlock *isGoodBB = BasicBlock::Create("isGood", function);
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
    BasicBlock *isGoodBB = BasicBlock::Create("isGood", function);
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
    Constant *refVal = ConstantArray::get(value);
    Value *refPtr = code->CreateConstGEP1_32(refVal, 0);
    Value *isGood = code->CreateCall2(compiler->xl_same_text,
                                      treeValue, refPtr);
    BasicBlock *isGoodBB = BasicBlock::Create("isGood", function);
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
    BasicBlock *isGoodBB = BasicBlock::Create("isGood", function);
    code->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code->SetInsertPoint(isGoodBB);
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
    Value *isGood = code->CreateCall2(compiler->xl_type_check,
                                      valueVal, typeVal);
    BasicBlock *isGoodBB = BasicBlock::Create("isGood", function);
    code->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code->SetInsertPoint(isGoodBB);
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

ExpressionReduction::ExpressionReduction(CompiledUnit &u, Tree *src)
// ----------------------------------------------------------------------------
//    Snapshot current basic blocks in the compiled unit
// ----------------------------------------------------------------------------
    : unit(u), source(src),
      storage(NULL), computed(NULL),
      savedfailbb(NULL),
      entrybb(NULL), savedbb(NULL), successbb(NULL),
      savedvalue(u.value)
{
    // We need storage and a compute flag to skip this computation if needed
    storage = u.NeedStorage(src);
    computed = u.NeedLazy(src);

    // Save compile unit's data
    savedfailbb = u.failbb;
    u.failbb = NULL;

    // Create the end-of-expression point if we find a match or had it already
    successbb = u.BeginLazy(src);
}


ExpressionReduction::~ExpressionReduction()
// ----------------------------------------------------------------------------
//   Destructor restores the prevous sucess and fail points
// ----------------------------------------------------------------------------
{
    CompiledUnit &u = unit;

    // Mark the end of a lazy expression evaluation
    u.EndLazy(source, successbb);

    // Restore saved 'failbb' and value map
    u.failbb = savedfailbb;
    u.value = savedvalue;
}


void ExpressionReduction::NewForm ()
// ----------------------------------------------------------------------------
//    Indicate that we are testing a new form for evaluating the expression
// ----------------------------------------------------------------------------
{
    CompiledUnit &u = unit;

    // Save previous basic blocks in the compiled unit
    savedbb = u.code->GetInsertBlock();
    assert(savedbb || !"NewForm called after unconditional success");

    // Create entry / exit basic blocks for this expression
    entrybb = BasicBlock::Create("subexpr", u.function);
    u.failbb = NULL;

    // Set the insertion point to the new invokation code
    u.code->SetInsertPoint(entrybb);

}


void ExpressionReduction::Succeeded(void)
// ----------------------------------------------------------------------------
//   We successfully compiled a reduction for that expression
// ----------------------------------------------------------------------------
//   In that case, we connect the basic blocks to evaluate the expression
{
    CompiledUnit &u = unit;

    // Branch from current point (end of expression) to exit of evaluation
    u.code->CreateBr(successbb);

    // Branch from initial basic block position to this subcase
    u.code->SetInsertPoint(savedbb);
    u.code->CreateBr(entrybb);

    // If there were tests, we keep testing from that 'else' spot
    if (u.failbb)
    {
        u.code->SetInsertPoint(u.failbb);
    }
    else
    {
        // Create a fake basic block in case someone decides to add code
        BasicBlock *empty = BasicBlock::Create("empty", u.function);
        u.code->SetInsertPoint(empty);
    }
    u.failbb = NULL;
}


void ExpressionReduction::Failed()
// ----------------------------------------------------------------------------
//    We figured out statically that the current form doesn't apply
// ----------------------------------------------------------------------------
{
    CompiledUnit &u = unit;

    u.code->CreateUnreachable();
    if (u.failbb)
    {
        IRBuilder<> failTail(u.failbb);
        failTail.CreateUnreachable();
        u.failbb = NULL;
    }

    u.code->SetInsertPoint(savedbb);
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
