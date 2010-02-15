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
#include "llvm/LLVMContext.h"
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
#include <llvm/Support/StandardPasses.h>
#include <llvm/System/DynamicLibrary.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Support/raw_ostream.h>

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


Compiler::Compiler(kstring moduleName, uint optimize_level)
// ----------------------------------------------------------------------------
//   Initialize the various instances we may need
// ----------------------------------------------------------------------------
    : module(NULL), provider(NULL), runtime(NULL), optimizer(NULL),
      treeTy(NULL), treePtrTy(NULL), treePtrPtrTy(NULL),
      integerTreeTy(NULL), integerTreePtrTy(NULL),
      realTreeTy(NULL), realTreePtrTy(NULL),
      prefixTreeTy(NULL), prefixTreePtrTy(NULL),
      evalTy(NULL), evalFnTy(NULL), infoPtrTy(NULL), charPtrTy(NULL),
      xl_evaluate(NULL), xl_same_text(NULL), xl_same_shape(NULL),
      xl_type_check(NULL), xl_type_error(NULL),
      xl_new_integer(NULL), xl_new_real(NULL), xl_new_character(NULL),
      xl_new_text(NULL), xl_new_xtext(NULL), xl_new_block(NULL),
      xl_new_prefix(NULL), xl_new_postfix(NULL), xl_new_infix(NULL),
      functions()
{
    // Thanks to Dr. Albert Graef (pure programming language) for inspiration

    // Initialize native target (new features)
    InitializeNativeTarget();

    // LLVM Context (new feature)
    context = new llvm::LLVMContext();

    // Create module where we will build the code
    module = new Module(moduleName, *context);
    provider = new ExistingModuleProvider(module);

    // Select "fast JIT" if optimize level is 0, optimizing JIT otherwise
    runtime = EngineBuilder(module).create();
    runtime->DisableLazyCompilation(false);

    // Setup the optimizer - REVISIT: Adjust with optimization level
    uint optLevel = 2;
    optimizer = new FunctionPassManager(provider);
    createStandardFunctionPasses(optimizer, optLevel);
    {
        // Register target data structure layout info
        optimizer->add(new TargetData(*runtime->getTargetData()));

        // Promote allocas to registers.
        optimizer->add(createPromoteMemoryToRegisterPass());

        // Do simple "peephole" optimizations and bit-twiddling optimizations.
        optimizer->add(createInstructionCombiningPass());

        // Inlining of tails
        // optimizer->add(createTailDuplicationPass());
        optimizer->add(createTailCallEliminationPass());

        // Re-order blocks to eliminate branches
        optimizer->add(createBlockPlacementPass());

        // Collapse duplicate variables into canonical form
        // optimizer->add(createPredicateSimplifierPass());

        // Reassociate expression for better constant propagation
        optimizer->add(createReassociatePass());

        // Eliminate common subexpressions.
        optimizer->add(createGVNPass());

        // Simplify the control flow graph (deleting unreachable blocks, etc).
        optimizer->add(createCFGSimplificationPass());

        // Place phi nodes at loop boundaries to simplify other loop passes
        optimizer->add(createLCSSAPass());

        // Loop invariant code motion and memory promotion
        optimizer->add(createLICMPass());

        // Transform a[n] into *ptr++
        optimizer->add(createLoopStrengthReducePass());

        // Unroll loops (can it help in our case?)
        optimizer->add(createLoopUnrollPass());
    }

    // Other target options
    // DwarfExceptionHandling = true;   // Present in 2.6, but crashes
    // JITEmitDebugInfo = true;         // Not present in 2.6
    UnwindTablesMandatory = true;
    PerformTailCallOpt = true;
    // NoFramePointerElim = true;

    // Install a fallback mechanism to resolve references to the runtime, on
    // systems which do not allow the program to dlopen itself.
    runtime->InstallLazyFunctionCreator(unresolved_external);

    // Create the Symbol pointer type
    PATypeHolder structInfoTy = OpaqueType::get(*context); // struct Info
    infoPtrTy = PointerType::get(structInfoTy, 0);         // Info *

    // Create the eval_fn type
    PATypeHolder structTreeTy = OpaqueType::get(*context); // struct Tree
    treePtrTy = PointerType::get(structTreeTy, 0);      // Tree *
    treePtrPtrTy = PointerType::get(treePtrTy, 0);      // Tree **
    std::vector<const Type *> evalParms;
    evalParms.push_back(treePtrTy);
    evalTy = FunctionType::get(treePtrTy, evalParms, false);
    evalFnTy = PointerType::get(evalTy, 0);

    // Verify that there wasn't a change in the Tree type invalidating us
    struct LocalTree
    {
        LocalTree (const Tree &o) :
            tag(o.tag), code(o.code), info(o.info) {}
        ulong    tag;
        eval_fn  code;
        Info    *info;
    };
    // If this assert fails, you changed struct tree and need to modify here
    XL_CASSERT(sizeof(LocalTree) == sizeof(Tree));
               
    // Create the Tree type
    std::vector<const Type *> treeElements;
    treeElements.push_back(LLVM_INTTYPE(ulong));           // tag
    treeElements.push_back(evalFnTy);                      // code
    treeElements.push_back(infoPtrTy);                     // symbols
    treeTy = StructType::get(*context, treeElements);      // struct Tree {}
    cast<OpaqueType>(structTreeTy.get())->refineAbstractTypeTo(treeTy);
    treeTy = cast<StructType> (structTreeTy.get());
#define TAG_INDEX       0
#define CODE_INDEX      1
#define INFO_INDEX      2


    // Create the Integer type
    std::vector<const Type *> integerElements = treeElements;
    integerElements.push_back(LLVM_INTTYPE(longlong));  // value
    integerTreeTy = StructType::get(*context, integerElements);   // struct Integer{}
    integerTreePtrTy = PointerType::get(integerTreeTy,0); // Integer *
#define INTEGER_VALUE_INDEX     3

    // Create the Real type
    std::vector<const Type *> realElements = treeElements;
    realElements.push_back(Type::getDoubleTy(*context));  // value
    realTreeTy = StructType::get(*context, realElements); // struct Real{}
    realTreePtrTy = PointerType::get(realTreeTy, 0);      // Real *
#define REAL_VALUE_INDEX        3

    // Create the Prefix type (which we also use for Infix and Block)
    std::vector<const Type *> prefixElements = treeElements;
    prefixElements.push_back(treePtrTy);                // Tree *
    prefixElements.push_back(treePtrTy);                // Tree *
    prefixTreeTy = StructType::get(*context, prefixElements); // struct Prefix
    prefixTreePtrTy = PointerType::get(prefixTreeTy, 0);// Prefix *
#define LEFT_VALUE_INDEX        3
#define RIGHT_VALUE_INDEX       4

    // Record the type names
    module->addTypeName("tree", treeTy);
    module->addTypeName("integer", integerTreeTy);
    module->addTypeName("real", realTreeTy);
    module->addTypeName("eval", evalTy);
    module->addTypeName("prefix", prefixTreeTy);
    module->addTypeName("info*", infoPtrTy);

    // Create a reference to the evaluation function
    charPtrTy = PointerType::get(LLVM_INTTYPE(char), 0);
    const Type *boolTy = Type::getInt1Ty(*context);
#define FN(x) #x, (void *) XL::x
    xl_evaluate = ExternFunction(FN(xl_evaluate),
                                 treePtrTy, 1, treePtrTy);
    xl_same_text = ExternFunction(FN(xl_same_text),
                                  boolTy, 2, treePtrTy, charPtrTy);
    xl_same_shape = ExternFunction(FN(xl_same_shape),
                                   boolTy, 2, treePtrTy, treePtrTy);
    xl_type_check = ExternFunction(FN(xl_type_check),
                                   boolTy, 2, treePtrTy, treePtrTy);
    xl_type_error = ExternFunction(FN(xl_type_error),
                                   treePtrTy, 1, treePtrTy);
    xl_new_integer = ExternFunction(FN(xl_new_integer),
                                    treePtrTy, 1, LLVM_INTTYPE(longlong));
    xl_new_real = ExternFunction(FN(xl_new_real),
                                 treePtrTy, 1, Type::getDoubleTy(*context));
    xl_new_character = ExternFunction(FN(xl_new_character),
                                      treePtrTy, 1, charPtrTy);
    xl_new_text = ExternFunction(FN(xl_new_text),
                                 treePtrTy, 1, charPtrTy);
    xl_new_xtext = ExternFunction(FN(xl_new_xtext),
                                 treePtrTy, 3, charPtrTy, charPtrTy, charPtrTy);
    xl_new_closure = ExternFunction(FN(xl_new_closure),
                                    treePtrTy, -2,
                                    treePtrTy, LLVM_INTTYPE(uint));
    xl_new_block = ExternFunction(FN(xl_new_block),
                                  treePtrTy, 2, treePtrTy,treePtrTy);
    xl_new_prefix = ExternFunction(FN(xl_new_prefix),
                                   treePtrTy, 3, treePtrTy,treePtrTy,treePtrTy);
    xl_new_postfix = ExternFunction(FN(xl_new_postfix),
                                    treePtrTy, 3,treePtrTy,treePtrTy,treePtrTy);
    xl_new_infix = ExternFunction(FN(xl_new_infix),
                                  treePtrTy, 3, treePtrTy,treePtrTy,treePtrTy);
}


