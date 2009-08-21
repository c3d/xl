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
    module->addTypeName("struct.tree", treeTy);
    module->addTypeName("struct.treeptr", treePtrTy);
    module->addTypeName("struct.treeptrptr", treePtrPtrTy);
    module->addTypeName("struct.integer", integerTreeTy);
    module->addTypeName("struct.integerptr", integerTreePtrTy);
    module->addTypeName("struct.real", realTreeTy);
    module->addTypeName("struct.realptr", realTreePtrTy);
    module->addTypeName("struct.eval", evalTy);
    module->addTypeName("struct.evalfn", evalFnTy);

    // Create a reference to the evaluation function
    xl_evaluate = Function::Create(evalTy, Function::ExternalLinkage,
                                   "xl_evaluate", module);
    sys::DynamicLibrary::AddSymbol("xl_evaluate", (void *) XL::xl_evaluate);

    // Create a reference to the xl_same_text function
    std::vector<const Type *> sameTextParms;
    PointerType *i8PtrTy = PointerType::get(LLVM_INTTYPE(char), 0);
    sameTextParms.push_back(treePtrTy);
    sameTextParms.push_back(i8PtrTy);
    FunctionType *sameTextTy = FunctionType::get(Type::Int1Ty,
                                                 sameTextParms, false);
    xl_same_text = Function::Create(sameTextTy, Function::ExternalLinkage,
                                    "xl_same_text", module);
    sys::DynamicLibrary::AddSymbol("xl_same_text", (void*) XL::xl_same_text);

    // Create a reference to the xl_same_shape function
    std::vector<const Type *> sameShapeParms;
    sameTextParms.push_back(treePtrTy);
    sameTextParms.push_back(treePtrTy);
    FunctionType *sameShapeTy = FunctionType::get(Type::Int1Ty,
                                                  sameShapeParms, false);
    xl_same_shape = Function::Create(sameShapeTy, Function::ExternalLinkage,
                                     "xl_same_shape", module);
    sys::DynamicLibrary::AddSymbol("xl_same_shape", (void*) XL::xl_same_shape);

    // Create a reference to the xl_type_check function
    xl_type_check = Function::Create(sameShapeTy, Function::ExternalLinkage,
                                     "xl_type_check", module);
    sys::DynamicLibrary::AddSymbol("xl_type_check", (void*) XL::xl_type_check);
}


Function *Compiler::EnterBuiltin(text name, Tree *form, eval_fn code)
// ----------------------------------------------------------------------------
//   Declare a built-in function
// ----------------------------------------------------------------------------
//   The input is not technically an eval_fn, but has as many parameters as
//   there are variables in the form
{
    // Find the parameters
    tree_list parms;
    Context *context = Context::context;
    context->ParameterList (form, parms);

    // Create the LLVM function
    std::vector<const Type *> parmTypes;
    for (tree_list::iterator p = parms.begin(); p != parms.end(); p++)
        parmTypes.push_back(treePtrTy);
    FunctionType *fnTy = FunctionType::get(treePtrTy, parmTypes, false);
    Function *result = Function::Create(fnTy, Function::ExternalLinkage,
                                        name, module);

    // Record the runtime symbol address
    sys::DynamicLibrary::AddSymbol(name, (void*) code);

    // Associate the function with the tree form
    functions[form] = result;

    return result;    
}



// ============================================================================
// 
//   CompiledUnit - A particular piece of code we generate for a tree
// 
// ============================================================================

CompiledUnit::CompiledUnit(Compiler *comp, Tree *source, tree_list parms)
// ----------------------------------------------------------------------------
//   CompiledUnit constructor
// ----------------------------------------------------------------------------
    : compiler(comp), builder(NULL), function(NULL),
      entrybb(NULL), exitbb(NULL), invokebb(NULL), failbb(NULL)
{
    // If a compilation for that tree is alread in progress, fwd decl
    if (compiler->functions[source])
        return;

    // Create the instruction builder for this compiled unit
    builder = new IRBuilder<> ();

    // Create the function signature, one entry per parameter
    std::vector<const Type *> signature;
    Type *treeTy = compiler->treePtrTy;
    for (ulong p = 0; p < parms.size(); p++)
        signature.push_back(treeTy);
    FunctionType *fnTy = FunctionType::get(treeTy, signature, false);
    function = Function::Create(fnTy, Function::InternalLinkage,
                                "xleval", compiler->module);

    // Save it in the compiler
    compiler->functions[source] = function;

    // Create entry block for the function
    entrybb = BasicBlock::Create("entry", function);

    // This is where we will initially insert
    invokebb = entrybb;

    // Associate the value for the input tree
    Function::arg_iterator args = function->arg_begin();
    tree_list::iterator parm;
    for (parm = parms.begin(); parm != parms.end(); parm++)
    {
        Value* inputArg = args++;
        map[*parm] = inputArg;
    }

    // Create the pointer to the return value, initialize with input
    builder->SetInsertPoint(entrybb);
    Value *zero = ConstantPointerNull::get(compiler->treePtrTy);
    result = builder->CreateAlloca(compiler->treePtrTy, 0, "result");
    builder->CreateStore(zero, result);

    // Create the exit basic block and return statement
    exitbb = BasicBlock::Create("exit");
    builder->SetInsertPoint(exitbb);
    Value *retVal = builder->CreateLoad(result);
    builder->CreateRet(retVal);

    // Begin by inserting code at the top
    builder->SetInsertPoint(entrybb);
}


