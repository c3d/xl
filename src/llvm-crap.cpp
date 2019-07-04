// *****************************************************************************
// llvm-crap.cpp                                                      XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2018-2019, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

#include "llvm-crap.h"
#include "renderer.h"
#include "errors.h"
#include "main.h"               // For options



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

#define LLVM_CRAP_DIAPER_OPEN
#include "llvm-crap.h"

// llvm-config.h seems quite usable nowadays
#include <llvm/Config/llvm-config.h>

// Therefore, we can check that this is being built with a sane environment
#if LLVM_VERSION_MAJOR * 100 +                                    \
    LLVM_VERSION_MINOR *  10 +                                    \
    LLVM_VERSION_PATCH          != LLVM_VERSION
# error "Inconsistent LLVM version (llvm-config vs. headers)"
#endif // LLVM_VERSION check


// Below are the headers that I did not really sort yet
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
#include <llvm/Support/CommandLine.h>
#include <llvm/ADT/Statistic.h>


// Sometimes, headers magically disappear
#if LLVM_VERSION < 27
# include <llvm/ModuleProvider.h>
#endif

// Sometimes, headers are renamed. With a really different name.
// Not that it contains exactly the same thing, but it's the replacement.
#if LLVM_VERSION < 30
# include <llvm/Support/StandardPasses.h>
#else
# include <llvm/Transforms/IPO/PassManagerBuilder.h>
#endif

// It sometimes takes more than one version to decide where to put things
// For example, knowing your target is completely unoptional.
// Is it IR? Well, everything is IR, since LLVM is an IR.
// IMHO, target data layout belongs more to 'target' than to 'IR',
// but who am I to say? Someone else thought it was funny to change twice.
#if LLVM_VERSION < 32
# include <llvm/Support/IRBuilder.h>
# include <llvm/Target/TargetData.h>
#elif LLVM_VERSION < 33
# include <llvm/IRBuilder.h>
# include <llvm/DataLayout.h>
#else
# include <llvm/IR/IRBuilder.h>
# include <llvm/IR/DataLayout.h>
#endif

// It's also funny to move headers around for really no good reason at all,
// including core features that absolutely everybody has to use.
#if LLVM_VERSION < 33
# include <llvm/CallingConv.h>
# include <llvm/Constants.h>
# include <llvm/DerivedTypes.h>
# include <llvm/GlobalValue.h>
# include <llvm/Instructions.h>
# include <llvm/LLVMContext.h>
# include <llvm/Module.h>
#else
# include <llvm/IR/CallingConv.h>
# include <llvm/IR/Constants.h>
# include <llvm/IR/DerivedTypes.h>
# include <llvm/IR/GlobalValue.h>
# include <llvm/IR/Instructions.h>
# include <llvm/IR/LLVMContext.h>
# include <llvm/IR/Module.h>
#endif

// Let's avoid doing all the renaming at once.
// TargetSelect no longer in Target? This is perfectly logical, trust me!
#if LLVM_VERSION < 342
# include <llvm/Target/TargetSelect.h>
#else
# include <llvm/Support/TargetSelect.h>
#endif

// Repeat at nauseam. Can you figure out why "Verifier" is not "Analysis" ?
#if LLVM_VERSION < 350
#include <llvm/Analysis/Verifier.h>
#else
#include <llvm/IR/Verifier.h>
#endif

// Apparently, nobody complained loudly enough that LLVM would stop moving
// headers around. So around 370, they thought they needed to step things up.
// Time to not just move headers, but really change how things work inside.
#if LLVM_VERSION < 370
# include <llvm/ExecutionEngine/JIT.h>
# include <llvm/PassManager.h>
#else // >= 370
# define LLVM_CRAP_MCJIT 1
# include <llvm/ExecutionEngine/MCJIT.h>
# include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
# include "llvm/ExecutionEngine/SectionMemoryManager.h"
# include <llvm/IR/LegacyPassManager.h>
# include <llvm/IR/Mangler.h>
# include "llvm/ExecutionEngine/Orc/CompileUtils.h"
# include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
# include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
# include "llvm/ExecutionEngine/Orc/IndirectionUtils.h"
# include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
# include "llvm/ExecutionEngine/Orc/LazyEmittingLayer.h"
#endif // >= 370

#if LLVM_VERSION > 381
# include "llvm/ExecutionEngine/Orc/OrcRemoteTargetClient.h"
# include <llvm/Transforms/Scalar/GVN.h>
#endif // 381

#if   LLVM_VERSION < 380
# include "llvm/ExecutionEngine/Orc/OrcTargetSupport.h"
#elif LLVM_VERSION < 390
# include "llvm/ExecutionEngine/Orc/OrcArchitectureSupport.h"
#endif // LLVM_VERSION 390

#if LLVM_VERSION < 500
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#else // LLVM_VERSION >= 500
# include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#endif // LLVM_VERSION 500

#if LLVM_VERSION >= 600
# include "llvm/ExecutionEngine/Orc/SymbolStringPool.h"
#endif // LLVM_VERSION 600

// Finally, link everything together.
// That, apart for the warnings, has remained somewhat stable
#include "llvm/LinkAllIR.h"
#include "llvm/LinkAllPasses.h"

#define LLVM_CRAP_DIAPER_CLOSE
#include "llvm-crap.h"



// ============================================================================
//
//    Sane headers from real professionals
//
// ============================================================================

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>



// ============================================================================
//
//   Recorders related to LLVM
//
// ============================================================================

RECORDER(llvm,                  16, "LLVM general operations");
RECORDER(llvm_prototypes,       16, "LLVM function prototypes");
RECORDER(llvm_externals,        16, "LLVM external functions");
RECORDER(llvm_functions,        16, "LLVM functions");
RECORDER(llvm_constants,        16, "LLVM constant values");
RECORDER(llvm_builtins,         16, "LLVM builtin functions");
RECORDER(llvm_modules,          16, "LLVM modules");
RECORDER(llvm_globals,          16, "LLVM global variables");
RECORDER(llvm_blocks,           16, "LLVM basic blocks");
RECORDER(llvm_labels,           16, "LLVM labels for trees");
RECORDER(llvm_calls,            16, "LLVM calls");
RECORDER(llvm_stats,            16, "LLVM statistics");
RECORDER(llvm_code,             16, "LLVM code generation");
RECORDER(llvm_gc,               16, "LLVM garbage collection");
RECORDER(llvm_ir,               16, "LLVM intermediate representation");