Compiler::~Compiler()
// ----------------------------------------------------------------------------
//    Destructor deletes the various things we had created
// ----------------------------------------------------------------------------
{
    delete context;
}


void Compiler::Reset()
// ----------------------------------------------------------------------------
//    Clear the contents of a compiler
// ----------------------------------------------------------------------------
{
    functions.clear();
    globals.clear();
    closures.clear();
    closet.clear();
    deleted.clear();
}


Function *Compiler::EnterBuiltin(text name,
                                 Tree *from, Tree *to,
                                 tree_list parms,
                                 eval_fn code)
// ----------------------------------------------------------------------------
//   Declare a built-in function
// ----------------------------------------------------------------------------
//   The input is not technically an eval_fn, but has as many parameters as
//   there are variables in the form
{
    Function *result = builtins[name];
    if (result)
    {
        functions[to] = result;
    }
    else
    {
        // Create the LLVM function
        std::vector<const Type *> parmTypes;
        parmTypes.push_back(treePtrTy); // First arg is self
        for (tree_list::iterator p = parms.begin(); p != parms.end(); p++)
            parmTypes.push_back(treePtrTy);
        FunctionType *fnTy = FunctionType::get(treePtrTy, parmTypes, false);
        result = Function::Create(fnTy, Function::ExternalLinkage,
                                  name, module);

        // Record the runtime symbol address
        sys::DynamicLibrary::AddSymbol(name, (void*) code);

        // Associate the function with the tree form
        functions[to] = result;
        builtins[name] = result;
    }

    return result;    
}


