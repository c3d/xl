#ifndef LLVM_CRAP_H
#define LLVM_CRAP_H
// *****************************************************************************
// llvm-crap.h                                                        XL project
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
// (C) 2014-2015,2017-2019, Christophe de Dinechin <christophe@dinechin.org>
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

#ifndef LLVM_VERSION
#error "Sorry, no can do anything without knowing the LLVM version"
#elif LLVM_VERSION < 370
// At some point, I have only so much time to waste on this.
// Feel free to enhance if you care about earlier versions of LLVM.
#error "LLVM 3.6 and earlier are not supported in this code."
#endif

#define LLVM_CRAP_DIAPER_OPEN
#include "llvm-crap.h"

// Fortunately, these LLVM headers are sufficient for our interface,
// and remained somewhat consistent across versions. Lucky us!
#include <llvm/IR/Type.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>

#include <recorder/recorder.h>
#include <string>

#define LLVM_CRAP_DIAPER_CLOSE
#include "llvm-crap.h"


// ============================================================================
//
//   LLVM data types
//
// ============================================================================

namespace llvm {
class Type;
class IntegerType;
class PointerType;
class ArrayType;
class StructType;
class FunctionType;

class Module;
class Function;
class BasicBlock;
class Value;
class GlobalValue;
class GlobalVariable;
class Constant;
}

typedef const char *            kstring;
typedef std::string             text;



// ============================================================================
//
//   Recorders related to LLVM
//
// ============================================================================

RECORDER_DECLARE(llvm);
RECORDER_DECLARE(llvm_prototypes);
RECORDER_DECLARE(llvm_externals);
RECORDER_DECLARE(llvm_functions);
RECORDER_DECLARE(llvm_constants);
RECORDER_DECLARE(llvm_builtins);
RECORDER_DECLARE(llvm_globals);
RECORDER_DECLARE(llvm_blocks);
RECORDER_DECLARE(llvm_labels);
RECORDER_DECLARE(llvm_calls);
RECORDER_DECLARE(llvm_stats);
RECORDER_DECLARE(llvm_code);
RECORDER_DECLARE(llvm_gc);
RECORDER_DECLARE(llvm_ir);



// ============================================================================
//
//   New ORC-based JIT support for after LLVM 5.0
//
// ============================================================================

namespace XL
{

// LLVM data types required for the JIT interface
typedef llvm::Type              Type,           *Type_p;
typedef llvm::IntegerType       IntegerType,    *IntegerType_p;
typedef llvm::PointerType       PointerType,    *PointerType_p;
typedef llvm::ArrayType         ArrayType,      *ArrayType_p;
typedef llvm::StructType        StructType,     *StructType_p;
typedef llvm::FunctionType      FunctionType,   *FunctionType_p;

typedef llvm::Module            Module,         *Module_p;
typedef llvm::Function          Function,       *Function_p;
typedef llvm::BasicBlock        BasicBlock,     *BasicBlock_p;
typedef llvm::Value             Value,          *Value_p;
typedef llvm::GlobalValue       GlobalValue,    *GlobalValue_p;
typedef llvm::GlobalVariable    GlobalVariable, *GlobalVariable_p;
typedef llvm::Constant          Constant,       *Constant_p;

typedef std::vector<Type_p>     Signature;
typedef std::vector<Value_p>    Values;

typedef intptr_t                ModuleID;


class JIT
// ----------------------------------------------------------------------------
//  Interface to the LLVM JIT
// ----------------------------------------------------------------------------
//  The LLVM C++ interface keeps changing
//  This interface attempts to abstract away all these changes
//  This version is for ORC, the latest iteration of the LLVM JIT
{
    friend class JITBlock;
    friend class JITBlockPrivate;
    class JITPrivate &p;

public:
    enum { BitsPerByte = 8 };

public:
    JIT(int argc, char **argv);
    ~JIT();

public:
    static Type_p       Type(Value_p value);
    static Type_p       ReturnType(Function_p fn);
    static Type_p       PointedType(Type_p ptrt);
    static bool         IsStructType(Type_p strt);

    static bool         InUse(Function_p f);
    static void         EraseFromParent(Function_p f);

    static bool         VerifyFunction(Function_p function);
    static void         Print(kstring label, Value_p value);
    static void         Print(kstring label, Type_p type);

public:
    // JIT attributes
    void                SetOptimizationLevel(unsigned opt);
    void                PrintStatistics();
    static void         StackTrace();

    // Types
    template<typename T>
    IntegerType_p       IntegerType();
    IntegerType_p       IntegerType(unsigned bits);
    Type_p              FloatType(unsigned bits);
    StructType_p        OpaqueType(kstring name = nullptr);
    StructType_p        StructType(StructType_p base, const Signature &body);
    StructType_p        StructType(const Signature &items, kstring n = nullptr);
    FunctionType_p      FunctionType(Type_p r,const Signature &p,bool va=false);
    PointerType_p       PointerType(Type_p rty);
    Type_p              VoidType();

    // Modules
    ModuleID            CreateModule(text name);
    void                DeleteModule(ModuleID id);

    // Functions
    Function_p          Function(FunctionType_p type, text name);
    void                Finalize(Function_p function);
    void *              ExecutableCode(Function_p f);

    // Prototypes and external functions
    Function_p          ExternFunction(FunctionType_p fty, text name);
    Function_p          Prototype(Function_p callee);
    Value_p             Prototype(Value_p callee);
};


template<typename T>
inline IntegerType_p JIT::IntegerType()
// ----------------------------------------------------------------------------
//    Return the integer type for type T
// ----------------------------------------------------------------------------
{
    return IntegerType(BitsPerByte * sizeof(T));
}


class JITBlock
// ----------------------------------------------------------------------------
//   Interface to the LLVM IR builder
// ----------------------------------------------------------------------------
{
    class JITPrivate      &p;
    class JITBlockPrivate &b;

public:
    JITBlock(JIT &jit, Function_p function, kstring name);
    JITBlock(const JITBlock &from, kstring name);
    JITBlock(JIT &jit);
    ~JITBlock();

    static Type_p       Type(Value_p value)      { return JIT::Type(value); }
    static Type_p       ReturnType(Function_p f) { return JIT::ReturnType(f); }

    Constant_p          BooleanConstant(bool value);
    Constant_p          IntegerConstant(Type_p ty, uint64_t value);
    Constant_p          IntegerConstant(Type_p ty, int64_t value);
    Constant_p          IntegerConstant(Type_p ty, unsigned value);
    Constant_p          IntegerConstant(Type_p ty, int value);
    Constant_p          FloatConstant(Type_p ty, double value);
    Constant_p          PointerConstant(Type_p pty, void *address);
    Value_p             TextConstant(text value);

    void                SwitchTo(JITBlock &block);

    Value_p             Call(Value_p c, Value_p a);
    Value_p             Call(Value_p c, Value_p a1, Value_p a2);
    Value_p             Call(Value_p c, Value_p a1, Value_p a2, Value_p a3);
    Value_p             Call(Value_p c, Values &args);

    BasicBlock_p        Block();
    Value_p             Return(Value_p value = nullptr);
    Value_p             Branch(JITBlock &to);
    Value_p             Branch(BasicBlock_p to);
    Value_p             IfBranch(Value_p cond, JITBlock &t, JITBlock &f);
    Value_p             IfBranch(Value_p cond, BasicBlock_p t, BasicBlock_p f);
    Value_p             Select(Value_p cond, Value_p t, Value_p f);

    Value_p             Alloca(Type_p type, kstring name = "");
    Value_p             AllocateReturnValue(Function_p f, kstring name = "");
    Value_p             StructGEP(Value_p ptr, unsigned idx, kstring name="");

#define UNARY(Name)                                                     \
    Value_p             Name(Value_p l, kstring name = "");
#define BINARY(Name)                                                    \
    Value_p             Name(Value_p l, Value_p r, kstring name = "");
#define CAST(Name)                                                      \
    Value_p             Name(Value_p l, Type_p r, kstring name = "");
#include "llvm-crap.tbl"
};

} // namespace XL