namespace XL
{

// ============================================================================
//
//     Adjust types, namespaces, etc
//
// ============================================================================

// The Kaleidoscope JIT code does not bother with namespaces...
using namespace llvm;
#ifdef LLVM_CRAP_MCJIT
using namespace llvm::legacy;
using namespace llvm::orc;
#endif // LLVM_CRAP_MCJIT

#if LLVM_VERSION < 400
typedef TargetAddress                                JITTargetAddress;
#endif // LLVM_VERSION
#if LLVM_VERSION < 500
typedef std::unique_ptr<Module>                      Module_s;
typedef ObjectLinkingLayer<>                         LinkingLayer;
typedef IRCompileLayer<LinkingLayer>                 CompileLayer;
#elif LLVM_VERSION < 700
typedef std::shared_ptr<Module>                      Module_s;
typedef RTDyldObjectLinkingLayer                     LinkingLayer;
typedef IRCompileLayer<LinkingLayer, SimpleCompiler> CompileLayer;
#elif LLVM_VERSION >= 700
typedef std::unique_ptr<Module>                      Module_s;
#if LLVM_VERSION < 800
typedef RTDyldObjectLinkingLayer                     LinkingLayer;
typedef IRCompileLayer<LinkingLayer, SimpleCompiler> CompileLayer;
#elif LLVM_VERSION >= 800
typedef LegacyRTDyldObjectLinkingLayer               LinkingLayer;
typedef LegacyIRCompileLayer<LinkingLayer, SimpleCompiler> CompileLayer;
#endif // LLVM_VERSION 800
#endif // LLVM_VERSION >= 500
typedef std::unique_ptr<TargetMachine>               TargetMachine_u;
typedef std::shared_ptr<Function>                    Function_s;
typedef std::function<Module_s(Module_s)>            Optimizer;
#if LLVM_VERSION < 800
typedef IRTransformLayer<CompileLayer, Optimizer>    OptimizeLayer;
#elif LLVM_VERSION >= 800
typedef LegacyIRTransformLayer<CompileLayer, Optimizer> OptimizeLayer;
#endif // LLVM_VERSION 800
#if LLVM_VERSION < 380
typedef orc::LazyEmittingLayer<CompileLayer>         LazyEmittingLayer;
typedef orc::JITCompileCallbackManager<LazyEmittingLayer,
                                       OrcX86_64>    JITCompileCallbackManager;
#endif // LLVM_VERSION 380
typedef std::unique_ptr<JITCompileCallbackManager>   CompileCallbacks_u;

#if LLVM_VERSION >= 380
typedef std::unique_ptr<IndirectStubsManager>        IndirectStubs_u;
#endif

#if LLVM_VERSION < 380
typedef LazyEmittingLayer::ModuleSetHandleT          ModuleHandle;
#elif LLVM_VERSION < 500
typedef CompileLayer::ModuleSetHandleT               ModuleHandle;
#elif LLVM_VERSION < 700
typedef CompileLayer::ModuleHandleT                  ModuleHandle;
#else // LLVM_VERSION >= 700
typedef std::shared_ptr<SymbolResolver>              SymbolResolver_s;
typedef VModuleKey                                   ModuleHandle;
#endif // LLVM_VERSION vs. 700



// ============================================================================
//
//    Implementation of an LLVM JIT, ORC-based if ORC is there
//
// ============================================================================

struct JITInitializer
// ----------------------------------------------------------------------------
//    A structure only used to pre-initialize LLVM
// ----------------------------------------------------------------------------
{
    JITInitializer(int argc, char **argv);
};


JITInitializer::JITInitializer(int argc, char **argv)
// ----------------------------------------------------------------------------
//   Initialize the stuff in LLVM that must be initialized before target
// ----------------------------------------------------------------------------
{
    // Parse LLVM options
    std::vector<char *> llvmArgv;
    llvmArgv.push_back(argv[0]);
    for (int arg = 1; arg < argc; arg++)
        if (strncmp(argv[arg], "-llvm", 5) == 0)
            llvmArgv.push_back(argv[arg] + 5);
    llvm::cl::ParseCommandLineOptions(llvmArgv.size(), &llvmArgv[0]);
    llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);

    // I get a crash on Linux if I don't do that. Unclear why.
    LLVMInitializeAllTargetMCs();

    // Initialize native target (new features)
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    InitializeNativeTargetDisassembler();

    // Reinstall abort() handler that LLVM removes
    recorder_dump_on_common_signals(0, 0);
}


class JITPrivate
// ----------------------------------------------------------------------------
//   JIT private data (from Kaleidoscope)
// ----------------------------------------------------------------------------
{
    friend class        JIT;
    friend class        JITBlock;
    friend class        JITBlockPrivate;

    JITInitializer      initializer;
    unsigned            optLevel;
    LLVMContext         context;
    TargetMachine_u     target;
    const DataLayout    layout;
#if LLVM_VERSION >= 700
#if LLVM_VERSION < 800
    SymbolStringPool    strings;
#endif // LLVM_VERSION 800
    ExecutionSession    session;
    typedef std::shared_ptr<SymbolResolver> SymbolResolver_s;
    SymbolResolver_s    resolver;
#endif // LLVM_VERSION >= 700
    LinkingLayer        linker;
    CompileLayer        compiler;
#if LLVM_VERSION < 380
    SectionMemoryManager memoryManager;
    LazyEmittingLayer   lazyEmitter;
#else // LLVM_VERSION >= 380
    OptimizeLayer       optimizer;
#endif // LLVM_VERSION 380
    CompileCallbacks_u  callbacks;
#if LLVM_VERSION >= 380
    IndirectStubs_u     stubs;
#endif // LLVM_VERSION

    Module_s            module;
    ModuleHandle        moduleHandle;

public:
    JITPrivate(int argc, char **argv);
    ~JITPrivate();

private:
    Module_p            Module();
    ModuleID            CreateModule(text name);
    void                DeleteModule(ModuleID mod);
    Module_s            OptimizeModule(Module_s module);
    text                Mangle(text name);
    JITSymbol           Symbol(text name);
    JITTargetAddress    Address(text name);
};



static inline uint64_t globalSymbolAddress(const text &name)
// ----------------------------------------------------------------------------
//    Return the address of the symbol in the current address space
// ----------------------------------------------------------------------------
{
    uint64_t result = RTDyldMemoryManager::getSymbolAddressInProcess(name);
    record(llvm_externals, "Global address for %s is %p",
           name.c_str(), (void *) result);
    return result;
}


#if LLVM_VERSION >= 380
#if LLVM_VERSION < 390
std::unique_ptr<JITCompileCallbackManager>
createLocalCompileCallbackManager(const Triple &T,
                                  TargetAddress ErrorHandlerAddress)