adapter_fn Compiler::EnterArrayToArgsAdapter(uint numargs)
// ----------------------------------------------------------------------------
//   Generate code to call a function with N arguments
// ----------------------------------------------------------------------------
//   The generated code serves as an adapter between code that has
//   tree arguments in a C array and code that expects them as an arg-list.
//   For example, it allows you to call foo(Tree *src, Tree *a1, Tree *a2)
//   by calling generated_adapter(foo, Tree *src, Tree *args[2])
{
    // Check if we already computed it
    eval_fn result = array_to_args_adapters[numargs];
    if (result)
        return (adapter_fn) result;

    // Generate the function type: Tree *generated(eval_fn, Tree *, Tree **)
    std::vector<const Type *> parms;
    parms.push_back(evalFnTy);
    parms.push_back(treePtrTy);
    parms.push_back(treePtrPtrTy);
    FunctionType *fnType = FunctionType::get(treePtrTy, parms, false);
    Function *adapter = Function::Create(fnType, Function::InternalLinkage,
                                        "xl_adapter", module);

    // Generate the function type for the called function
    std::vector<const Type *> called;
    called.push_back(treePtrTy);
    for (uint a = 0; a < numargs; a++)
        called.push_back(treePtrTy);
    FunctionType *calledType = FunctionType::get(treePtrTy, called, false);
    PointerType *calledPtrType = PointerType::get(calledType, 0);

    // Create the entry for the function we generate
    BasicBlock *entry = BasicBlock::Create(*context, "adapt", adapter);
    IRBuilder<> code(entry);

    // Read the arguments from the function we are generating
    Function::arg_iterator inArgs = adapter->arg_begin();
    Value *fnToCall = inArgs++;
    Value *sourceTree = inArgs++;
    Value *treeArray = inArgs++;

    // Cast the input function pointer to right type
    Value *fnTyped = code.CreateBitCast(fnToCall, calledPtrType, "fnCast");

    // Add source as first argument to output arguments
    std::vector<Value *> outArgs;
    outArgs.push_back (sourceTree);

    // Read other arguments from the input array
    for (uint a = 0; a < numargs; a++)
    {
        Value *elementPtr = code.CreateConstGEP1_32(treeArray, a);
        Value *fromArray = code.CreateLoad(elementPtr, "arg");
        outArgs.push_back(fromArray);
    }

    // Call the function
    Value *retVal = code.CreateCall(fnTyped, outArgs.begin(), outArgs.end());

    // Return the result
    code.CreateRet(retVal);

    // Verify the function and optimize it.
    verifyFunction (*adapter);
    if (optimizer)
        optimizer->run(*adapter);

    // Enter the result in the map
    result = (eval_fn) runtime->getPointerToFunction(adapter);
    array_to_args_adapters[numargs] = result;

    // And return it to the caller
    return (adapter_fn) result;
}


