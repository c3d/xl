#ifndef LLVM_CRAP_H
#define LLVM_CRAP_H
// ****************************************************************************
//  llvm-crap.h                                      XL / Tao / XL projects
// ****************************************************************************
//
//   File Description:
//
//     LLVM Compatibility Recovery Adaptive Protocol
//
//     LLVM keeps breaking the API from release to release.
//     Of course, none of the choices are documented anywhere, and
//     the documentation is left out of date for years.
//
//     So this file is an attempt at reverse engineering all the API
//     changes over time to be able to compile with various versions of LLVM
//     Be prepared for the worst. It's so ugly I had to dispose of it by
//     putting it in its own separate file.
//
//
// ****************************************************************************
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 2014 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2014 Taodyne SAS
// ****************************************************************************

#ifndef LLVM_VERSION
#error "Sorry, no can do anything without knowing the LLVM version"
#elif LLVM_VERSION < 27
// At some point, I have only so much time to waste on this.
// Feel free to enhance if you care about earlier versions of LLVM.
#error "LLVM 2.6 and earlier are not supported in this code."
#endif


// ============================================================================
//
//   Diagnostic we see in the new headers
//
// ============================================================================

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#ifdef DEBUG
#define OLD_DEBUG       DEBUG
#undef DEBUG
#endif // DEBUG



// ============================================================================
//
//                          HEADER FILE ADJUSTMENTS
//
// ============================================================================

// Where any sane library would offer something like #include <llvm.h>,
// LLVM insists on you loading every single header file they have.
// And then changes their name regularly, just for fun. When they don't
// simply remove them! It's not like anybody is using header names, right?
// The wise man says: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAARGH!
//
// SURGEON GENERAL WARNING: Do not read the code below, or else.

// Apparently, nobody complained loudly enough that LLVM would stop moving
// headers around. This is the most recent damage.
#if LLVM_VERSION < 370
#undef LLVM_CRAP_MCJIT
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/PassManager.h>
#else // >= 370
#define LLVM_CRAP_MCJIT         1
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LegacyPassManager.h>
#if LLVM_VERSION > 381
#include <llvm/Transforms/Scalar/GVN.h>
#endif // > 371
#endif // >= 370
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Signals.h>


// Ignore badly indented 'if' in 3.52
// (and it's getting harder and harder to ignore a warning)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#include <llvm/Support/CommandLine.h>
#pragma GCC diagnostic pop

#include <llvm/ADT/Statistic.h>

// Sometimes, headers magically disappear
#if LLVM_VERSION < 27
#include <llvm/ModuleProvider.h>
#endif

#if LLVM_VERSION < 30
#include <llvm/Support/StandardPasses.h>
#else
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#endif

// Sometimes, key headers magically appear. This one appeared in 2.6.
// Don't bother supporting anything earlier.
#if LLVM_VERSION < 33
#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/LLVMContext.h>
#else
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#endif


// It's also funny to move headers around for really no good reason at all,
// including core features that absolutely everybody has to use.
#if LLVM_VERSION < 33
#include <llvm/DerivedTypes.h>
#include <llvm/Module.h>
#include <llvm/GlobalValue.h>
#include <llvm/Instructions.h>
#else
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Instructions.h>
#endif

// While we are at it, why not also change the name of headers providing a
// given feature, just for additional humorous obfuscation?
#if LLVM_VERSION < 32
#include <llvm/Support/IRBuilder.h>
#elif LLVM_VERSION < 33
#include <llvm/IRBuilder.h>
#else
#include <llvm/IR/IRBuilder.h>
#endif

// For example, knowing your target is completely unoptional.
// Is it IR? Well, everything is IR, since LLVM is an IR.
// IMHO, target data layout belongs more to 'target' than to 'IR',
// but who am I to say?
#if LLVM_VERSION < 32
#include <llvm/Target/TargetData.h>
#elif LLVM_VERSION < 33
#include <llvm/DataLayout.h>
#else
#include <llvm/IR/DataLayout.h>
#endif