// ----------------------------------------------------------------------------
//   Simplified version of 3.9.1 util function, only x86-64 support in 3.8.1
// ----------------------------------------------------------------------------
{
    switch (T.getArch())
    {
    default:
        return nullptr;

    case Triple::x86_64:
    {
        typedef orc::LocalJITCompileCallbackManager<orc::OrcX86_64> CCMgrT;
        return llvm::make_unique<CCMgrT>(ErrorHandlerAddress);
    }
    }
}


std::function<std::unique_ptr<IndirectStubsManager>()>
createLocalIndirectStubsManagerBuilder(const Triple &T)
// ----------------------------------------------------------------------------
//   Simplified version of 3.9.1 util function, only x86-64 support in 3.8.1
// ----------------------------------------------------------------------------
{
    switch (T.getArch())
    {
    default:
        return nullptr;

    case Triple::x86_64:
        return []()
        {
            return llvm::make_unique<
                orc::LocalIndirectStubsManager<orc::OrcX86_64>>();
        };
    }
}
#endif // LLVM_VERSION < 390


static inline IndirectStubs_u createStubs(TargetMachine &target)
// ----------------------------------------------------------------------------
//   Built a sstubs manager
// ----------------------------------------------------------------------------
{
    auto b = createLocalIndirectStubsManagerBuilder(target.getTargetTriple());
    return b();
}
#endif // LLVM_VERSION >= 380


JITPrivate::JITPrivate(int argc, char **argv)
// ----------------------------------------------------------------------------
//   Constructor for JIT private helper
// ----------------------------------------------------------------------------
// Lifted straight from Kaleidoscope chapter 1
// in LLVM version c921f23f8e457c3b462190e92189fae3ffcc93b0
// (This code is apparently churning quite a bit, and not for simplicity)
// If constructors take lambdas, this is functional code, right?
// Yuck. Barf.
    : initializer(argc, argv),
      optLevel(3),
      context(),
      target(EngineBuilder().selectTarget()),
      layout(target->createDataLayout()),
#if LLVM_VERSION < 500
      linker(),
#elif LLVM_VERSION < 700
      linker([]() { return std::make_shared<SectionMemoryManager>(); }),
#else // LLVM_VERSION >= 700
#if LLVM_VERSION < 800
      strings(),
#endif // LLVM_VERSION 800
      session(),
      resolver(createLegacyLookupResolver(
#if LLVM_VERSION >= 700
                   session,
#endif // LLVM_VERSION >= 700
                   [this](const text &name) -> JITSymbol
                   {
                       if (auto sym = stubs->findStub(name, false))
                           return sym;
                       if (auto sym = optimizer.findSymbol(name, false))
                           return sym;
                       else if (auto err = sym.takeError())
                           return std::move(err);
                       if (auto addr = globalSymbolAddress(name))
                           return JITSymbol(addr, JITSymbolFlags::Exported);
                       return nullptr;
                   },
                   [](llvm::Error err)
                   {
                       cantFail(std::move(err), "lookupFlags failed");
                   })),
      linker(session,
             [this](VModuleKey key)
             {
                 return LinkingLayer::Resources
                 {
                     std::make_shared<SectionMemoryManager>(),
                     resolver
                 };
             }),
#endif // LLVM_VERSION >= 700
      compiler(linker, SimpleCompiler(*target)),
#if LLVM_VERSION < 380
      memoryManager(),
      lazyEmitter(compiler),
      callbacks(llvm::make_unique<JITCompileCallbackManager>
                (lazyEmitter, memoryManager, context, 0, 64)),
#else // LLVM_VERSION >= 380
      optimizer(compiler,
                [this](Module_s module) {
                    return OptimizeModule(std::move(module));
                }),
#if LLVM_VERSION < 800
      callbacks(createLocalCompileCallbackManager(target->getTargetTriple(),
#if LLVM_VERSION >= 700
                                                  session,
#endif // LLVM_VERSION >= 700
                                                  0)),
#else // LLVM_VERSION >= 800
      callbacks(
          // Need 'cantFail' now
          cantFail(
              createLocalCompileCallbackManager(target->getTargetTriple(),
                                                session, 0))),
#endif // LLVM_VERSION 800
      stubs(createStubs(*target)),
#endif // LLVM_VERSION 380
      module(),
      moduleHandle()
{
    llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
    record(llvm, "JITPrivate %p constructed", this);
}


JITPrivate::~JITPrivate()
// ----------------------------------------------------------------------------
//    Destructor for JIT private helper
// ----------------------------------------------------------------------------
{
    DeleteModule((intptr_t) &moduleHandle);
    record(llvm, "JITPrivate %p destroyed", this);
}


Module_p JITPrivate::Module()
// ----------------------------------------------------------------------------
//   Return the current module if there is any
// ----------------------------------------------------------------------------
{
    record(llvm_modules, "Module %p in %p", module.get(), this);
    return module.get();
}


ModuleID JITPrivate::CreateModule(text name)
// ----------------------------------------------------------------------------
//   Add a module to the JIT
// ----------------------------------------------------------------------------
{
    assert (!module.get() && "Creating module while module is active");

    // The "Can't make up my mind" school of programming
#if LLVM_VERSION < 500
    module = llvm::make_unique<llvm::Module>(name, context);
#elif LLVM_VERSION < 700
    module = std::make_shared<llvm::Module>(name, context);
#elif LLVM_VERSION >= 700
    module = llvm::make_unique<llvm::Module>(name, context);
#endif // LLVM_VERSION 500
    record(llvm_modules, "Created module %p in %p", module.get(), this);
    module->setDataLayout(layout);
#if LLVM_VERSION >= 700
    moduleHandle = session.allocateVModule();
#endif
    return (intptr_t) &moduleHandle;
}


void JITPrivate::DeleteModule(ModuleID modID)
// ----------------------------------------------------------------------------
//   Remove the last module from the JIT
// ----------------------------------------------------------------------------
{
    assert(modID == (intptr_t) &moduleHandle && "Removing an unknown module");
#if LLVM_VERSION >= 700
    assert(moduleHandle && "Removing a module with null VModKey");
#endif

    // If we have transferred the module, let the optimizer cleanup,
    // else simply delete it here
    if (!module.get())
    {
#if LLVM_VERSION < 380
        lazyEmitter.removeModuleSet(moduleHandle);
#elif LLVM_VERSION < 500
        optimizer.removeModuleSet(moduleHandle);
#elif LLVM_VERSION < 600
        cantFail(optimizer.removeModule(moduleHandle));
#else
        cantFail(optimizer.removeModule(moduleHandle),
                 "Unable to remove module");
#endif // LLVM_VERSION
    }

#if LLVM_VERSION >= 700
    moduleHandle = 0;
#endif // LLVM_VERSION >= 700

    module = nullptr;
}


