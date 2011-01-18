// ****************************************************************************
//  compiler.cpp                                                    XLR project
// ****************************************************************************
// 
//   File Description:
// 
//    Just-in-time (JIT) compilation of XL trees
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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "compiler.h"
#include "compiler-gc.h"
#include "compiler-llvm.h"
#include "unit.h"
#include "args.h"
#include "options.h"
#include "context.h"
#include "renderer.h"
#include "runtime.h"
#include "errors.h"
#include "types.h"

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
//    Compiler - Global information about the LLVM compiler
// 
// ============================================================================
//
// The Compiler class is where we store all the global information that
// persists during the lifetime of the program: LLVM data structures,
// LLVM definitions for frequently used types, XL runtime functions, ...
// 

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
    : module(NULL), runtime(NULL), optimizer(NULL), moduleOptimizer(NULL),
      booleanTy(NULL),
      integerTy(NULL), integer8Ty(NULL), integer16Ty(NULL), integer32Ty(NULL),
      realTy(NULL), real32Ty(NULL),
      characterTy(NULL), charPtrTy(NULL), textTy(NULL),
      treeTy(NULL), treePtrTy(NULL), treePtrPtrTy(NULL),
      integerTreeTy(NULL), integerTreePtrTy(NULL),
      realTreeTy(NULL), realTreePtrTy(NULL),
      textTreeTy(NULL), textTreePtrTy(NULL),
      nameTreeTy(NULL), nameTreePtrTy(NULL),
      blockTreeTy(NULL), blockTreePtrTy(NULL),
      prefixTreeTy(NULL), prefixTreePtrTy(NULL),
      postfixTreeTy(NULL), postfixTreePtrTy(NULL),
      infixTreeTy(NULL), infixTreePtrTy(NULL),
      nativeTy(NULL), nativeFnTy(NULL),
      evalTy(NULL), evalFnTy(NULL),
      infoPtrTy(NULL), contextPtrTy(NULL),
      strcmp_fn(NULL),
      xl_evaluate(NULL), xl_same_text(NULL), xl_same_shape(NULL),
      xl_infix_match_check(NULL), xl_type_check(NULL), xl_form_error(NULL),
      xl_new_integer(NULL), xl_new_real(NULL), xl_new_character(NULL),
      xl_new_text(NULL), xl_new_ctext(NULL), xl_new_xtext(NULL),
      xl_new_block(NULL),
      xl_new_prefix(NULL), xl_new_postfix(NULL), xl_new_infix(NULL),
      xl_new_closure(NULL)
{
    // Register a listener with the garbage collector
    CompilerGarbageCollectionListener *cgcl =
        new CompilerGarbageCollectionListener(this);
    Allocator<Tree>     ::Singleton()->AddListener(cgcl);
    Allocator<Integer>  ::Singleton()->AddListener(cgcl);
    Allocator<Real>     ::Singleton()->AddListener(cgcl);
    Allocator<Text>     ::Singleton()->AddListener(cgcl);
    Allocator<Name>     ::Singleton()->AddListener(cgcl);
    Allocator<Infix>    ::Singleton()->AddListener(cgcl);
    Allocator<Prefix>   ::Singleton()->AddListener(cgcl);
    Allocator<Postfix>  ::Singleton()->AddListener(cgcl);
    Allocator<Block>    ::Singleton()->AddListener(cgcl);

    // Initialize native target (new features)
    InitializeNativeTarget();

    // LLVM Context (new feature)
    llvm = new llvm::LLVMContext();

    // Create module where we will build the code
    module = new Module(moduleName, *llvm);

    // Select "fast JIT" if optimize level is 0, optimizing JIT otherwise
    runtime = EngineBuilder(module).create();
    runtime->DisableLazyCompilation(true);

    // Setup the optimizer - REVISIT: Adjust with optimization level
    optimizer = new FunctionPassManager(module);
    moduleOptimizer = new PassManager;

    // Install a fallback mechanism to resolve references to the runtime, on
    // systems which do not allow the program to dlopen itself.
    runtime->InstallLazyFunctionCreator(unresolved_external);

    // Get the basic types
    booleanTy = Type::getInt1Ty(*llvm);
    integerTy = llvm::IntegerType::get(*llvm, 64);
    integer8Ty = llvm::IntegerType::get(*llvm, 8);
    integer16Ty = llvm::IntegerType::get(*llvm, 16);
    integer32Ty = llvm::IntegerType::get(*llvm, 32);
    characterTy = LLVM_INTTYPE(char);
    realTy = Type::getDoubleTy(*llvm);
    real32Ty = Type::getFloatTy(*llvm);
    charPtrTy = PointerType::get(LLVM_INTTYPE(char), 0);

    // Create the 'text' type, assume it contains a single char *
    std::vector<const Type *> textElements;
    textElements.push_back(charPtrTy);             // _M_p in gcc's impl
    textTy = StructType::get(*llvm, textElements); // text

    // Create the Info and Symbol pointer types
    PATypeHolder structInfoTy = OpaqueType::get(*llvm); // struct Info
    infoPtrTy = PointerType::get(structInfoTy, 0);         // Info *
    PATypeHolder structCtxTy = OpaqueType::get(*llvm);  // struct Context
    contextPtrTy = PointerType::get(structCtxTy, 0);       // Context *

    // Create the Tree and Tree pointer types
    PATypeHolder structTreeTy = OpaqueType::get(*llvm); // struct Tree
    treePtrTy = PointerType::get(structTreeTy, 0);      // Tree *
    treePtrPtrTy = PointerType::get(treePtrTy, 0);      // Tree **

    // Create the native_fn type
    std::vector<const Type *> nativeParms;
    nativeParms.push_back(contextPtrTy);
    nativeParms.push_back(treePtrTy);
    nativeTy = FunctionType::get(treePtrTy, nativeParms, false);
    nativeFnTy = PointerType::get(nativeTy, 0);

    // Create the eval_fn type
    std::vector<const Type *> evalParms;
    evalParms.push_back(treePtrTy);
    evalTy = FunctionType::get(treePtrTy, evalParms, false);
    evalFnTy = PointerType::get(evalTy, 0);

    // Verify that there wasn't a change in the Tree type invalidating us
    struct LocalTree
    {
        LocalTree (const Tree &o): tag(o.tag), info(o.info) {}
        ulong    tag;
        XL::Info*info;          // We check that the size is the same
    };
    // If this assert fails, you changed struct tree and need to modify here
    XL_CASSERT(sizeof(LocalTree) == sizeof(Tree));
               
    // Create the Tree type
    std::vector<const Type *> treeElements;
    treeElements.push_back(LLVM_INTTYPE(ulong));           // tag
    treeElements.push_back(infoPtrTy);                     // info
    treeTy = StructType::get(*llvm, treeElements);      // struct Tree {}
    cast<OpaqueType>(structTreeTy.get())->refineAbstractTypeTo(treeTy);
    treeTy = cast<StructType> (structTreeTy.get());

    // Create the Integer type
    std::vector<const Type *> integerElements = treeElements;
    integerElements.push_back(LLVM_INTTYPE(longlong));       // value
    integerTreeTy = StructType::get(*llvm, integerElements); // struct Integer
    integerTreePtrTy = PointerType::get(integerTreeTy,0);    // Integer *

    // Create the Real type
    std::vector<const Type *> realElements = treeElements;
    realElements.push_back(Type::getDoubleTy(*llvm));    // value
    realTreeTy = StructType::get(*llvm, realElements);   // struct Real{}
    realTreePtrTy = PointerType::get(realTreeTy, 0);     // Real *

    // Create the Text type
    std::vector<const Type *> textTreeElements = treeElements;
    textTreeElements.push_back(textTy);                        // value
    textTreeElements.push_back(textTy);                        // opening
    textTreeElements.push_back(textTy);                        // closing
    textTreeTy = StructType::get(*llvm, textTreeElements);     // struct Text
    textTreePtrTy = PointerType::get(textTreeTy, 0);           // Text *

    // Create the Name type
    std::vector<const Type *> nameElements = treeElements;
    nameElements.push_back(textTy);
    nameTreeTy = StructType::get(*llvm, nameElements);    // struct Name{}
    nameTreePtrTy = PointerType::get(nameTreeTy, 0);      // Name *

    // Create the Block type
    std::vector<const Type *> blockElements = treeElements;
    blockElements.push_back(treePtrTy);                  // Tree *
    blockElements.push_back(textTy);                     // opening
    blockElements.push_back(textTy);                     // closing
    blockTreeTy = StructType::get(*llvm, blockElements); // struct Block
    blockTreePtrTy = PointerType::get(blockTreeTy, 0);   // Block *

    // Create the Prefix type
    std::vector<const Type *> prefixElements = treeElements;
    prefixElements.push_back(treePtrTy);                   // Tree *
    prefixElements.push_back(treePtrTy);                   // Tree *
    prefixTreeTy = StructType::get(*llvm, prefixElements); // struct Prefix
    prefixTreePtrTy = PointerType::get(prefixTreeTy, 0);   // Prefix *

    // Create the Postfix type
    std::vector<const Type *> postfixElements = prefixElements;
    postfixTreeTy = StructType::get(*llvm, postfixElements); // Postfix
    postfixTreePtrTy = PointerType::get(postfixTreeTy, 0);   // Postfix *

    // Create the Infix type
    std::vector<const Type *> infixElements = prefixElements;
    infixElements.push_back(textTy);                        // name
    infixTreeTy = StructType::get(*llvm, infixElements);    // Infix
    infixTreePtrTy = PointerType::get(infixTreeTy, 0);      // Infix *

    // Record the type names
    module->addTypeName("boolean", booleanTy);
    module->addTypeName("integer", integerTy);
    module->addTypeName("character", characterTy);
    module->addTypeName("real", realTy);
    module->addTypeName("text", charPtrTy);

    module->addTypeName("Tree", treeTy);
    module->addTypeName("Integer", integerTreeTy);
    module->addTypeName("Real", realTreeTy);
    module->addTypeName("Text", textTreeTy);
    module->addTypeName("Block", blockTreeTy);
    module->addTypeName("Name", nameTreeTy);
    module->addTypeName("Prefix", prefixTreeTy);
    module->addTypeName("Postfix", postfixTreeTy);
    module->addTypeName("Infix", infixTreeTy);
    module->addTypeName("eval_fn", evalTy);
    module->addTypeName("native_fn", nativeTy);
    module->addTypeName("Info*", infoPtrTy);
    module->addTypeName("Context*", contextPtrTy);

    // Create a reference to the evaluation function
#define FN(x) #x, (void *) XL::x
    strcmp_fn = ExternFunction("strcmp", (void *) strcmp,
                               LLVM_INTTYPE(int), 2, charPtrTy, charPtrTy);
    xl_evaluate = ExternFunction(FN(xl_evaluate),
                                 treePtrTy, 2, contextPtrTy, treePtrTy);
    xl_same_text = ExternFunction(FN(xl_same_text),
                                  booleanTy, 2, treePtrTy, charPtrTy);
    xl_same_shape = ExternFunction(FN(xl_same_shape),
                                   booleanTy, 2, treePtrTy, treePtrTy);
    xl_infix_match_check = ExternFunction(FN(xl_infix_match_check),
                                          treePtrTy, 2, treePtrTy, charPtrTy);
    xl_type_check = ExternFunction(FN(xl_type_check), treePtrTy,
                                   3, contextPtrTy, treePtrTy, treePtrTy);
    xl_form_error = ExternFunction(FN(xl_form_error),
                                   treePtrTy, 1, treePtrTy);
    xl_new_integer = ExternFunction(FN(xl_new_integer),
                                    integerTreePtrTy, 1, integerTy);
    xl_new_real = ExternFunction(FN(xl_new_real),
                                 realTreePtrTy, 1, realTy);
    xl_new_character = ExternFunction(FN(xl_new_character),
                                      textTreePtrTy, 1, characterTy);
    xl_new_text = ExternFunction(FN(xl_new_text), textTreePtrTy, 1, textTy);
    xl_new_ctext = ExternFunction(FN(xl_new_ctext), textTreePtrTy, 1,charPtrTy);
    xl_new_xtext = ExternFunction(FN(xl_new_xtext), textTreePtrTy, 4,
                                  charPtrTy, integerTy, charPtrTy, charPtrTy);
    xl_new_block = ExternFunction(FN(xl_new_block), blockTreePtrTy, 2,
                                  blockTreePtrTy,treePtrTy);
    xl_new_prefix = ExternFunction(FN(xl_new_prefix), prefixTreePtrTy, 3,
                                   prefixTreePtrTy, treePtrTy, treePtrTy);
    xl_new_postfix = ExternFunction(FN(xl_new_postfix), postfixTreePtrTy, 3,
                                    postfixTreePtrTy, treePtrTy, treePtrTy);
    xl_new_infix = ExternFunction(FN(xl_new_infix), infixTreePtrTy, 3,
                                  infixTreePtrTy,treePtrTy,treePtrTy);
    xl_new_closure = ExternFunction(FN(xl_new_closure),
                                    treePtrTy, -2,
                                    treePtrTy, LLVM_INTTYPE(uint));

    // Initialize the llvm_entries table
    for (CompilerLLVMTableEntry *le = CompilerLLVMTable; le->name; le++)
        llvm_primitives[le->name] = le;
}


