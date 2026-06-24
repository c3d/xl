// *****************************************************************************
// compiler-unit.cpp                                                  XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2009-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010-2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
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
//
// The compilation unit is where most of the "action" happens, e.g. where
// the code generation happens for a given tree. It records all information
// that is transient, i.e. only exists during a given compilation phase
//
// In the following, we will consider a rewrite such as:
//    foo X:natural, Y is bar X + Y
//
// Such a rewrite is transformed into a function with a prototype that
// depends on the arguments, i.e. something like:
//    retType foo(int X, Tree *Y);
//
// The actual retType is determined dynamically from the return type of bar.
// An additional "closure" argument will be passed if the function captures
// variables from the surrounding context.

#include "compiler-unit.h"
#include "compiler-function.h"
#include "compiler-expr.h"
#include "compiler-types.h"
#include "errors.h"
#include "runtime.h"
#include "llvm-crap.h"
#include "native.h"

#include <recorder/recorder.h>
#include <stdio.h>
#include <sstream>


RECORDER(compiler_unit, 64, "Compilation unit (where all compilation happens)");
RECORDER(compiler_cache, 64, "Cache of compiled functions");

XL_BEGIN

using namespace llvm;

CompilerUnit::CompilerUnit(Compiler &compiler, Scope *scope, Tree *source)
// ----------------------------------------------------------------------------
//   Create new compiler unit
// ----------------------------------------------------------------------------
    : compiler(compiler),
      jit(compiler.jit),
      module(compiler, "xl.module"),
      context(new Context(scope)),
      source(source),
      types(new CompilerTypes(scope, nullptr)),
      globals(),
      compiled()
{
    // Local copy of the types for the macro below
    JIT::IntegerType_p  booleanTy        = compiler.booleanTy;
    JIT::IntegerType_p  naturalTy        = compiler.naturalTy;
    JIT::IntegerType_p  unsignedTy       = compiler.unsignedTy;
    JIT::IntegerType_p  ulongTy          = compiler.ulongTy;
    JIT::IntegerType_p  ulonglongTy      = compiler.ulonglongTy;
    JIT::Type_p         realTy           = compiler.realTy;
    JIT::IntegerType_p  characterTy      = compiler.characterTy;
    JIT::PointerType_p  charPtrTy        = compiler.charPtrTy;
    JIT::StructType_p   textTy           = compiler.textTy;
    JIT::PointerType_p  textPtrTy        = compiler.textPtrTy;
    JIT::PointerType_p  treePtrTy        = compiler.treePtrTy;
    JIT::PointerType_p  naturalTreePtrTy = compiler.naturalTreePtrTy;
    JIT::PointerType_p  realTreePtrTy    = compiler.realTreePtrTy;
    JIT::PointerType_p  textTreePtrTy    = compiler.textTreePtrTy;
    JIT::PointerType_p  blockTreePtrTy   = compiler.blockTreePtrTy;
    JIT::PointerType_p  prefixTreePtrTy  = compiler.prefixTreePtrTy;
    JIT::PointerType_p  postfixTreePtrTy = compiler.postfixTreePtrTy;
    JIT::PointerType_p  infixTreePtrTy   = compiler.infixTreePtrTy;
    JIT::PointerType_p  scopePtrTy       = compiler.scopePtrTy;
    JIT::PointerType_p  evalFnTy         = compiler.evalFnTy;

    // Initialize all the static functions
#define MTYPE(Name, Arity, Code)
#define UNARY(Name)
#define BINARY(Name)
#define CAST(Name)
#define ALIAS(Name, Arity, Original)
#define SPECIAL(Name, Arity, Code)
#define EXTERNAL(Name, RetTy, ...)                              \
    {                                                           \
        JIT::Signature sig { __VA_ARGS__ };                     \
        JIT::FunctionType_p fty = jit.FunctionType(RetTy, sig); \
        Name = jit.Function(fty, #Name);                        \
    }
#define VA_EXTERNAL(Name, RetTy, ...)                                   \
    {                                                                   \
        JIT::Signature sig { __VA_ARGS__ };                             \
        JIT::FunctionType_p fty = jit.FunctionType(RetTy, sig, true);   \
        Name = jit.Function(fty, #Name);                                \
    }
#include "compiler-primitives.tbl"

    Native::EnterPrototypes(compiler);

    AddClosureType(compiler.closureValueTy);

    record(compiler_unit, "Created unit %p scope %t source %t",
           this, scope, source);
    record(types, "Types %p created for source %t", types, source);
}


CompilerUnit::~CompilerUnit()
// ----------------------------------------------------------------------------
//   Destroy compiler unit
// ----------------------------------------------------------------------------
{
    record(compiler_unit, "Deleted unit %p scope %t source %t",
           this, context->Symbols(), source);
}


eval_fn CompilerUnit::Compile()
// ----------------------------------------------------------------------------
//   Compilation of the whole unit
// ----------------------------------------------------------------------------
//   This is the only point where we do expensive analysis of the XL source,
//   such as ProcessDeclaration or TypeAnalysis. The other operations in this
//   compilation unit all assume that these steps have been performed.
//   We return xl_identity on all error cases to avoid error cascades
{
    Scope *scope = context->Symbols();
    record(compiler_unit, "Compile %t in scope %t", source, scope);

    lazyBindingSource.clear();
    namedStorage.clear();
    namedStorageTypes.clear();
    closureInProgress.clear();

    Tree *type = types->TypeAnalysis(source);
    if (!type)
    {
        // Type analysis failed
        Ooops("Type analysis for $1 failed", source);
        record(compiler_unit, "Type analysis for %t failed", source);
        return xl_identity;
    }
    if (type == declaration_type)
    {
        // No instruction in input source, return as is
        record(compiler_unit, "No instructions in %t, identity", source);
        return xl_identity;
    }

    Errors errors;
    CompilerEval function(*this, source, types);
    JIT::Value_p global = function.Function();
    Global(source, global);

    JIT::Value_p returned = function.Compile(source, true);
    if (!returned || errors.HadErrors())
    {
        Ooops("Compilation failed", source);
        record(compiler_unit, "Compilation for %t failed", source);
        return xl_identity;
    }

    eval_fn result = function.Finalize(true);
    record(compiler_unit, "Compilation of %t returned %p",
           source, (void *) result);
    if (errors.HadErrors())
    {
        Ooops("Finalization failed", source);
        result = xl_identity;
    }
    return result;
}


JIT::Value_p CompilerUnit::Global(Tree *tree)
// ----------------------------------------------------------------------------
//    Return the LLVM value associated with the tree
// ----------------------------------------------------------------------------
{
    auto it = globals.find(tree);
    if (it != globals.end())
        return it->second;
    return nullptr;
}


void CompilerUnit::Global(Tree *tree, JIT::Value_p value)
// ----------------------------------------------------------------------------
//    Record the global value associated to a tree
// ----------------------------------------------------------------------------
{
    globals[tree] = value;
}


JIT::Value_p CompilerUnit::NamedStorageSlot(text name) const
// ----------------------------------------------------------------------------
//   Return the alloca for a unit-level mutable local, if any
// ----------------------------------------------------------------------------
{
    auto it = namedStorage.find(name);
    if (it != namedStorage.end())
        return it->second;
    return nullptr;
}


JIT::Type_p CompilerUnit::NamedStorageType(text name) const
// ----------------------------------------------------------------------------
//   Machine type for a unit-level mutable local alloca
// ----------------------------------------------------------------------------
{
    auto it = namedStorageTypes.find(name);
    if (it != namedStorageTypes.end())
        return it->second;
    return nullptr;
}


void CompilerUnit::SetNamedStorage(text name,
                                 JIT::Value_p slot,
                                 JIT::Type_p ty)
// ----------------------------------------------------------------------------
//   Record a mutable local alloca shared across rewrite functions
// ----------------------------------------------------------------------------
{
    namedStorage[name] = slot;
    if (ty)
        namedStorageTypes[name] = ty;
    record(compiler_unit, "Named storage %s slot %v type %T",
           name.c_str(), slot, ty);
}


JIT::Value_p CompilerUnit::LoadNamedStorage(JITBlock &code, text name)
// ----------------------------------------------------------------------------
//   Load a unit-level mutable local
// ----------------------------------------------------------------------------
{
    JIT::Value_p slot = NamedStorageSlot(name);
    if (!slot)
        return nullptr;
    JIT::Type_p ty = NamedStorageType(name);
    if (!ty)
        ty = code.Type(slot);
    return code.Load(ty, slot, "named");
}


JIT::Function_p &CompilerUnit::Compiled(Scope *scope,
                                        RewriteCandidate *rc,
                                        const JIT::Values &args)
// ----------------------------------------------------------------------------
//    Return a unique entry corresponding to this overload
// ----------------------------------------------------------------------------
{
    // Build a unique key to check if we already have the function in cache
    std::ostringstream os;
    os << (void *) rc->rewrite << "@" << (void *) scope;
    record(compiler_cache, "Key for %t", rc->rewrite);
    for (auto value : args)
    {
        JIT::Type_p type = JIT::Type(value);
        record(compiler_cache, "  Arg %T", type);
        os << '|' << (void *) type;
    }
    text key = os.str();
    record(compiler_cache, "Key is %s", key.c_str());
    return compiled[key];
}


JIT::Function_p &CompilerUnit::CompiledUnbox(JIT::Type_p type)
// ----------------------------------------------------------------------------
//    Return a unique entry corresponding to this unbox function
// ----------------------------------------------------------------------------
{
    // Build a unique key to check if we already have the function in cache
    std::ostringstream os;
    os << "Unbox" << (void *) type;
    text key = os.str();
    return compiled[key];
}


JIT::Function_p &CompilerUnit::CompiledClosure(Scope *scope, Tree *expr)
// ----------------------------------------------------------------------------
//    Return a unique function entry for the closure function
// ----------------------------------------------------------------------------
{
    std::ostringstream os;
    os << "Closure" << (void *) expr << "@" << (void *) scope;
    text key = os.str();
    return compiled[key];
}


JIT::Function_p &CompilerUnit::CompiledCSymbol(text key)
// ----------------------------------------------------------------------------
//    Return a cached LLVM declaration for a C runtime symbol + signature
// ----------------------------------------------------------------------------
{
    return compiled[key];
}


bool CompilerUnit::IsClosureType(JIT::Type_p type)
// ----------------------------------------------------------------------------
//    Check if this is a known closure type
// ----------------------------------------------------------------------------
{
    return clotypes.count(type) > 0;
}


void CompilerUnit::AddClosureType(JIT::Type_p type)
// ----------------------------------------------------------------------------
//   Mark the type as a closure type
// ----------------------------------------------------------------------------
{
    clotypes.insert(type);
}

XL_END