static void dumpModule(Module_p module, kstring message)
// ----------------------------------------------------------------------------
//   Dump a module for debugging purpose
// ----------------------------------------------------------------------------
{
    llvm::errs() << "; " << message << ":\n";
    module->print(llvm::errs(), nullptr);
}


Module_s JITPrivate::OptimizeModule(Module_s module)
// ----------------------------------------------------------------------------
//   Run the optimization pass
// ----------------------------------------------------------------------------
{
    if (RECORDER_TRACE(llvm_code) & 0x10)
        dumpModule(module.get(), "Dump of module before optimizations");

    // Create a function pass manager.
    legacy::FunctionPassManager fpm(module.get());

    // Add some optimizations.
    fpm.add(createInstructionCombiningPass());
    fpm.add(createReassociatePass());
    fpm.add(createGVNPass());
    fpm.add(createCFGSimplificationPass());

    if (optLevel >= 3)
    {
        // Start of function pass.
        // Break up aggregate allocas, using SSAUpdater.
        fpm.add(createScalarizerPass());
        fpm.add(createEarlyCSEPass());              // Catch trivial redundancies
        fpm.add(createJumpThreadingPass());         // Thread jumps.
        fpm.add(createCorrelatedValuePropagationPass()); // Propagate conditionals
        fpm.add(createCFGSimplificationPass());     // Merge & remove BBs
        fpm.add(createInstructionCombiningPass());  // Combine silly seq's
        fpm.add(createCFGSimplificationPass());     // Merge & remove BBs
        fpm.add(createReassociatePass());           // Reassociate expressions
        fpm.add(createLoopRotatePass());            // Rotate Loop
        fpm.add(createLICMPass());                  // Hoist loop invariants
        fpm.add(createInstructionCombiningPass());
        fpm.add(createIndVarSimplifyPass());        // Canonicalize indvars
        fpm.add(createLoopIdiomPass());             // Recognize idioms like memset
        fpm.add(createLoopDeletionPass());          // Delete dead loops
        fpm.add(createLoopUnrollPass());            // Unroll small loops
        fpm.add(createInstructionCombiningPass());  // Clean up after the unroller
        fpm.add(createGVNPass());                   // Remove redundancies
        fpm.add(createMemCpyOptPass());             // Remove memcpy / form memset
        fpm.add(createSCCPPass());                  // Constant prop with SCCP

        // Run instcombine after redundancy elimination to exploit opportunities
        // opened up by them.
        fpm.add(createInstructionCombiningPass());
        fpm.add(createJumpThreadingPass());         // Thread jumps
        fpm.add(createCorrelatedValuePropagationPass());
        fpm.add(createDeadStoreEliminationPass());  // Delete dead stores
        fpm.add(createAggressiveDCEPass());         // Delete dead instructions
        fpm.add(createCFGSimplificationPass());     // Merge & remove BBs
    }

    // Run the optimizations over all functions in the module being JITed
    fpm.doInitialization();
    for (auto &f : *module)
        fpm.run(f);

    if (RECORDER_TRACE(llvm_code) & 0x20)
        dumpModule(module.get(), "Dump of module after optimizations");

    // Emit LLVM IR if requested
    if (Opt::emitIR)
        module->print(llvm::outs(), nullptr);

    return module;
}


text JITPrivate::Mangle(text name)
// ----------------------------------------------------------------------------
//   Return the symbol associated with the name
// ----------------------------------------------------------------------------
{
    text mangled;
    raw_string_ostream stream(mangled);
    Mangler::getNameWithPrefix(stream, name, layout);
    return stream.str();
}


JITSymbol JITPrivate::Symbol(text name)
// ----------------------------------------------------------------------------
//   Return the symbol associated with the name
// ----------------------------------------------------------------------------
{
#if LLVM_VERSION < 380
    return lazyEmitter.findSymbol(Mangle(name), true);
#else // LLVM_VERSION >= 380
    return optimizer.findSymbol(Mangle(name), true);
#endif // LLVM_VERSION 380
}


JITTargetAddress JITPrivate::Address(text name)
// ----------------------------------------------------------------------------
//   Return the address for the given symbol
// ----------------------------------------------------------------------------
{
#if LLVM_VERSION < 700
# if LLVM_VERSION < 390
#  define rtsym(sym)    RuntimeDyld::SymbolInfo((sym).getAddress(),     \
                                                (sym).getFlags())
#  define syminfo       RuntimeDyld::SymbolInfo
# elif LLVM_VERSION < 400
#  define rtsym(sym)    ((sym).toRuntimeDyldSymbol())
#  define syminfo       RuntimeDyld::SymbolInfo
# else // LLVM_VERSION >= 400
#  define rtsym(sym)    (sym)
#  define syminfo       JITSymbol
# endif // LLVM_VERSION 400
# define nullsym         syminfo(nullptr)

# if LLVM_VERSION < 380
#  define optimizer lazyEmitter
# endif

    static text hadErrorWith = "";
    if (module.get())
    {
        auto resolver = createLambdaResolver(
            [&](const std::string &name) {
# if LLVM_VERSION >= 380
                if (auto sym = stubs->findStub(name, false))
                    return rtsym(sym);
# endif // LLVM_VERSION 380
                if (auto sym = optimizer.findSymbol(name, false))
                    return rtsym(sym);
# if LLVM_VERSION < 390
                if (auto addr = globalSymbolAddress(name))
                    return syminfo(addr, JITSymbolFlags::Exported);
                hadErrorWith = name;
                return syminfo(UINT64_MAX, JITSymbolFlags::None);
# endif // LLVM_VERSION < 390
                return nullsym;
            },
            [](const std::string &name) {
                if (auto addr = globalSymbolAddress(name))
                    return syminfo(addr, JITSymbolFlags::Exported);
                hadErrorWith = name;
# if LLVM_VERSION < 500
                return syminfo(UINT64_MAX, JITSymbolFlags::None);
# else // LLVM_VERSION >= 500
                return JITSymbol(make_error<JITSymbolNotFound>(name));
# endif // LLVM_VERSION 500
            });

# if LLVM_VERSION < 500
        std::vector< std::unique_ptr<llvm::Module> > modules;
        modules.push_back(std::move(module));
        moduleHandle = optimizer
                   .addModuleSet(std::move(modules),
                                 make_unique<SectionMemoryManager>(),
                                 std::move(resolver));
# else // LLVM_VERSION >= 500
        moduleHandle = cantFail(optimizer.addModule(std::move(module),
                                                    std::move(resolver)));
# endif // LLVM_VERSION 500
    }