Function *Compiler::ExternFunction(kstring name, void *address,
                                   const Type *retType, int parmCount, ...)
// ----------------------------------------------------------------------------
//   Return a Function for some given external symbol
// ----------------------------------------------------------------------------
{
    va_list va;
    std::vector<const Type *> parms;
    bool isVarArg = parmCount < 0;
    if (isVarArg)
        parmCount = -parmCount;

    va_start(va, parmCount);
    for (int i = 0; i < parmCount; i++)
    {
        Type *ty = va_arg(va, Type *);
        parms.push_back(ty);
    }
    va_end(va);
    FunctionType *fnType = FunctionType::get(retType, parms, isVarArg);
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
    GlobalValue *result = new GlobalVariable (*module, treePtrTy, isConstant,
                                              GlobalVariable::ExternalLinkage,
                                              null, name->value);
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
    IFTRACE(labels)
        name += "[" + text(*constant) + "]";
    GlobalValue *result = new GlobalVariable (*module, treePtrTy, isConstant,
                                              GlobalVariable::InternalLinkage,
                                              NULL, name);
    Tree **address = Context::context->AddGlobal(constant);
    runtime->addGlobalMapping(result, address);
    globals[constant] = result;
    return result;
}


bool Compiler::IsKnown(Tree *tree)
// ----------------------------------------------------------------------------
//    Test if global is known
// ----------------------------------------------------------------------------
{
    return globals.count(tree) > 0;
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


void Compiler::FreeResources(Tree *tree)
// ----------------------------------------------------------------------------
//   Free the LLVM resources associated to the tree, if any
// ----------------------------------------------------------------------------
//   In the first pass, we need to clear the body and machine code for all
//   functions. This is because if we have foo() calling bar() and bar()
//   calling foo(), we will get an LLVM assert deleting one while the
//   other's body still makes a reference.
{
    if (functions.count(tree) > 0 && closet.count(tree) == 0)
    {
        std::cerr << "Freed one tree\n";
        Function *f = functions[tree];
        f->deleteBody();
        runtime->freeMachineCodeForFunction(f);
        deleted.insert(tree);
    }
}


void Compiler::FreeResources()
// ----------------------------------------------------------------------------
//   Delete LLVM functions for all trees we want to erase
// ----------------------------------------------------------------------------
//   At this stage, we have deleted all the bodies
{
    while (!deleted.empty())
    {
        std::cerr << "Freed functions\n";
        deleted_set::iterator i = deleted.begin();
        Tree *tree = *i;
        Function *f = functions[tree];
        delete f;               // Now safe to do
        functions.erase(tree);
        deleted.erase(tree);
    }
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
    : compiler(comp), context(comp->context), source(src),
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
    IFTRACE(labels)
        label += "[" + text(*src) + "]";
    function = Function::Create(fnTy, Function::InternalLinkage,
                                label.c_str(), compiler->module);

    // Save it in the compiler
    compiler->functions[src] = function;

    // Create function entry point, where we will have all allocas
    allocabb = BasicBlock::Create(*context, "allocas", function);
    data = new IRBuilder<> (allocabb);

    // Create entry block for the function
    entrybb = BasicBlock::Create(*context, "entry", function);
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
    exitbb = BasicBlock::Create(*context, "exit", function);
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
    {
        llvm::raw_stderr_ostream llvmstderr;
        function->print(llvmstderr);
    }

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
        IFTRACE(labels)
            label += "[" + text(*tree) + "]";
        const char *clabel = label.c_str();
        result = data->CreateAlloca(compiler->treePtrTy, 0, clabel);
        storage[tree] = result;
        if (value.count(tree))
            data->CreateStore(value[tree], result);
        else if (Value *global = compiler->Known(tree))
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
        result = data->CreateLoad(result, "treek");
        if (storage.count(what))
            data->CreateStore(result, storage[what]);
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
    Value *trueFlag = ConstantInt::get(LLVM_BOOLTYPE, 1);
    code->CreateStore(trueFlag, result);

    // Return the test flag
    return result;
}


BasicBlock *CompiledUnit::BeginLazy(Tree *subexpr)
// ----------------------------------------------------------------------------
//    Begin lazy evaluation of a block of code
// ----------------------------------------------------------------------------
{
    BasicBlock *skip = BasicBlock::Create(*context, "skip", function);
    BasicBlock *work = BasicBlock::Create(*context, "work", function);

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
        failbb = BasicBlock::Create(*context, "fail", function);
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
        Ooops("Internal: Using left of uncompiled '$1'", tree);
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
        Ooops("Internal: Using right of uncompiled '$1'", tree);
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
        Value *doneFlag = NeedLazy(dest);
        Value *trueFlag = ConstantInt::get(LLVM_BOOLTYPE, 1);
        code->CreateStore(trueFlag, doneFlag);
    }

    return result;
}