// Repeat at nauseam. If you can figure out why the verifier moved
// from "analysis" to "IR", let me know.
#if LLVM_VERSION < 350
#include <llvm/Analysis/Verifier.h>
#else
#include <llvm/IR/Verifier.h>
#endif

// This is perfectly logical, trust me!
#if LLVM_VERSION < 342
#include <llvm/Target/TargetSelect.h>
#else
#include <llvm/Support/TargetSelect.h>
#endif



// ============================================================================
//
//               WORKAROUNDS FOR GRATUITOUS API BREAKAGE
//
// ============================================================================

// It is not enough to break header files. It's also important to make
// changes that require distributed changes throughout the code. And to
// provide no help whatsoever fixing them, e.g. with macros or some kind
// of advanced warning that you will soon need to change the code.


// In 3.0, types suddenly became non-const, and you had to pass arguments
// using a newly created class called ArrayRef. Of course, ArrayRef could
// have been given one additional constructor taking a vector of const type
// pointers, and the compiler would have automagically adjusted things for you,
// even if that meant lying slightly about the const-ness of input types.
// But why bother when everybody can simply use a #ifdef every single time
// they call a function, build a structure, etc.
#if LLVM_VERSION < 30
typedef const llvm::Type *              llvm_type;
typedef const llvm::IntegerType *       llvm_integer_type;
#else // 3.0 or later
typedef llvm::Type *                    llvm_type;
typedef llvm::IntegerType *             llvm_integer_type;
#endif

// Other stuff that we need here. I'm waiting for the time they rename Value.
typedef llvm::Value *                   llvm_value;
typedef llvm::IRBuilder<> *             llvm_builder;
typedef llvm::Module *                  llvm_module;

// The opaque type went away in LLVM 3.0, although it still seems to be
// present in the textual form of the IR (if you trust the doc, which I don't)
// So now you need to create a structure type instead. It's actually better,
// too bad it was done without any typedef or macro to help users.
// So here are the typedefs or macros you need.
#if LLVM_VERSION < 30
typedef llvm::PATypeHolder llvm_struct;
#else // LLVM_VERSION >= 30
typedef llvm::StructType *llvm_struct;
#endif // LLVM_VERSION

// The pass manager became a template.
#if LLVM_VERSION < 371
typedef llvm::PassManager                       LLVMCrap_PassManager;
typedef llvm::FunctionPassManager               LLVMCrap_FunctionPassManager;
#else
typedef llvm::legacy::PassManager               LLVMCrap_PassManager;
typedef llvm::legacy::FunctionPassManager       LLVMCrap_FunctionPassManager;
#endif



// ============================================================================
//
//   New JIT support for after LLVM 3.5
//
// ============================================================================
//
// MCJIT requires one module per function. How helpful!
// This is inspired by code found at
// http://blog.llvm.org/2013/07/using-mcjit-with-kaleidoscope-tutorial.html
// All the functions below have a single purpose: to build an API that
// is semi-stable despite LLVM changing things underneath.