# define hadError(r)     (!(r) || hadErrorWith.length())
# if LLVM_VERSION < 500
#  define errorMsg(r)   ("Unresolved symbols: " + hadErrorWith);        \
                        hadErrorWith = ""
# else // LLVM_VERSION >= 500
#  define errorMsg(r)   (hadErrorWith.length()                          \
                         ? "Unresolved symbols: " + hadErrorWith        \
                         : toString((r).takeError()));                  \
                        hadErrorWith = ""
# endif // LLVM_VERSION 500

#else // LLVM_VERSION >= 700
    if (module.get())
        cantFail(optimizer.addModule(moduleHandle, std::move(module)));
# define hadError(r)    (!(r))
# define errorMsg(r)    (toString((r).takeError()))
#endif // LLVM_VERSION < 700
    auto r = Symbol(name).getAddress();
    if (hadError(r))
    {
        text message = errorMsg(r);
        Ooops("Generating machine code for $1 failed: $2")
            .Arg(name, "'")
            .Arg(message, "");
        return 0;
    }
#if LLVM_VERSION < 500
    return r;
#else // LLVM_VERSION >= 500
    return *r;
#endif // LLVM_VERSION 500
#undef hadError
#undef errorMsg
#undef nullsym
#undef rtsym
#undef syminfo
#undef optimizer
}



// ============================================================================
//
//   JIT interface
//
// ============================================================================

JIT::JIT(int argc, char **argv)
// ----------------------------------------------------------------------------
//   Create the JIT
// ----------------------------------------------------------------------------
    : p(*new JITPrivate(argc, argv))
{
    recorder_type_fn print_value = recorder_render<raw_string_ostream,Value_p>;
    recorder_type_fn print_type = recorder_render<raw_string_ostream,Type_p>;
    recorder_configure_type('v', print_value);
    recorder_configure_type('T', print_type);
}


JIT::~JIT()
// ----------------------------------------------------------------------------
//   Delete the JIT
// ----------------------------------------------------------------------------
{
    if (p.Module())
        Ooops("Internal: Deleting JIT while a module is active");
    delete &p;
}


Type_p JIT::Type(Value_p value)
// ----------------------------------------------------------------------------
//   The type for the given value
// ----------------------------------------------------------------------------
{
    return value->getType();
}


Type_p JIT::ReturnType(Function_p fn)
// ----------------------------------------------------------------------------
//   The return type for the function
// ----------------------------------------------------------------------------
{
    return fn->getReturnType();
}


Type_p JIT::PointedType(Type_p type)
// ----------------------------------------------------------------------------
//   The type pointed to by a pointer
// ----------------------------------------------------------------------------
{
    if (type->isPointerTy())
    {
        PointerType_p ptype = (PointerType_p) type;
        return ptype->getElementType();
    }
    return nullptr;
}


bool JIT::IsStructType(Type_p strt)
// ----------------------------------------------------------------------------
//   Check if a type is a structure type
// ----------------------------------------------------------------------------
{
    return strt->isStructTy();
}


bool JIT::InUse(Function_p function)
// ----------------------------------------------------------------------------
//   Check if the function is currently in use
// ----------------------------------------------------------------------------
{
    return function->use_empty();
}


void JIT::EraseFromParent(Function_p function)
// ----------------------------------------------------------------------------
//   Erase function from the parent
// ----------------------------------------------------------------------------
{
    return function->eraseFromParent();
}


bool JIT::VerifyFunction(Function_p function)
// ----------------------------------------------------------------------------
//   Verify the input function, return true in case of error
// ----------------------------------------------------------------------------
//   WARNING: This returns true if there is an error, following llvm
{
    return llvm::verifyFunction(*function, &llvm::errs());
}


void JIT::Print(kstring label, Value_p value)
// ----------------------------------------------------------------------------
//   Print the tree on the error output
// ----------------------------------------------------------------------------
{
    if (label)
        // Emit a ';' so that we can pass that to llvm-mc -assemble
        llvm::outs() << "; " << label;
    value->print(llvm::outs());
}


void JIT::Print(kstring label, Type_p type)
// ----------------------------------------------------------------------------
//   Print the tree on the error output
// ----------------------------------------------------------------------------
{
    if (label)
        llvm::outs() << label;
    type->print(llvm::outs());
}


void JIT::SetOptimizationLevel(unsigned optLevel)
// ----------------------------------------------------------------------------
//   Set the optimization level
// ----------------------------------------------------------------------------
{
    p.optLevel = optLevel;
}


void JIT::PrintStatistics()
// ----------------------------------------------------------------------------
//   Print the LLVM statistics at end of compilation
// ----------------------------------------------------------------------------
{
    llvm::PrintStatistics(llvm::errs());
}


void JIT::StackTrace()
// ----------------------------------------------------------------------------
//    Dump the stack trace in case of crash
// ----------------------------------------------------------------------------
{
    sys::PrintStackTrace(llvm::errs());
}


IntegerType_p JIT::IntegerType(unsigned bits)
// ----------------------------------------------------------------------------
//   Create an integer type with the given number of bits
// ----------------------------------------------------------------------------
{
    return IntegerType::get(p.context, bits);
}


Type_p JIT::FloatType(unsigned bits)
// ----------------------------------------------------------------------------
//   Create a floating-type type with the given number of bits
// ----------------------------------------------------------------------------
{
    assert (bits == 16 || bits == 32 || bits == 64);
    if (bits == 16)
        return Type::getHalfTy(p.context);
    if (bits == 32)
        return Type::getFloatTy(p.context);
    return Type::getDoubleTy(p.context);
}


StructType_p JIT::OpaqueType(kstring name)
// ----------------------------------------------------------------------------
//   Create an opaque type (i.e. a struct without a content)
// ----------------------------------------------------------------------------
{
    return StructType::create(p.context, name);
}


StructType_p JIT::StructType(StructType_p base, const Signature &elements)
// ----------------------------------------------------------------------------
//    Refine a forward-declared structure type
// ----------------------------------------------------------------------------
{
    base->setBody(elements);
    return base;
}


StructType_p JIT::StructType(const Signature &items, kstring name)
// ----------------------------------------------------------------------------
//    Define a structure type in one pass
// ----------------------------------------------------------------------------
{
    StructType_p type = StructType::create(p.context,
                                           ArrayRef<Type_p>(items),
                                           name);
    return type;
}