CompiledUnit::~CompiledUnit()
// ----------------------------------------------------------------------------
//   Delete what we must...
// ----------------------------------------------------------------------------
{
    delete builder;
}    


BasicBlock *CompiledUnit::BeginInvokation()
// ----------------------------------------------------------------------------
//   Begin a new invokation
// ----------------------------------------------------------------------------
//   A tree invokation typically has the form:
//     if (cond1) if (cond2) if (cond3) invoke(T)
//   However, we may determine during compilation of if(cond2) that the
//   call is statically not valid. So we save the initial basic block,
//   and decide at the end to connect it or not. Let LLVM optimize branches
//   and dead code away...
{
    BasicBlock *saved = invokebb;
    assert(!failbb); assert(invokebb);    
    invokebb = BasicBlock::Create("invoke");
    builder->SetInsertPoint(invokebb);
    return saved;
}


void CompiledUnit::EndInvokation(llvm::BasicBlock *bb, bool success)
// ----------------------------------------------------------------------------
//   Connect the basic block we saved... or not
// ----------------------------------------------------------------------------
{
    if (success)
    {
        // Branch current point to exit
        builder->CreateBr(exitbb);

        // Since we successfully generated a call, connect to it
        builder->SetInsertPoint(bb);
        builder->CreateBr(invokebb);

        // If there were tests and they failed, connect to them
        if (failbb)
        {
            invokebb = failbb;
            failbb = NULL;
        }
        else
        {
            // Call was unconditional, we should never use invokebb again
            invokebb = NULL;
        }
    }
    else
    {
        // Drop whatever tests we did so far, restore insert where it was
        invokebb = bb;
        failbb = NULL;
    }
}


llvm::Value *CompiledUnit::Invoke(Tree *callee, tree_list args)
// ----------------------------------------------------------------------------
//    Generate a call with the given arguments
// ----------------------------------------------------------------------------
{
    assert(callee->code);
    Function *toCall = compiler->functions[callee];
    assert(toCall);

    std::vector<Value *> argValues;
    tree_list::iterator a;
    for (a = args.begin(); a != args.end(); a++)
    {
        Tree *arg = *a;
        Value *value = map[arg];
        assert(value);
        argValues.push_back(value);
    }

    Value *callVal = builder->CreateCall(toCall,
                                         argValues.begin(), argValues.end());

    // Store this as the result
    map[callee] = callVal;
    builder->CreateStore(callVal, result);

    return callVal;
}


Value *CompiledUnit::Return(Tree *value)
// ----------------------------------------------------------------------------
//   Store the given value in the return storage
// ----------------------------------------------------------------------------
{
    Value *retVal = map[value];
    assert(retVal);
    builder->CreateStore(retVal, result);
    return retVal;
}