namespace LLVMCrap {

class JIT
// ----------------------------------------------------------------------------
//  Keep track of JIT information for MCJIT versions after 3.5
// ----------------------------------------------------------------------------
{
    typedef void *(*resolver_fn) (const text &name);

public:
    static llvm::LLVMContext &GlobalContext();

public:
    JIT();
    ~JIT();

    operator llvm::LLVMContext &();
    llvm::Module *      Module();
    void                SetModule(llvm::Module *module);

    llvm_struct         OpaqueType();
    llvm::StructType *  Struct(llvm_struct old,
                               std::vector<llvm_type> &elements);
    llvm_value          TextConstant(llvm_builder code, text value);

    llvm::Module *      CreateModule(text name);
    llvm::Function *    CreateFunction(llvm::FunctionType *type,
                                       text name);
    void                FinalizeFunction(llvm::Function *f);
    void *              FunctionPointer(llvm::Function *f);

    llvm::Function *    CreateExternFunction(llvm::FunctionType *type,
                                             text name);
    llvm::Constant *    CreateConstant(llvm::PointerType *type, void *pointer);
    llvm_value          Prototype(llvm_value callee);

    // Attributes
    void                SetResolver(resolver_fn resolver);
    void                SetOptimizationLevel(uint opt);
    void                SetName(llvm::Type *type, text name);
    void                AddGlobalMapping(llvm::GlobalValue *global, void *addr);
    void                EraseGlobalMapping(llvm::GlobalValue *global);

    // Code generation
    llvm_value          CreateStructGEP(llvm_builder bld,
                                        llvm_value ptr, unsigned idx,
                                        const llvm::Twine &name = "");
    llvm_value          CreateCall(llvm_builder bld,
                                   llvm_value callee,
                                   llvm_value arg1);
    llvm_value          CreateCall(llvm_builder bld,
                                   llvm_value callee,
                                   llvm_value arg1, llvm_value arg2);
    llvm_value          CreateCall(llvm_builder bld,
                                   llvm_value callee,
                                   llvm_value a1, llvm_value a2, llvm_value a3);
    llvm_value          CreateCall(llvm_builder bld,
                                   llvm_value callee,
                                   std::vector<llvm::Value *> &args);

    void                Dump();

private:
    llvm::LLVMContext &                         context;
    llvm::Module *                              module;
    resolver_fn                                 resolver;
    LLVMCrap_PassManager *                      moduleOptimizer;

#if LLVM_CRAP_MCJIT
    typedef std::pair<llvm::GlobalValue *, void *> global_t;

    std::vector<llvm::Module *>                 modules;
    std::vector<llvm::ExecutionEngine *>        engines;
    std::vector<global_t>                       globals;
#else // !LLVM_CRAP_MCJIT
    llvm::ExecutionEngine *                     runtime;
    LLVMCrap_FunctionPassManager *              optimizer;
#endif // LLVM_CRAP_MCJIT
    uint                                        optimizeLevel;
};


inline llvm::LLVMContext &JIT::GlobalContext()
// ----------------------------------------------------------------------------
//  There used to be a global context, now you should roll out your own
// ----------------------------------------------------------------------------
{
#if LLVM_VERSION < 390
    return llvm::getGlobalContext();
#else // >= 390
    static llvm::LLVMContext llvmContext;
    return llvmContext;
#endif // 390
}


inline JIT::JIT()
// ----------------------------------------------------------------------------
//   Constructor for JIT helper
// ----------------------------------------------------------------------------
    : context(GlobalContext()),
      module(), resolver(), moduleOptimizer()
#ifndef LLVM_CRAP_MCJIT
    , runtime(), optimizer()
#endif // LLVM_CRAP_MCJIT
{
    using namespace llvm;

#if LLVM_VERSION >= 33
    // I get a crash on Linux if I don't do that. Unclear why.
    LLVMInitializeAllTargetMCs();
#endif
    // Initialize native target (new features)
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    InitializeNativeTargetDisassembler();

#if LLVM_VERSION < 31
    JITExceptionHandling = false;  // Bug #1026
    JITEmitDebugInfo = true;
    NoFramePointerElim = true;
#endif
#if LLVM_VERSION < 30
    UnwindTablesMandatory = true;
#endif

#ifndef LLVM_CRAP_MCJIT

    // Create module where we will build the code
    module = new llvm::Module("xl", llvm::getGlobalContext());

#if LLVM_VERSION < 360
    // Select the fast JIT
    EngineBuilder engineBuilder(module);
#else
    // WTF version of the above
    EngineBuilder engineBuilder(std::move(moduleOwner));
    engineBuilder.setErrorStr(&llvm_crap_error_string);
#endif

#if LLVM_VERSION >= 31
    TargetOptions targetOpts;
    // targetOpts.JITEmitDebugInfo = true;
    engineBuilder.setEngineKind(EngineKind::JIT);
    engineBuilder.setTargetOptions(targetOpts);
#if LLVM_VERSION < 342
    engineBuilder.setUseOrcMCJITReplacement(true);
#endif // 350
#endif
#ifndef LLVM_CRAP_MCJIT
    runtime = engineBuilder.create();
#else // LLVM_CRAP_MCJIT
    ExecutionEngine *runtime = engineBuilder.create();
#endif // LLVM_CRAP_MCJIT

    // Check if we were successful in the creation of the runtime
    if (!runtime)
    {
        std::cerr << "ERROR: Unable to initialize LLVM\n"
#if LLVM_VERSION >= 360
                  << "LLVM error: " << llvm_crap_error_string
#endif
                  << "\n";
        exit(1);
    }

    // Make sure that code is generated as early as possible
    runtime->DisableLazyCompilation(true);

    // Setup the optimizer - REVISIT: Adjust with optimization level
    optimizer = new LLVMCrap_FunctionPassManager(module);
    moduleOptimizer = new LLVMCrap_PassManager;
#else // LLVM_CRAP_MCJIT
    sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
#endif // LLVM_CRAP_MCJIT
}


inline JIT::~JIT()
// ----------------------------------------------------------------------------
//    Destructor for JIT helper
// ----------------------------------------------------------------------------
{
    delete module;
    delete moduleOptimizer;
#ifdef LLVM_CRAP_MCJIT
    for (auto ei : engines)
        delete ei;
#else // !LLVM_CRAP_MCJIT
    delete optimizer;
#endif // LLVM_CRAP_MCJIT
}


inline JIT::operator llvm::LLVMContext &()
// ----------------------------------------------------------------------------
//   The JIT can be used as a context for compatibility with older code
// ----------------------------------------------------------------------------
{
    return context;
}


inline llvm::Module * JIT::Module()
// ----------------------------------------------------------------------------
//   Return the current compilation module for the JIT
// ----------------------------------------------------------------------------
{
    return module;
}


inline void JIT::SetModule(llvm::Module *mod)
// ----------------------------------------------------------------------------
//   Set the current module
// ----------------------------------------------------------------------------
{
    module = mod;
}


inline llvm::StructType *JIT::Struct(llvm_struct old,
                                     std::vector<llvm_type> &elements)
// ----------------------------------------------------------------------------
//    Refine a forward-declared structure type
// ----------------------------------------------------------------------------
{
#if LLVM_VERSION < 30
    llvm::StructType *stype = llvm::StructType::get(context, elements);
    llvm::cast<llvm::OpaqueType>(old.get())->refineAbstractTypeTo(stype);
    return llvm::cast<llvm::StructType> (old.get());
#else
    old->setBody(elements);
    return old;
#endif
}


inline llvm_struct JIT::OpaqueType()
// ----------------------------------------------------------------------------
//   Create an opaque type (i.e. a struct without a content)
// ----------------------------------------------------------------------------
{
#if LLVM_VERSION < 30
    return llvm::OpaqueType::get(context);
#else // LLVM_VERSION >= 30
    return llvm::StructType::create(context);
#endif // LLVM_VERSION
}


inline llvm_value JIT::TextConstant(llvm_builder code, text value)
// ----------------------------------------------------------------------------
//    Return a constant array of characters for the input text
// ----------------------------------------------------------------------------
{
#if LLVM_VERSION < 31
    llvm::Constant *array = llvm::ConstantArray::get(context, value);
#else
    llvm::Constant *array = llvm::ConstantDataArray::getString(context, value);
#endif
    llvm::GlobalVariable *gv =
                   new llvm::GlobalVariable(*module,
                                            array->getType(),
                                            true,
                                            llvm::GlobalValue::PrivateLinkage,
                                            array);
    // gv->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
    llvm::ArrayType *arrayTy = (llvm::ArrayType *) array->getType();
    llvm::PointerType *ptrTy = llvm::PointerType::get(arrayTy->getElementType(), 0);
    llvm_value result = code->CreateBitCast(gv, ptrTy);
    return result;
}


inline llvm::Module *JIT::CreateModule(text name)
// ----------------------------------------------------------------------------
//   Create a new module applicable to the current function
// ----------------------------------------------------------------------------
//   If the current module has been JITed already, we need to create a new one
//   as the MCJIT will have "closed" all relocations
{
#ifndef LLVM_CRAP_MCJIT
    return module;
#else // LLVM_CRAP_MCJIT
    static unsigned index = 0;
    name += '.';
    name += std::to_string(++index);

    llvm::Module *m = new llvm::Module(name, context);
    module = m;
    modules.push_back(m);

#if 0 && LLVM_VERSION >= 30
    llvm::PassManagerBuilder opts;
    opts.OptLevel = optimizeLevel;
    unsigned Threshold = optimizeLevel > 2 ? 275 : 225;
    opts.Inliner = llvm::createFunctionInliningPass(Threshold);
    opts.populateFunctionPassManager(*optimizer);
    opts.populateModulePassManager(*moduleOptimizer);
#endif // LLVM_VERSION 3.0

    return m;
#endif // LLVM_CRAP_MCJIT
}


inline llvm::Function *JIT::CreateExternFunction(llvm::FunctionType *type,
                                                 text name)
// ----------------------------------------------------------------------------
//    Create a function with the given name and type
// ----------------------------------------------------------------------------
{
    assert(module);
    return llvm::Function::Create(type, llvm::Function::ExternalLinkage,
                                  name, module);
}


inline llvm::Constant *JIT::CreateConstant(llvm::PointerType *type,
                                           void *pointer)
// ----------------------------------------------------------------------------
//    Create a constant pointer
// ----------------------------------------------------------------------------
{
    llvm::APInt addr(8 * sizeof(void*), (uintptr_t) pointer);
    llvm::Constant *result = llvm::Constant::getIntegerValue(type, addr);
    return result;
}


inline llvm_value JIT::Prototype(llvm_value callee)
// ----------------------------------------------------------------------------
//   Return a function acceptable for this module
// ----------------------------------------------------------------------------
//   If the function is in this module, return it, else return prototype for it
{
#ifndef LLVM_CRAP_MCJIT
    return (llvm::Function *) callee;
#else // LLVM_CRAP_MCJIT
    llvm::Function *function = (llvm::Function *) callee;
    assert(function);
    assert(module);
    text name = function->getName();

    // First check if we don't already have it in the current module
    if (module)
    {
        if (llvm::Function *f = module->getFunction(name))
        {
            IFTRACE(prototypes)
                llvm::errs() << "Prototype for " << name
                             << " found in current module " << module->getName()
                             << "\n";
            return f;
        }
    }

    // Otherwise search in other modules
    for (auto m : modules)
    {
        if (m != module)
        {
            if (llvm::Function *f = m->getFunction(name))
            {
                IFTRACE(prototypes)
                    llvm::errs() << "Prototype for " << name
                                 << " created in module " << module->getName()
                                 << " from " << m->getName()
                                 << " type " << *f->getFunctionType()
                                 << "\n";

                // Create prototype based on original function type
                llvm::Function *proto =
                    llvm::Function::Create(f->getFunctionType(),
                                           llvm::Function::ExternalLinkage,
                                           name,
                                           module);
                return proto;
            }
        }
    }

    IFTRACE(prototypes)
        llvm::errs() << "No function found for " << *callee
                     << " (" << (void *) callee << ", " << name << ")"
                     << " probably a function pointer\n";

    return callee;
#endif // LLVM_CRAP_MCJIT
}


inline llvm::Function *JIT::CreateFunction(llvm::FunctionType *type,
                                           text name)
// ----------------------------------------------------------------------------
//    Create a function with the given name and type
// ----------------------------------------------------------------------------
{
#if LLVM_CRAP_MCJIT
    if (!module)
    {
        static unsigned index = 0;
        name += '.';
        name += std::to_string(++index);
        CreateModule(name);
    }
#endif
    llvm::Function *result =
        llvm::Function::Create(type, llvm::Function::ExternalLinkage,
                               name, module);
    IFTRACE(llvm)
        llvm::errs() << "Creating " << result->getName() << "\n";
    return result;
}


inline void JIT::FinalizeFunction(llvm::Function* f)
// ----------------------------------------------------------------------------
//   Finalize function code generation
// ----------------------------------------------------------------------------
{
    IFTRACE(llvm)
        llvm::errs() << "Finalizing " << f->getName() << "\n";
    llvm::verifyFunction(*f);
}



inline void *JIT::FunctionPointer(llvm::Function* f)
// ----------------------------------------------------------------------------
//   Return an executable pointer to the function
// ----------------------------------------------------------------------------
//   In the MCJIT implementation, things are a bit more complicated,
//   since we can't just incremently add functions to modules.
{
#ifndef LLVM_CRAP_MCJIT
    if (optimizer)
        optimizer->run(*f);
    return runtime->getPointerToFunction(f);
#else // LLVM_CRAP_MCJIT
    for (auto engine : engines)
        if (void *ptr = engine->getPointerToFunction(f))
            return ptr;

    if (module)
    {
        text error;
        llvm::EngineBuilder builder((std::unique_ptr<llvm::Module>(module)));
#if LLVM_VERSION >= 360
        builder.setErrorStr(&error);
#endif // LLVM_VERSION >= 360
#if LLVM_VERSION >= 31
        builder.setEngineKind(llvm::EngineKind::JIT);
        builder.setUseOrcMCJITReplacement(true);
#endif // LLVM_VERSION >= 31

        llvm::ExecutionEngine *engine = builder.create();
        if (!engine)
        {
            std::cerr << "Error creating ExecutionEngine: " << error << "\n";
            return nullptr;
        }
        engines.push_back(engine);

        module = nullptr;
        if (resolver)
            engine->InstallLazyFunctionCreator(resolver);
        engine->clearAllGlobalMappings();
        for (auto const &global : globals)
        {
            IFTRACE(globals)
                std::cerr << "Global " << global.first
                          << "=" << global.second
                          << "\n";
            engine->addGlobalMapping(global.first, global.second);
        }
        engine->finalizeObject();
        void *result = engine->getPointerToFunction(f);
        return result;
    }
    return nullptr;
#endif // LLVM_CRAP_MCJIT
}


inline void JIT::SetResolver(resolver_fn r)
// ----------------------------------------------------------------------------
//   Set the name resolver to use for external symbols
// ----------------------------------------------------------------------------
{
    resolver = r;
#ifndef LLVM_CRAP_MCJIT
    runtime->InstallLazyFunctionCreator(r);
#endif // LLVM_CRAP_MCJIT
}


inline void JIT::SetOptimizationLevel(uint opt)
// ----------------------------------------------------------------------------
//   Set the optimization level
// ----------------------------------------------------------------------------
{
    optimizeLevel = opt;
    using namespace llvm;

#if LLVM_VERSION < 30
    createStandardModulePasses(moduleOptimizer, optLevel,
                               true,    /* Optimize size */
                               true,    /* Unit at a time */
                               true,    /* Unroll loops */
                               true,    /* Simplify lib calls */
                               false,   /* Have exception */
                               llvm::createFunctionInliningPass());
    createStandardLTOPasses(moduleOptimizer,
                            true, /* Internalize */
                            true, /* RunInliner */
                            true  /* Verify Each */);

    createStandardFunctionPasses(optimizer, optLevel);
#endif // LLVM_VErSION < 30

#ifndef LLVM_CRAP_MCJIT
    //   Add XL function passes
    LLVMCrap_FunctionPassManager &PM = *optimizer;
    PM.add(createInstructionCombiningPass()); // Clean up after IPCP & DAE
    PM.add(createCFGSimplificationPass()); // Clean up after IPCP & DAE

    // Start of function pass.
    // Break up aggregate allocas, using SSAUpdater.
#if LLVM_VERSION < 391
     PM.add(createScalarReplAggregatesPass(-1, false));
#else // >= 391
     PM.add(createScalarizerPass());
#endif // 391
     PM.add(createEarlyCSEPass());              // Catch trivial redundancies
#if LLVM_VERSION < 342
     PM.add(createSimplifyLibCallsPass());      // Library Call Optimizations
#endif
     PM.add(createJumpThreadingPass());         // Thread jumps.
     PM.add(createCorrelatedValuePropagationPass()); // Propagate conditionals
     PM.add(createCFGSimplificationPass());     // Merge & remove BBs
     PM.add(createInstructionCombiningPass());  // Combine silly seq's
     PM.add(createCFGSimplificationPass());     // Merge & remove BBs
     PM.add(createReassociatePass());           // Reassociate expressions
     PM.add(createLoopRotatePass());            // Rotate Loop
     PM.add(createLICMPass());                  // Hoist loop invariants
     PM.add(createInstructionCombiningPass());
     PM.add(createIndVarSimplifyPass());        // Canonicalize indvars
     PM.add(createLoopIdiomPass());             // Recognize idioms like memset
     PM.add(createLoopDeletionPass());          // Delete dead loops
     PM.add(createLoopUnrollPass());            // Unroll small loops
     PM.add(createInstructionCombiningPass());  // Clean up after the unroller
     PM.add(createGVNPass());                   // Remove redundancies
     PM.add(createMemCpyOptPass());             // Remove memcpy / form memset
     PM.add(createSCCPPass());                  // Constant prop with SCCP

     // Run instcombine after redundancy elimination to exploit opportunities
     // opened up by them.
     PM.add(createInstructionCombiningPass());
     PM.add(createJumpThreadingPass());         // Thread jumps
     PM.add(createCorrelatedValuePropagationPass());
     PM.add(createDeadStoreEliminationPass());  // Delete dead stores
     PM.add(createAggressiveDCEPass());         // Delete dead instructions
     PM.add(createCFGSimplificationPass());     // Merge & remove BBs

#if LLVM_VERSION >= 390
     PM.doInitialization();
#endif // LLVM_VERSION >= 390


    // If we use the old compiler, we need lazy compilation, see bug #718
    if (optimizeLevel == 1)
        runtime->DisableLazyCompilation(false);
#endif // LLVM_CRAP_MCJIT
}


inline void JIT::SetName(llvm::Type *type, text name)
// ----------------------------------------------------------------------------
//   Set the name for a type (for debugging purpose)
// ----------------------------------------------------------------------------
{
#if LLVM_VERSION < 30
    module->addTypeName(name, type);
#else
    // Not sure if it's possible to set the name for non-struct types anymore
    if (type->isStructTy())
    {
        llvm::StructType *stype = (llvm::StructType *) type;
        if (!stype->isLiteral())
            stype->setName(name);
    }
#endif
}


inline void JIT::AddGlobalMapping(llvm::GlobalValue *value, void *address)
// ----------------------------------------------------------------------------
//   Map a global value in memory
// ----------------------------------------------------------------------------
{
#ifndef LLVM_CRAP_MCJIT
    runtime->addGlobalMapping(value, address);
#else // LLVM_CRAP_MCJIT
    globals.push_back(global_t(value, address));
#endif // LLVM_CRAP_MCJIT
}


inline void JIT::EraseGlobalMapping(llvm::GlobalValue *value)
// ----------------------------------------------------------------------------
//   Erase global mapping from execution engines
// ----------------------------------------------------------------------------
{
#ifndef LLVM_CRAP_MCJIT
    runtime->updateGlobalMapping(value, NULL);
#else // LLVM_CRAP_MCJIT
    for (auto ei : engines)
        ei->updateGlobalMapping(value, NULL);
#endif // LLVM_CRAP_MCJIT
}


inline llvm_value JIT::CreateStructGEP(llvm_builder bld,
                                       llvm_value ptr, unsigned idx,
                                       const llvm::Twine &name)
// ----------------------------------------------------------------------------
//   Accessing a struct element used to be complicated. Now it's incompatible.
// ----------------------------------------------------------------------------
{
#if LLVM_VERSION < 371
    return bld->CreateConstGEP2_32(ptr, 0, idx, name);
#else // >= 371
    return bld->CreateStructGEP(nullptr, ptr, idx, name);
#endif // 371
}


inline llvm_value JIT::CreateCall(llvm_builder bld,
                                  llvm_value callee,
                                  llvm_value arg1)
// ----------------------------------------------------------------------------
//   Why not change the 'call' instruction, nobody uses it.
// ----------------------------------------------------------------------------
{
    llvm_value proto = Prototype(callee);
#if LLVM_VERSION < 390
    return bld->CreateCall(proto, arg1);
#else // >= 390
    return bld->CreateCall(proto, {arg1});
#endif // 390

}

inline llvm_value JIT::CreateCall(llvm_builder bld,
                                  llvm_value callee,
                                  llvm_value arg1, llvm_value arg2)
// ----------------------------------------------------------------------------
//   Why not change the 'call' instruction, nobody uses it.
// ----------------------------------------------------------------------------
{
    llvm_value proto = Prototype(callee);
#if LLVM_VERSION < 371
    return bld->CreateCall2(proto, arg1, arg2);
#else // >= 371
    return bld->CreateCall(proto, {arg1, arg2});
#endif // 371

}

inline llvm_value JIT::CreateCall(llvm_builder bld,
                                  llvm_value callee,
                                  llvm_value arg1,
                                  llvm_value arg2,
                                  llvm_value arg3)
// ----------------------------------------------------------------------------
//   Why not change the 'call' instruction, nobody uses it.
// ----------------------------------------------------------------------------
{
    llvm_value proto = Prototype(callee);
#if LLVM_VERSION < 371
    return bld->CreateCall3(proto, arg1, arg2, arg3);
#else // >= 371
    return bld->CreateCall(proto, {arg1, arg2, arg3});
#endif // 371
}


inline llvm_value JIT::CreateCall(llvm_builder bld,
                                  llvm_value callee,
                                  std::vector<llvm::Value *> &args)
// ----------------------------------------------------------------------------
//   Create a call from arguments
// ----------------------------------------------------------------------------
{
    llvm_value proto = Prototype(callee);
#if LLVM_VERSION < 30
    return bld->CreateCall(proto, args.begin(), args.end());
#else // LLVM_VERSION >= 30
    return bld->CreateCall(proto, llvm::ArrayRef<llvm::Value *> (args));
#endif // LLVM_VERSION
}


inline void JIT::Dump()
// ----------------------------------------------------------------------------
//   Dump all modules
// ----------------------------------------------------------------------------
{
#ifdef LLVM_CRAP_MCJIT
    for (auto module : modules)
        module->dump();
#else
    module->dump();
#endif // LLVM_CRAP_MCJIT
}


// ============================================================================
//
//    A helper class to push/pop modules with the MCJIT
//
// ============================================================================

struct JITModule
// ----------------------------------------------------------------------------
//   Save and restore the current JIT module
// ----------------------------------------------------------------------------
{
    JITModule(JIT &llvm, kstring name)
        : llvm(llvm), module(llvm.Module())
    {
        llvm.CreateModule(name);
    }
    ~JITModule()
    {
        llvm.SetModule(module);
    }

private:
    JIT &               llvm;
    llvm::Module *      module;
};


} // namespace LLVMCrap



// ============================================================================
//
//    Cleanup
//
// ============================================================================

#pragma GCC diagnostic pop
#undef DEBUG
#define DEBUG OLD_DEBUG

#endif // LLVM_CRAP_H