FunctionType_p JIT::FunctionType(Type_p rty, const Signature &parms, bool va)
// ----------------------------------------------------------------------------
//    Create a function type
// ----------------------------------------------------------------------------
{
    return llvm::FunctionType::get(rty, parms, va);
}


PointerType_p JIT::PointerType(Type_p rty)
// ----------------------------------------------------------------------------
//    Create a pointer type (always in address space 0)
// ----------------------------------------------------------------------------
{
    return llvm::PointerType::get(rty, 0);
}


Type_p JIT::VoidType()
// ----------------------------------------------------------------------------
//   Return the void type
// ----------------------------------------------------------------------------
{
    return llvm::Type::getVoidTy(p.context);
}


ModuleID JIT::CreateModule(text name)
// ----------------------------------------------------------------------------
//   Create a module in the JIT
// ----------------------------------------------------------------------------
{
    return p.CreateModule(name);
}


void JIT::DeleteModule(ModuleID modID)
// ----------------------------------------------------------------------------
//   Delete a module in the JIT
// ----------------------------------------------------------------------------
{
    p.DeleteModule(modID);
}


Function_p JIT::Function(FunctionType_p type, text name)
// ----------------------------------------------------------------------------
//    Create a function with the given name and type
// ----------------------------------------------------------------------------
{
    Module_p module = p.Module();
    assert(module && "Creating a function without a module");
    Function_p f = llvm::Function::Create(type,
                                          llvm::Function::ExternalLinkage,
                                          name, module);
    record(llvm_functions, "Created function %v type %v in module %p",
           f, type, p.Module());

    return f;
}


void JIT::Finalize(Function_p f)
// ----------------------------------------------------------------------------
//   Finalize function code generation
// ----------------------------------------------------------------------------
{
    record(llvm_functions, "Finalizing %v", f);
    verifyFunction(*f);
}


void *JIT::ExecutableCode(Function_p f)
// ----------------------------------------------------------------------------
//   Return an executable pointer to the function
// ----------------------------------------------------------------------------
{
    JITTargetAddress address = p.Address(f->getName());
    record(llvm_functions, "Address of %v is %p", f, (void *) address);
    return (void *) address;
}


Function_p JIT::ExternFunction(FunctionType_p type, text name)
// ----------------------------------------------------------------------------
//    Create an extern function with the given name and type
// ----------------------------------------------------------------------------
{
    assert(p.module);
    Function_p f = llvm::Function::Create(type,
                                          llvm::Function::ExternalLinkage,
                                          name, p.Module());
    record(llvm_externals, "Extern function %s is %v type %T", name, f, type);
    return f;
}


Function_p JIT::Prototype(Function_p function)
// ----------------------------------------------------------------------------
//   Return a function prototype acceptable for this module
// ----------------------------------------------------------------------------
//   If the function is in this module, return it, else return prototype for it
{
    Module_p module = p.Module();
    text     name   = function->getName();

    // First check if we don't already have it in the current module
    if (module)
    {
        if (Function_p f = module->getFunction(name))
        {
            record(llvm_prototypes,
                   "Prototype for %v found in current module %p",
                   f, module);
            return f;
        }
    }

    FunctionType_p type = function->getFunctionType();
    Function_p proto = llvm::Function::Create(type,
                                              llvm::Function::ExternalLinkage,
                                              name,
                                              module);
    record(llvm_prototypes, "Created prototype %v for %v type %T in module %p",
           proto, function, type, module);
    return proto;
}


Value_p JIT::Prototype(Value_p callee)
// ----------------------------------------------------------------------------
//   Build a prototype from a callee that may not be a function
// ----------------------------------------------------------------------------
{
    Type_p type = callee->getType();
    if (type->isFunctionTy())
        return Prototype(Function_p(callee));

    record(llvm_prototypes, "Prototype for value %v type %T", callee, type);
    assert(type->isPointerTy() && "JIT::Prototype requires a callable value");
    return callee;
}



// ============================================================================
//
//    JITBlock class
//
// ============================================================================

typedef IRBuilder<> JITBuilder;

class JITBlockPrivate
// ----------------------------------------------------------------------------
//   LLVM data for a basic block in the JIT
// ----------------------------------------------------------------------------
{
    friend class JITBlock;
    JIT &               jit;
    BasicBlock_p        block;
    JITBuilder *        builder;
    kstring             name;

    JITBlockPrivate(JIT &jit, Function_p function, kstring name);
    JITBlockPrivate(const JITBlockPrivate &other, kstring name);
    JITBlockPrivate(JIT &jit);
    ~JITBlockPrivate();

    JITBlockPrivate &operator =(const JITBlockPrivate &block);

    JITBuilder *        operator->() { return builder; }
};


JITBlockPrivate::JITBlockPrivate(JIT &jit, Function_p function, kstring name)
// ----------------------------------------------------------------------------
//   Create private data for a JIT block
// ----------------------------------------------------------------------------
    : jit(jit),
      block(BasicBlock::Create(jit.p.context, name, function)),
      builder(new JITBuilder(block)),
      name(name)
{
    record(llvm_blocks, "Create JIT block '%+s' %p for block %p",
           name, this, block);
}


JITBlockPrivate::JITBlockPrivate(const JITBlockPrivate &other, kstring name)
// ----------------------------------------------------------------------------
//   Create a new basic block in the same function as 'other'
// ----------------------------------------------------------------------------
    : jit(other.jit),
      block(BasicBlock::Create(jit.p.context, name, other.block->getParent())),
      builder(new JITBuilder(block)),
      name(name)
{
    record(llvm_blocks, "Copy JIT block '%+s' %p for block %p from %p",
           name, this, block, &other);
}


JITBlockPrivate::JITBlockPrivate(JIT &jit)
// ----------------------------------------------------------------------------
//   An empty JIT block private for extern functions
// ----------------------------------------------------------------------------
    : jit(jit),
      block(nullptr),
      builder(nullptr),
      name(nullptr)
{}


JITBlockPrivate::~JITBlockPrivate()
// ----------------------------------------------------------------------------
//   Delete the private information for the block
// ----------------------------------------------------------------------------
{
    delete builder;
}


JITBlockPrivate &JITBlockPrivate::operator=(const JITBlockPrivate &o)
// ----------------------------------------------------------------------------
//   Copy data from the other private block
// ----------------------------------------------------------------------------
{
    XL_ASSERT(&jit == &o.jit);
    block = BasicBlock::Create(jit.p.context, o.name, o.block->getParent());
    delete builder;
    builder = new JITBuilder(block);
    name = o.name;
    return *this;
}


