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

#include <llvm/IR/Type.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>

#include <recorder/recorder.h>
#include <string>


#ifndef LLVM_VERSION
#error "Sorry, no can do anything without knowing the LLVM version"
#elif LLVM_VERSION < 500
// At some point, I have only so much time to waste on this.
// Feel free to enhance if you care about earlier versions of LLVM.
#error "LLVM versions that do not support ORC are no longer supported."
#endif



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

    static bool         InUse(Function_p f);
    static void         EraseFromParent(Function_p f);

    static bool         VerifyFunction(Function_p function);
    static void         Print(kstring label, Value_p value);

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

extern void debugv(XL::Value_p);
extern void debugv(XL::Type_p);

#endif // LLVM_CRAP_H