Compiler::~Compiler()
// ----------------------------------------------------------------------------
//    Destructor deletes the various things we had created
// ----------------------------------------------------------------------------
{
    delete llvm;
    delete optimizer;
    delete moduleOptimizer;
}


program_fn Compiler::CompileProgram(Context *context, Tree *program)
// ----------------------------------------------------------------------------
//   Compile a whole XL program
// ----------------------------------------------------------------------------
//   This is the entry point used to compile a top-level XL program.
//   It will process all the declarations in the program and then compile
//   the rest of the code as a function taking no arguments.
{
    Context_p topContext = new Context(context, context);
    CompiledUnit topUnit(this, topContext);

    if (!topUnit.TypeCheck(program))
        return NULL;
    if (!topUnit.TopLevelFunction())
        return NULL;
    llvm_value returned = topUnit.Compile(program);
    if (!returned)
        return NULL;
    if (!topUnit.Return(returned))
        return NULL;
    return (program_fn) topUnit.Finalize(true);
}


void Compiler::Setup(Options &options)
// ----------------------------------------------------------------------------
//   Setup the compiler after we have parsed the options
// ----------------------------------------------------------------------------
{
    uint optimize_level = options.optimize_level;

    createStandardFunctionPasses(optimizer, optimize_level);
    createStandardModulePasses(moduleOptimizer, optimize_level,
                               true,    /* Optimize size */
                               false,   /* Unit at a time */
                               true,    /* Unroll loops */
                               true,    /* Simplify lib calls */
                               false,    /* Have exception */
                               llvm::createFunctionInliningPass());
    createStandardLTOPasses(moduleOptimizer,
                            true, /* Internalize */
                            true, /* RunInliner */
                            true  /* Verify Each */);

    // Other target options
    // DwarfExceptionHandling = true;// Present in LLVM 2.6, but crashes
    JITEmitDebugInfo = true;         // Not present in LLVM 2.6
    UnwindTablesMandatory = true;
    // PerformTailCallOpt = true;
    NoFramePointerElim = options.debug;
}