JITBlock::JITBlock(JIT &jit, Function_p function, kstring name)
// ----------------------------------------------------------------------------
//   Create a new JIT block
// ----------------------------------------------------------------------------
    : p(jit.p),
      b(*new JITBlockPrivate(jit, function, name))
{}


JITBlock::JITBlock(const JITBlock &other, kstring name)
// ----------------------------------------------------------------------------
//   Create a JIT block for the same function as another block
// ----------------------------------------------------------------------------
    : p(other.p),
      b(*new JITBlockPrivate(other.b, name))
{}


JITBlock::JITBlock(JIT &jit)
// ----------------------------------------------------------------------------
//   Create a JIT block for the same function as another block
// ----------------------------------------------------------------------------
    : p(jit.p),
      b(*new JITBlockPrivate(jit))
{}


JITBlock::~JITBlock()
// ----------------------------------------------------------------------------
//   Delete the private information
// ----------------------------------------------------------------------------
{
    delete &b;
}


JITBlock &JITBlock::operator=(const JITBlock &block)
// ----------------------------------------------------------------------------
//   Copy a JIT block
// ----------------------------------------------------------------------------
{
    XL_ASSERT(&p == &block.p);
    b = block.b;
    return *this;
}


Constant_p JITBlock::BooleanConstant(bool value)
// ----------------------------------------------------------------------------
//   Build a boolean integer constant
// ----------------------------------------------------------------------------
{
    Type_p ty = llvm::Type::getInt1Ty(p.context);
    Constant_p result = ConstantInt::get(ty, value);
    record(llvm_constants, "Unsigned constant %v for %llu", result, value);
    return result;
}


Constant_p JITBlock::IntegerConstant(Type_p ty, uint64_t value)
// ----------------------------------------------------------------------------
//   Build an unsigned integer constant
// ----------------------------------------------------------------------------
{
    assert(ty->isIntegerTy());
    Constant_p result = ConstantInt::get(ty, value);
    record(llvm_constants, "Unsigned constant %v for %llu", result, value);
    return result;
}


Constant_p JITBlock::IntegerConstant(Type_p ty, int64_t value)
// ----------------------------------------------------------------------------
//   Build a signed integer constant
// ----------------------------------------------------------------------------
{
    assert(ty->isIntegerTy());
    Constant_p result = ConstantInt::get(ty, value);
    record(llvm_constants, "Signed constant %v for %lld", result, value);
    return result;
}


Constant_p JITBlock::IntegerConstant(Type_p ty, unsigned value)
// ----------------------------------------------------------------------------
//   Build an unsigned integer constant
// ----------------------------------------------------------------------------
{
    return IntegerConstant(ty, uint64_t(value));
}


Constant_p JITBlock::IntegerConstant(Type_p ty, int value)
// ----------------------------------------------------------------------------
//   Build a signed integer constant
// ----------------------------------------------------------------------------
{
    return IntegerConstant(ty, int64_t(value));
}


Constant_p JITBlock::FloatConstant(Type_p ty, double value)
// ----------------------------------------------------------------------------
//   Build a floating-point constant
// ----------------------------------------------------------------------------
{
    Constant_p result = ConstantFP::get(ty, value);
    record(llvm_constants, "FP constant %v for %g", result, value);
    return result;
}


Constant_p JITBlock::PointerConstant(Type_p type, void *pointer)
// ----------------------------------------------------------------------------
//    Create a constant pointer
// ----------------------------------------------------------------------------
{
    llvm::APInt addr(JIT::BitsPerByte * sizeof(void *), (uintptr_t) pointer);
    Constant_p result = llvm::Constant::getIntegerValue(type, addr);
    record(llvm_constants, "Pointer constant %v for %p", result, pointer);
    return result;
}


Value_p JITBlock::TextConstant(text value)
// ----------------------------------------------------------------------------
//   Return a constant array of characters for the input text
// ----------------------------------------------------------------------------
{
    Value_p result = b->CreateGlobalStringPtr(value);
    record(llvm_constants, "Text constant %v for %s", result, value.c_str());
    return result;
}


void JITBlock::SwitchTo(JITBlock &block)
// ----------------------------------------------------------------------------
//   Switch the insertion point to the point in the other block
// ----------------------------------------------------------------------------
{
    record(llvm_ir, "Switching insertion point of %p to %p (%v)",
           this, &block, block.b.block);
    b->SetInsertPoint(block.b.block);
}


void JITBlock::SwitchTo(BasicBlock_p block)
// ----------------------------------------------------------------------------
//   Switch the insertion point to the given basic block
// ----------------------------------------------------------------------------
{
    record(llvm_ir, "Switching insertion point of %p to %p",
           this, block);
    b->SetInsertPoint(block);
}


Value_p JITBlock::Call(Value_p callee, Value_p arg1)
// ----------------------------------------------------------------------------
//   Create a call with one argument
// ----------------------------------------------------------------------------
{
    Value_p proto = b.jit.Prototype(callee);
    Value_p result = b->CreateCall(proto, {arg1});
    record(llvm_ir, "Call %v(%v) = %v", callee, arg1, result);
    return result;
}


Value_p JITBlock::Call(Value_p callee, Value_p arg1, Value_p arg2)
// ----------------------------------------------------------------------------
//   Create a call with two arguments
// ----------------------------------------------------------------------------
{
    Value_p proto = b.jit.Prototype(callee);
    Value_p result = b->CreateCall(proto, {arg1, arg2});
    record(llvm_ir, "Call %v(%v, %v) = %v", callee, arg1, arg2, result);
    return result;
}


Value_p JITBlock::Call(Value_p callee, Value_p arg1, Value_p arg2, Value_p arg3)
// ----------------------------------------------------------------------------
//   Create a call with three arguments
// ----------------------------------------------------------------------------
{
    Value_p proto = b.jit.Prototype(callee);
    Value_p result = b->CreateCall(proto, {arg1, arg2, arg3});
    record(llvm_ir, "Call %v(%v, %v, %v) = %v", callee, arg1,arg2,arg3, result);
    return result;
}


Value_p JITBlock::Call(Value_p callee, Values &args)
// ----------------------------------------------------------------------------
//   Create a call with an arbitrary list of arguments
// ----------------------------------------------------------------------------
{
    Value_p proto = b.jit.Prototype(callee);
    Value_p result = b->CreateCall(proto, ArrayRef<Value_p>(args));
    record(llvm_ir, "Call %v(#%u) = %v", callee, args.size(), result);
    return result;
}


BasicBlock_p JITBlock::Block()
// ----------------------------------------------------------------------------
//   Return the basic block for this JIT block
// ----------------------------------------------------------------------------
{
    return b.block;
}