extern XL::Value_p xldebug(XL::Value_p);
extern XL::Type_p  xldebug(XL::Type_p);

#endif // LLVM_CRAP_H



// ============================================================================
//
//   Diagnostic interactively adjusting progressive emergency response (DIAPER)
//
// ============================================================================
//
//   This wonderful code is designed to contain the diagnostic-related damage
//   introduced, with ever increasing creativity, by the various iterations
//   of the LLVM headers, e.g. LLVM clang warnings against LLVM headers,
//   abuse and misuse of common macro names, and so on.


// ----------------------------------------------------------------------------
#ifdef LLVM_CRAP_DIAPER_OPEN
// ----------------------------------------------------------------------------
# pragma GCC diagnostic push

// It's getting harder and harder to ignore a warning
# pragma GCC diagnostic ignored "-Wpragmas"
# pragma GCC diagnostic ignored "-Wunknown-pragmas"
# pragma GCC diagnostic ignored "-Wunknown-warning-option"

// Ignore badly indented 'if' in 3.52
# if LLVM_VERSION >= 350 && LLVM_VERSION < 360
#  pragma GCC diagnostic ignored "-Wmisleading-indentation"
# endif

// All over the place
// # pragma GCC diagnostic ignored "-Wunused-parameter"

// Binding dereferenced null pointer in 3.7.1 LinkAllPasses.h
# if LLVM_VERSION >= 370 && LLVM_VERSION < 380
#  pragma GCC diagnostic ignored "-Wnull-dereference"
# endif

// memcpy in SmallVector for std::pair with non-trivial copy ctor (ongoing)
# if LLVM_VERSION >= 400
#  pragma GCC diagnostic ignored "-Wclass-memaccess"
# endif

// Some recent drops of LLVM have the EXTRAORINARY idea of defining DEBUG(x)
# ifdef DEBUG
#  define LLVM_CRAP_DIAPER_DEBUG       DEBUG
#  undef DEBUG
# endif // DEBUG

# undef LLVM_CRAP_DIAPER_OPEN
#endif // LLVM_CRAP_DIAPER_OPEN


// ----------------------------------------------------------------------------
#ifdef LLVM_CRAP_DIAPER_CLOSE
// ----------------------------------------------------------------------------
# pragma GCC diagnostic pop
# undef DEBUG
# ifdef LLVM_CRAP_DIAPER_DEBUG
#  define DEBUG LLVM_CRAP_DIAPER_DEBUG
# endif
# undef LLVM_CRAP_DIAPER_CLOSE
#endif // LLVM_CRAP_DIAPER_CLOSE