eval_fn CompiledUnit::Finalize()
// ----------------------------------------------------------------------------
//   Finalize the build of the current function
// ----------------------------------------------------------------------------
{
    // Branch to the exit block from the last test we did
    builder->CreateBr(exitbb);

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
        failbb = BasicBlock::Create("failed_test");
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
    builder->SetInsertPoint(entrybb); // Only valid location for alloca
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

    // Check that we already have a value for the given code
    Value *value = map[code];
    Value *result = NULL;
    if (value)
    {
        // WARNING: This relies on the layout of all nodes beginning the same
        Prefix *prefix = (Prefix *) code;
        result = map[prefix->left];
        if (result)
            return result;
        value = builder->CreateBitCast(value, compiler->prefixTreePtrTy);
        result = builder->CreateConstGEP2_32(value, 0,
                                             LEFT_VALUE_INDEX, "left");
        assert(!map[prefix->left]);
        map[prefix->left] = result;
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

    // Check that we already have a value for the given code
    Value *value = map[code];
    Value *result = NULL;
    if (value)
    {
        // WARNING: This relies on the layout of all nodes beginning the same
        Prefix *prefix = (Prefix *) code;
        result = map[prefix->right];
        if (result)
            return result;
        value = builder->CreateBitCast(value, compiler->prefixTreePtrTy);
        result = builder->CreateConstGEP2_32(value, 0,
                                             RIGHT_VALUE_INDEX, "right");
        assert(!map[prefix->right]);
        map[prefix->right] = result;
    }
    else
    {
        Context::context->Error("Internal: Using right of uncompiled '$1'",
                                code);
    }
    return result;
}


Value *CompiledUnit::LazyEvaluation(Tree *code, ulong id)
// ----------------------------------------------------------------------------
//   Evaluate the given tree on demand
// ----------------------------------------------------------------------------
{
    // Check if we have saved the tree value
    Value *cache = NeedLazy(code);

    // Create the basic blocks 
    BasicBlock *doEval = BasicBlock::Create("doEval");
    BasicBlock *doneEval = BasicBlock::Create("doneEval");

    // Load the cached value, and test if it null
    Value *cached = builder->CreateLoad(cache);
    Value *testIfNull = builder->CreateIsNull(cached);
    builder->CreateCondBr(testIfNull, doEval, doneEval);

    // If we need to evaluate, call the xl_evaluate function and store result
    builder->SetInsertPoint(doEval);
    Value *treeValue = map[code];
    if (!treeValue)
    {
        Context::context->Error("No value for '$1'", code);
        return NULL;
    }
    Value *evaluated = builder->CreateCall(compiler->xl_evaluate, treeValue);
    builder->CreateStore(evaluated, cache);
    builder->CreateBr(doneEval);

    // Load the cached value in the 'doneEval' basic block
    builder->SetInsertPoint(doneEval);
    cached = builder->CreateLoad(cache);

    return cached;
}


Value *CompiledUnit::EagerEvaluation(Tree *tree)
// ----------------------------------------------------------------------------
//   Evaluate the given tree immediately
// ----------------------------------------------------------------------------
{
    Value *treeValue = map[tree];
    assert(treeValue);
    Value *evaluated = builder->CreateCall(compiler->xl_evaluate, treeValue);
    builder->CreateStore(evaluated, result);
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
    Value *treeValue = map[code];
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
    BasicBlock *isRightKindBB = BasicBlock::Create("isRightKind");
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
    Value *treeValue = map[code];
    assert(treeValue);
    treeValue = builder->CreateBitCast(treeValue, compiler->integerTreePtrTy);
    Value *valueFieldPtr = builder->CreateConstGEP2_32(treeValue, 0,
                                                       INTEGER_VALUE_INDEX);
    Value *tval = builder->CreateLoad(valueFieldPtr, "treeValue");
    Constant *rval = ConstantInt::get(tval->getType(), value, "refValue");
    Value *isGood = builder->CreateICmpEQ(tval, rval, "isGood");
    BasicBlock *isGoodBB = BasicBlock::Create("isGood");
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
    Value *treeValue = map[code];
    assert(treeValue);
    treeValue = builder->CreateBitCast(treeValue, compiler->realTreePtrTy);
    Value *valueFieldPtr = builder->CreateConstGEP2_32(treeValue, 0,
                                                       REAL_VALUE_INDEX);
    Value *tval = builder->CreateLoad(valueFieldPtr, "treeValue");
    Constant *rval = ConstantFP::get(tval->getType(), value);
    Value *isGood = builder->CreateFCmpOEQ(tval, rval, "isGood");
    BasicBlock *isGoodBB = BasicBlock::Create("isGood");
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
    Value *treeValue = map[code];
    assert(treeValue);
    Constant *refVal = ConstantArray::get(value);
    Value *refPtr = builder->CreateConstGEP1_32(refVal, 0);
    Value *isGood = builder->CreateCall2(compiler->xl_same_text,
                                         treeValue, refPtr);
    BasicBlock *isGoodBB = BasicBlock::Create("isGood");
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
    Value *leftVal = map[left];
    Value *rightVal = map[right];
    assert(leftVal);
    assert(rightVal);
    if (leftVal == rightVal) // How unlikely?
        return NULL;

    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();
    Value *isGood = builder->CreateCall2(compiler->xl_same_shape,
                                         leftVal, rightVal);
    BasicBlock *isGoodBB = BasicBlock::Create("isGood");
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
    Value *valueVal = map[value];
    Value *typeVal = map[type];
    assert(valueVal);
    assert(typeVal);

    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();
    Value *isGood = builder->CreateCall2(compiler->xl_type_check,
                                         valueVal, typeVal);
    BasicBlock *isGoodBB = BasicBlock::Create("isGood");
    builder->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    builder->SetInsertPoint(isGoodBB);
    return isGoodBB;
}

XL_END

void debugm(XL::value_map &m)
{
    XL::value_map::iterator i;
    for (i = m.begin(); i != m.end(); i++)
        std::cerr << "map[" << (*i).first << "]=" << *(*i).second << '\n';
}