void Compiler::Reset()
// ----------------------------------------------------------------------------
//    Clear the contents of a compiler
// ----------------------------------------------------------------------------
{
}


CompilerInfo *Compiler::Info(Tree *tree, bool create)
// ----------------------------------------------------------------------------
//   Find or create the compiler-related info for a given tree
// ----------------------------------------------------------------------------
{
    CompilerInfo *result = tree->GetInfo<CompilerInfo>();
    if (!result && create)
    {
        result = new CompilerInfo(tree);
        tree->SetInfo<CompilerInfo>(result);
    }
    return result;
}


llvm::Function * Compiler::TreeFunction(Tree *tree)
// ----------------------------------------------------------------------------
//   Return the function associated to the tree
// ----------------------------------------------------------------------------
{
    CompilerInfo *info = Info(tree);
    return info ? info->function : NULL;
}


void Compiler::SetTreeFunction(Tree *tree, llvm::Function *function)
// ----------------------------------------------------------------------------
//   Associate a function to the given tree
// ----------------------------------------------------------------------------
{
    CompilerInfo *info = Info(tree, true);
    info->function = function;
}


llvm::GlobalValue * Compiler::TreeGlobal(Tree *tree)
// ----------------------------------------------------------------------------
//   Return the global value associated to the tree, if any
// ----------------------------------------------------------------------------
{
    CompilerInfo *info = Info(tree);
    return info ? info->global : NULL;
}


