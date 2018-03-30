// ****************************************************************************
//  unit.cpp                                                      XL project
// ****************************************************************************
//
//   File Description:
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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************
//
// The compilation unit is where most of the "action" happens, e.g. where
// the code generation happens for a given tree. It records all information
// that is transient, i.e. only exists during a given compilation phase
//
// In the following, we will consider a rewrite such as:
//    foo X:integer, Y -> bar X + Y
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
#include "errors.h"
#include "types.h"
#include "runtime.h"
#include "llvm-crap.h"
#include "recorder/recorder.h"
#include <stdio.h>
#include <sstream>


RECORDER(compiler_unit, 64, "Compilation unit (where all compilation happens)");

XL_BEGIN

using namespace llvm;

CompilerUnit::CompilerUnit(Compiler &compiler, Scope *scope, Tree *source)
// ----------------------------------------------------------------------------
//   Create new compiler unit
// ----------------------------------------------------------------------------
    : compiler(compiler),
      jit(compiler.jit),
      module(jit.CreateModule("xl.module")),
      context(new Context(scope)),
      source(source),
      types(new Types(scope)),
      globals(),
      compiled()
{
    // Local copy of the types for the macro below
    IntegerType_p  booleanTy        = compiler.booleanTy;
    IntegerType_p  integerTy        = compiler.integerTy;
    IntegerType_p  unsignedTy       = compiler.unsignedTy;
    IntegerType_p  ulongTy          = compiler.ulongTy;
    IntegerType_p  ulonglongTy      = compiler.ulonglongTy;
    Type_p         realTy           = compiler.realTy;
    IntegerType_p  characterTy      = compiler.characterTy;
    PointerType_p  charPtrTy        = compiler.charPtrTy;
    StructType_p   textTy           = compiler.textTy;
    PointerType_p  treePtrTy        = compiler.treePtrTy;
    PointerType_p  integerTreePtrTy = compiler.integerTreePtrTy;
    PointerType_p  realTreePtrTy    = compiler.realTreePtrTy;
    PointerType_p  textTreePtrTy    = compiler.textTreePtrTy;
    PointerType_p  blockTreePtrTy   = compiler.blockTreePtrTy;
    PointerType_p  prefixTreePtrTy  = compiler.prefixTreePtrTy;
    PointerType_p  postfixTreePtrTy = compiler.postfixTreePtrTy;
    PointerType_p  infixTreePtrTy   = compiler.infixTreePtrTy;
    PointerType_p  scopePtrTy       = compiler.scopePtrTy;
    PointerType_p  evalFnTy         = compiler.evalFnTy;

    // Initialize all the static functions
#define UNARY(Name)
#define BINARY(Name)
#define CAST(Name)
#define ALIAS(Name, Arity, Original)
#define SPECIAL(Name, Arity, Code)
#define EXTERNAL(Name, RetTy, ...)                              \
    {                                                           \
        Signature sig { __VA_ARGS__ };                          \
        FunctionType_p fty = jit.FunctionType(RetTy, sig);      \
        Name = jit.Function(fty, #Name);                        \
    }
#include "compiler-primitives.tbl"


    record(compiler_unit, "Created unit %p scope %t source %t",
           this, scope, source);
}


CompilerUnit::~CompilerUnit()
// ----------------------------------------------------------------------------
//   Destroy compiler unit
// ----------------------------------------------------------------------------
{
    jit.DeleteModule(module);
    record(compiler_unit, "Deleted unit %p scope %t source %t",
           this, context->CurrentScope(), source);
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
    Scope *scope = context->CurrentScope();
    record(compiler_unit, "Compile %t in scope %t", source, scope);
    if (!context->ProcessDeclarations(source))
    {
        // No instruction in input source, return as is
        record(compiler_unit, "No instructions in %t, identity", source);
        return xl_identity;
    }

    if (!types->TypeAnalysis(source))
    {
        // Type analysis failed
        Ooops("Type analysis for $1 failed", source);
        record(compiler_unit, "Type analysis for %t failed", source);
        return xl_identity;
    }

    CompilerEval function(*this, source, types);
    Value_p global = function.Function();
    Global(source, global);

    Value_p returned = function.Compile(source, true);
    if (!returned)
    {
        Ooops("Compilation for $1 failed", source);
        record(compiler_unit, "Compilation for %t failed", source);
        return xl_identity;
    }

    eval_fn result = function.Finalize(true);
    record(compiler_unit, "Compilation of %t returned %p",
           source, (void *) result);
    return result;
}


bool CompilerUnit::TypeAnalysis()
// ----------------------------------------------------------------------------
//   Perform type analysis on the given tree
// ----------------------------------------------------------------------------
{
    return types->TypeAnalysis(source) != nullptr;
}


Value_p CompilerUnit::Global(Tree *tree)
// ----------------------------------------------------------------------------
//    Return the LLVM value associated with the tree
// ----------------------------------------------------------------------------
{
    auto it = globals.find(tree);
    if (it != globals.end())
        return it->second;
    return nullptr;
}


void CompilerUnit::Global(Tree *tree, Value_p value)
// ----------------------------------------------------------------------------
//    Record the global value associated to a tree
// ----------------------------------------------------------------------------
{
    globals[tree] = value;
}


Function_p &CompilerUnit::Compiled(Scope *scope,
                                   RewriteCandidate *rc,
                                   const Values &args)
// ----------------------------------------------------------------------------
//    Return a unique entry corresponding to this overload
// ----------------------------------------------------------------------------
{
    // Build a unique key to check if we already have the function in cache
    std::ostringstream os;
    os << (void *) rc->rewrite << "@" << (void *) scope;
    for (auto value : args)
    {
        Type_p type = JIT::Type(value);
        os << '|' << (void *) type;
    }
    text key = os.str();

    return compiled[key];
}


Function_p &CompilerUnit::CompiledUnbox(Type_p type)
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


Function_p &CompilerUnit::CompiledClosure(Scope *scope, Tree *expr)
// ----------------------------------------------------------------------------
//    Return a unique function entry for the closure function
// ----------------------------------------------------------------------------
{
    std::ostringstream os;
    os << "Closure" << (void *) expr << "@" << (void *) scope;
    text key = os.str();
    return compiled[key];
}


bool CompilerUnit::IsClosureType(Type_p type)
// ----------------------------------------------------------------------------
//    Check if this is a known closure type
// ----------------------------------------------------------------------------
{
    return clotypes.count(type) > 0;
}


void CompilerUnit::AddClosureType(Type_p type)
// ----------------------------------------------------------------------------
//   Mark the type as a closure type
// ----------------------------------------------------------------------------
{
    clotypes.insert(type);
}

XL_END