Value *CompiledUnit::CallEvaluate(Tree *tree)
// ----------------------------------------------------------------------------
//   Call the evaluate function for the given tree
// ----------------------------------------------------------------------------
{
    Value *treeValue = Known(tree); assert(treeValue);
    if (noeval.count(tree))
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


Value *CompiledUnit::CreateClosure(Tree *callee, tree_list &args)
// ----------------------------------------------------------------------------
//   Create a closure for an expression we want to evaluate later
// ----------------------------------------------------------------------------
{
    std::vector<Value *> argV;
    Value *calleeVal = Known(callee); assert(calleeVal);
    Value *countVal = ConstantInt::get(LLVM_INTTYPE(uint), args.size());
    tree_list::iterator a;

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
{
    // Load left tree and get its code tag
    Type *treePtrTy = compiler->treePtrTy;
    Value *ptr = Known(callee); assert(ptr);
    Value *topPtr = ptr;
    Value *pfx = code->CreateBitCast(ptr,compiler->prefixTreePtrTy);
    Value *lf = code->CreateConstGEP2_32(pfx, 0, LEFT_VALUE_INDEX);
    Value *callTree = code->CreateLoad(lf);
    Value *callCode = code->CreateConstGEP2_32(callTree, 0, CODE_INDEX);
    callCode = code->CreateLoad(callCode);
    
    // Build argument list
    std::vector<Value *> argV;
    std::vector<const Type *> signature;
    argV.push_back(topPtr);     // Self argument
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


Value *CompiledUnit::CallTypeError(Tree *what)
// ----------------------------------------------------------------------------
//   Report a type error trying to evaluate some argument
// ----------------------------------------------------------------------------
{
    Value *ptr = ConstantTree(what); assert(what);
    Value *callVal = code->CreateCall(compiler->xl_type_error, ptr);
    MarkComputed(what, callVal);
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
        Ooops("No value for '$1'", tree);
        return NULL;
    }
    Value *tagPtr = code->CreateConstGEP2_32(treeValue, 0, 0, "tagPtr");
    Value *tag = code->CreateLoad(tagPtr, "tag");
    Value *mask = ConstantInt::get(tag->getType(), Tree::KINDMASK);
    Value *kind = code->CreateAnd(tag, mask, "tagAndMask");
    Constant *refTag = ConstantInt::get(tag->getType(), tagValue);
    Value *isRightTag = code->CreateICmpEQ(kind, refTag, "isRightTag");
    BasicBlock *isRightKindBB = BasicBlock::Create(*context,
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
    BasicBlock *isGoodBB = BasicBlock::Create(*context,
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
    BasicBlock *isGoodBB = BasicBlock::Create(*context,
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
    Constant *refVal = ConstantArray::get(*context, value);
    const Type *refValTy = refVal->getType();
    GlobalVariable *gvar = new GlobalVariable(*compiler->module, refValTy, true,
                                              GlobalValue::InternalLinkage,
                                              refVal, "str");
    Value *refPtr = code->CreateConstGEP2_32(gvar, 0, 0);
    Value *isGood = code->CreateCall2(compiler->xl_same_text,
                                      treeValue, refPtr);
    BasicBlock *isGoodBB = BasicBlock::Create(*context, "isGood", function);
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
    BasicBlock *isGoodBB = BasicBlock::Create(*context, "isGood", function);
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
    BasicBlock *isGoodBB = BasicBlock::Create(*context, "isGood", function);
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
    : unit(u), source(src), context(u.context),
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
    entrybb = BasicBlock::Create(*context, "subexpr", u.function);
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
        BasicBlock *empty = BasicBlock::Create(*context, "empty", u.function);
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

    u.CallTypeError(source);
    u.code->CreateUnreachable();
    if (u.failbb)
    {
        IRBuilder<> failTail(u.failbb);
        u.code->SetInsertPoint(u.failbb);
        u.CallTypeError(source);
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
    llvm::raw_stderr_ostream llvmstderr;
    for (i = m.begin(); i != m.end(); i++)
        llvmstderr << "map[" << (*i).first << "]=" << *(*i).second << '\n';
}


void debugv(void *v)
// ----------------------------------------------------------------------------
//   Dump a value for the debugger
// ----------------------------------------------------------------------------
{
    llvm::Value *value = (llvm::Value *) v;
    llvm::raw_stderr_ostream llvmstderr;
    llvmstderr << *value << "\n";
}