void Compiler::SetTreeGlobal(Tree *tree, llvm::GlobalValue *global, void *addr)
// ----------------------------------------------------------------------------
//   Set the global value associated to the tree
// ----------------------------------------------------------------------------
{
    CompilerInfo *info = Info(tree, true);
    info->global = global;
    runtime->addGlobalMapping(global, addr ? addr : &info->tree);
}


Function *Compiler::EnterBuiltin(text name,
                                 Tree *to,
                                 TreeList parms,
                                 eval_fn code)
// ----------------------------------------------------------------------------
//   Declare a built-in function
// ----------------------------------------------------------------------------
//   The input is not technically an eval_fn, but has as many parameters as
//   there are variables in the form
{
    IFTRACE(llvm)
        std::cerr << "EnterBuiltin " << name
                  << " C" << (void *) code << " T" << (void *) to;

    Function *result = builtins[name];
    if (result)
    {
        IFTRACE(llvm)
            std::cerr << " existing F " << result
                      << " replaces F" << TreeFunction(to) << "\n";
        SetTreeFunction(to, result);
    }
    else
    {
        // Create the LLVM function
        std::vector<const Type *> parmTypes;
        parmTypes.push_back(treePtrTy); // First arg is self
        for (TreeList::iterator p = parms.begin(); p != parms.end(); p++)
            parmTypes.push_back(treePtrTy);
        FunctionType *fnTy = FunctionType::get(treePtrTy, parmTypes, false);
        result = Function::Create(fnTy, Function::ExternalLinkage,
                                  name, module);

        // Record the runtime symbol address
        sys::DynamicLibrary::AddSymbol(name, (void*) code);

        IFTRACE(llvm)
            std::cerr << " new F " << result
                      << "replaces F" << TreeFunction(to) << "\n";

        // Associate the function with the tree form
        SetTreeFunction(to, result);
        builtins[name] = result;
    }

    return result;    
}