BasicBlock_p JITBlock::NewBlock(kstring name)
// ----------------------------------------------------------------------------
//   Create a new basic block in the same function as current block
// ----------------------------------------------------------------------------
{
    return BasicBlock::Create(p.context, name, b.block->getParent());
}


Value_p JITBlock::Return(Value_p value)
// ----------------------------------------------------------------------------
//  Return the given value, or RetVoid if nullptr
// ----------------------------------------------------------------------------
{
    auto inst = value ? b->CreateRet(value) : b->CreateRetVoid();
    record(llvm_ir, "Return(%v) is %v", value, inst);
    return inst;
}


Value_p JITBlock::Branch(JITBlock &to)
// ----------------------------------------------------------------------------
//   Create an unconditinal branch
// ----------------------------------------------------------------------------
{
    auto inst = b->CreateBr(to.b.block);
    record(llvm_ir, "Branch(%v) is %v", to.b.block, inst);
    return inst;
}


Value_p JITBlock::Branch(BasicBlock_p to)
// ----------------------------------------------------------------------------
//   Create an unconditinal branch
// ----------------------------------------------------------------------------
{
    auto inst = b->CreateBr(to);
    record(llvm_ir, "Block Branch(%v) is %v", to, inst);
    return inst;
}


Value_p JITBlock::IfBranch(Value_p cond, JITBlock &t, JITBlock &f)
// ----------------------------------------------------------------------------
//  Create a conditional branch
// ----------------------------------------------------------------------------
{
    auto inst = b->CreateCondBr(cond, t.b.block, f.b.block);
    record(llvm_ir, "Conditional branch(%v, %v, %v) = %v",
           cond, t.b.block, f.b.block, inst);
    return inst;
}


Value_p JITBlock::IfBranch(Value_p cond, BasicBlock_p t, BasicBlock_p f)
// ----------------------------------------------------------------------------
//  Create a conditional branch
// ----------------------------------------------------------------------------
{
    auto inst = b->CreateCondBr(cond, t, f);
    record(llvm_ir, "Conditional lock branch(%v, %v, %v) = %v",
           cond, t, f, inst);
    return inst;
}


Value_p JITBlock::Select(Value_p cond, Value_p t, Value_p f)
// ----------------------------------------------------------------------------
//   Create a select
// ----------------------------------------------------------------------------
{
    auto inst = b->CreateSelect(cond, t, f);
    record(llvm_ir, "Conditional select(%v, %v, %v) = %v",
           cond, t, f, inst);
    return inst;

}


Value_p JITBlock::Alloca(Type_p type, kstring name)
// ----------------------------------------------------------------------------
//  Do a local allocation
// ----------------------------------------------------------------------------
{
#if LLVM_VERSION < 500
    auto inst = b->CreateAlloca(type, 0, name);
#else // LLVM_VERSION >= 500
    auto inst = b->CreateAlloca(type, 0, nullptr, name);
#endif // LLVM_VERSION 500
    record(llvm_ir, "Alloca %s(%T) is %v", name, type, inst);
    return inst;
}


Value_p JITBlock::AllocateReturnValue(Function_p f, kstring name)
// ----------------------------------------------------------------------------
//   Do an alloca for the return value
// ----------------------------------------------------------------------------
{
    Type_p ret = f->getReturnType();
    if (ret->isVoidTy())
        return nullptr;
    return Alloca(ret, name);
}


Value_p JITBlock::StructGEP(Value_p ptr, unsigned idx, kstring name)
// ----------------------------------------------------------------------------
//   Accessing a struct element used to be complicated. Now it's incompatible.
// ----------------------------------------------------------------------------
{
    auto inst = b->CreateStructGEP(nullptr, ptr, idx, name);
    record(llvm_ir, "StructGEP %+s(%v, %u) is %v", name, ptr, idx, inst);
    return inst;
}


Value_p JITBlock::ArrayGEP(Value_p ptr, uint32_t idx, kstring name)
// ----------------------------------------------------------------------------
//   Accessing an array element with a fixed index
// ----------------------------------------------------------------------------
{
    auto inst =  b->CreateConstGEP1_32(nullptr, ptr, idx, name);
    record(llvm_ir, "ArrayGEP %+s(%v, %u) is %v", name, ptr, idx, inst);
    return inst;
}


#define UNARY(Name)                                                     \
/* ------------------------------------------------------------ */      \
/*  Create a unary operator                                     */      \
/* ------------------------------------------------------------ */      \
Value_p JITBlock::Name(Value_p v, kstring name)                         \
{                                                                       \
    auto value = b->Create##Name(v, name);                              \
    record(llvm_ir, #Name " %+s(%v) = %v", name, v, value);             \
    return value;                                                       \
}


#define BINARY(Name)                                                    \
/* ------------------------------------------------------------ */      \
/*  Create a binary operator                                    */      \
/* ------------------------------------------------------------ */      \
    Value_p JITBlock::Name(Value_p l, Value_p r, kstring name)          \
{                                                                       \
    auto value = b->Create##Name(l, r, name);                           \
    record(llvm_ir, #Name " %+s(%v, %v) = %v", name, l, r, value);      \
    return value;                                                       \
}


#define CAST(Name)                                                      \
/* ------------------------------------------------------------ */      \
/*  Create a cast operation                                     */      \
/* ------------------------------------------------------------ */      \
Value_p JITBlock::Name(Value_p v, Type_p t, kstring name)               \
{                                                                       \
    auto value = b->Create##Name(v, t, name);                           \
    record(llvm_ir, #Name " %+s(%v, type %T) = %v", name, v, t, value); \
    return value;                                                       \
}

#include "llvm-crap.tbl"

} // namespace XL



// ============================================================================
//
//    Debug helpers
//
// ============================================================================

XL::Value_p xldebug(XL::Value_p v)
// ----------------------------------------------------------------------------
//   Dump a value for the debugger
// ----------------------------------------------------------------------------
{
    llvm::errs() << "V" << (void *) v << ": ";
    if (v)
        v->print(llvm::errs());
    else
        llvm::errs() << "NULL";
    llvm::errs() << "\n";
    return v;
}


XL::Type_p xldebug(XL::Type_p t)
// ----------------------------------------------------------------------------
//   Dump a value for the debugger
// ----------------------------------------------------------------------------
{
    llvm::errs() << "T" << (void *) t << ": ";
    if (t)
        t->print(llvm::errs());
    else
        llvm::errs() << "NULL";
    llvm::errs() << "\n";
    return t;
}