adapter_fn Compiler::ArrayToArgsAdapter(uint numargs)
// ----------------------------------------------------------------------------
//   Generate code to call a function with N arguments
// ----------------------------------------------------------------------------
//   The generated code serves as an adapter between code that has
//   tree arguments in a C array and code that expects them as an arg-list.
//   For example, it allows you to call foo(Tree *src, Tree *a1, Tree *a2)
//   by calling generated_adapter(foo, Tree *src, Tree *args[2])
{
    IFTRACE(llvm)
        std::cerr << "EnterArrayToArgsAdapater " << numargs;

    // Check if we already computed it
    adapter_fn result = array_to_args_adapters[numargs];
    if (result)
    {
        IFTRACE(llvm)
            std::cerr << " existing C" << (void *) result << "\n";
        return result;
    }

    // Generate the function type:
    // Tree *generated(Context *, native_fn, Tree *, Tree **)
    std::vector<const Type *> parms;
    parms.push_back(nativeFnTy);
    parms.push_back(contextPtrTy);
    parms.push_back(treePtrTy);
    parms.push_back(treePtrPtrTy);
    FunctionType *fnType = FunctionType::get(treePtrTy, parms, false);
    Function *adapter = Function::Create(fnType, Function::InternalLinkage,
                                        "xl_adapter", module);

    // Generate the function type for the called function
    std::vector<const Type *> called;
    called.push_back(contextPtrTy);
    called.push_back(treePtrTy);
    for (uint a = 0; a < numargs; a++)
        called.push_back(treePtrTy);
    FunctionType *calledType = FunctionType::get(treePtrTy, called, false);
    PointerType *calledPtrType = PointerType::get(calledType, 0);

    // Create the entry for the function we generate
    BasicBlock *entry = BasicBlock::Create(*llvm, "adapt", adapter);
    IRBuilder<> code(entry);

    // Read the arguments from the function we are generating
    Function::arg_iterator inArgs = adapter->arg_begin();
    Value *fnToCall = inArgs++;
    Value *contextPtr = inArgs++;
    Value *sourceTree = inArgs++;
    Value *treeArray = inArgs++;

    // Cast the input function pointer to right type
    Value *fnTyped = code.CreateBitCast(fnToCall, calledPtrType, "fnCast");

    // Add source as first argument to output arguments
    std::vector<Value *> outArgs;
    outArgs.push_back (contextPtr);
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
    result = (adapter_fn) runtime->getPointerToFunction(adapter);
    array_to_args_adapters[numargs] = result;

    IFTRACE(llvm)
        std::cerr << " new C" << (void *) result << "\n";

    // And return it to the caller
    return result;
}


Function *Compiler::ExternFunction(kstring name, void *address,
                                   const Type *retType, int parmCount, ...)
// ----------------------------------------------------------------------------
//   Return a Function for some given external symbol
// ----------------------------------------------------------------------------
{
    IFTRACE(llvm)
        std::cerr << "ExternFunction " << name
                  << " has " << parmCount << " parameters "
                  << " C" << address;

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

    IFTRACE(llvm)
        std::cerr << " F" << result << "\n";

    return result;
}


Value *Compiler::EnterGlobal(Name *name, Name_p *address)
// ----------------------------------------------------------------------------
//   Enter a global variable in the symbol table
// ----------------------------------------------------------------------------
{
    Constant *null = ConstantPointerNull::get(treePtrTy);
    bool isConstant = false;
    GlobalValue *result = new GlobalVariable (*module, treePtrTy, isConstant,
                                              GlobalVariable::ExternalLinkage,
                                              null, name->value);
    SetTreeGlobal(name, result, address);

    IFTRACE(llvm)
        std::cerr << "EnterGlobal " << name->value
                  << " name T" << (void *) name
                  << " A" << address
                  << " address T" << (void *) address->Pointer()
                  << "\n";

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
                                              GlobalVariable::ExternalLinkage,
                                              NULL, name);
    SetTreeGlobal(constant, result, NULL);

    IFTRACE(llvm)
        std::cerr << "EnterConstant T" << (void *) constant
                  << " A" << (void *) &Info(constant)->tree << "\n";

    return result;
}


GlobalVariable *Compiler::TextConstant(text value)
// ----------------------------------------------------------------------------
//   Return a C-style string pointer for a string constant
// ----------------------------------------------------------------------------
{
    GlobalVariable *global;

    text_constants_map::iterator found = text_constants.find(value);
    if (found == text_constants.end())
    {
        Constant *refVal = ConstantArray::get(*llvm, value);
        const Type *refValTy = refVal->getType();
        global = new GlobalVariable(*module, refValTy, true,
                                    GlobalValue::InternalLinkage,
                                    refVal, "text");
        text_constants[value] = global;
    }
    else
    {
        global = (*found).second;
    }

    return global;
}


eval_fn Compiler::MarkAsClosure(XL::Tree *closure, uint ntrees)
// ----------------------------------------------------------------------------
//    Create the closure wrapper for ntrees elements, associate to result
// ----------------------------------------------------------------------------
{
    (void) closure;
    (void) ntrees;
    return NULL;
}


bool Compiler::IsKnown(Tree *tree)
// ----------------------------------------------------------------------------
//    Test if global is known
// ----------------------------------------------------------------------------
{
    return TreeGlobal(tree) != NULL;
}


llvm_type Compiler::MachineType(Tree *tree)
// ----------------------------------------------------------------------------
//    Return the LLVM type associated to a given XL type name
// ----------------------------------------------------------------------------
{
    // Check all "basic" types in basics.tbl
    if (tree == boolean_type || tree == xl_true || tree == xl_false)
        return booleanTy;
    if (tree == integer_type|| tree == integer64_type ||
        tree == unsigned_type || tree == unsigned64_type ||
        tree->Kind() == INTEGER)
        return integerTy;
    if (tree == real_type || tree == real64_type || tree->Kind() == REAL)
        return realTy;
    if (tree == character_type)
        return characterTy;
    if (tree == text_type)
        return charPtrTy;
    if (Text *text = tree->AsText())
    {
        if (text->opening == "'" && text->closing == "'")
            return characterTy;
        if (text->opening == "\"" && text->closing == "\"")
            return charPtrTy;
    }

    // Sized types
    if (tree == integer8_type || tree == unsigned8_type)
        return integer8Ty;
    if (tree == integer16_type || tree == unsigned16_type)
        return integer16Ty;
    if (tree == integer32_type || tree == unsigned32_type)
        return integer32Ty;
    if (tree == real32_type)
        return real32Ty;
    
    // Check special tree types in basics.tbl
    if (tree == symbol_type || tree == name_type || tree == operator_type)
        return nameTreePtrTy;
    if (tree == infix_type)
        return infixTreePtrTy;
    if (tree == prefix_type)
        return prefixTreePtrTy;
    if (tree == postfix_type)
        return postfixTreePtrTy;
    if (tree == block_type)
        return blockTreePtrTy;

    // Otherwise, it's a Tree *
    return treePtrTy;
}


llvm_value Compiler::Primitive(llvm_builder builder, text name,
                               uint arity, llvm_value *args)
// ----------------------------------------------------------------------------
//   Invoke an LLVM primitive, assuming it's found in the table
// ----------------------------------------------------------------------------
{
    // Find the entry in the primitives table
    llvm_entry_table::iterator found = llvm_primitives.find(name);
    if (found == llvm_primitives.end())
        return NULL;

    // If the entry doesn't have the expected arity, give up
    CompilerLLVMTableEntry *entry = (*found).second;
    if (entry->arity != arity)
        return NULL;

    // Invoke the entry
    llvm_value result = entry->function(builder, args);
    return result;
}


bool Compiler::MarkAsClosureType(llvm_type type)
// ----------------------------------------------------------------------------
//   Record which types are used as closures
// ----------------------------------------------------------------------------
{
    assert(type->isPointerTy() && "CLosures should be pointers");
    if (IsClosureType(type))
        return false;
    closure_types.push_back(type);
    return true;
}


bool Compiler::IsClosureType(llvm_type type)
// ----------------------------------------------------------------------------
//   Return true if the type is a closure type
// ----------------------------------------------------------------------------
{
    if (type->isPointerTy())
    {
        llvm_types::iterator begin = closure_types.begin();
        llvm_types::iterator end = closure_types.end();
        for (llvm_types::iterator t = begin; t != end; t++)
            if (type == *t)
                return true;
    }
    return false;
}


text Compiler::FunctionKey(Rewrite *rw, llvm_values &args)
// ----------------------------------------------------------------------------
//    Return a unique function key corresponding to a given overload
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << (void *) rw;

    for (llvm_values::iterator a = args.begin(); a != args.end(); a++)
    {
        llvm_value value = *a;
        llvm_type type = value->getType();
        out << ';' << (void *) type;
    }

    return out.str();
}


text Compiler::ClosureKey(Tree *tree, Context *context)
// ----------------------------------------------------------------------------
//    Return a unique function key corresponding to a given closure
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << (void *) tree << "@" << (void *) context;
    return out.str();
}


bool Compiler::FreeResources(Tree *tree)
// ----------------------------------------------------------------------------
//   Free the LLVM resources associated to the tree, if any
// ----------------------------------------------------------------------------
//   In the first pass, we need to clear the body and machine code for all
//   functions. This is because if we have foo() calling bar() and bar()
//   calling foo(), we will get an LLVM assert deleting one while the
//   other's body still makes a reference.
{
    bool result = true;

    IFTRACE(llvm)
        std::cerr << "FreeResources T" << (void *) tree;

    CompilerInfo *info = Info(tree);
    if (!info)
    {
        IFTRACE(llvm)
            std::cerr << " has no info\n";
        return true;
    }

    // Drop function reference if any
    if (Function *f = info->function)
    {
        bool inUse = !f->use_empty();
        
        IFTRACE(llvm)
            std::cerr << " function F" << f
                      << (inUse ? " in use" : " unused");
        
        if (inUse)
        {
            // Defer deletion until later
            result = false;
        }
        else
        {
            // Not in use, we can delete it directly
            f->eraseFromParent();
            info->function = NULL;
        }
    }
    
    // Drop any global reference
    if (GlobalValue *v = info->global)
    {
        bool inUse = !v->use_empty();
        
        IFTRACE(llvm)
            std::cerr << " global V" << v
                      << (inUse ? " in use" : " unused");
        
        if (inUse)
        {
            // Defer deletion until later
            result = false;
        }
        else
        {
            // Delete the LLVM value immediately if it's safe to do it.
            runtime->updateGlobalMapping(v, NULL);
            v->eraseFromParent();
            info->global = NULL;
        }
    }

    IFTRACE(llvm)
        std::cerr << (result ? " Delete\n" : "Preserved\n");

    return result;
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
        llvm::errs() << "map[" << (*i).first << "]=" << *(*i).second << '\n';
}


void debugv(llvm::Value *v)
// ----------------------------------------------------------------------------
//   Dump a value for the debugger
// ----------------------------------------------------------------------------
{
    llvm::errs() << *v << "\n";
}


void debugv(llvm::Type *t)
// ----------------------------------------------------------------------------
//   Dump a value for the debugger
// ----------------------------------------------------------------------------
{
    llvm::errs() << *t << "\n";
}

