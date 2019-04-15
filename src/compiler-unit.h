#ifndef COMPILER_UNIT_H
#define COMPILER_UNIT_H
// *****************************************************************************
// compiler-unit.h                                                    XL project
// *****************************************************************************
//
// File description:
//
//     Information about a single compilation unit, i.e. the code generated
//     for a particular tree rerite.
//
//
//
//
//
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

#include "compiler.h"
#include "compiler-args.h"
#include "llvm-crap.h"
#include "types.h"
#include <map>

XL_BEGIN

typedef std::map<Tree *, Value_p>  value_map;
typedef std::map<text, Function_p> compiled_map;
typedef std::set<Type_p>           closure_set;

class CompilerUnit
// ----------------------------------------------------------------------------
//   A unit of compilation, roughly similar to a 'module' in LLVM
// ----------------------------------------------------------------------------
{
    Compiler &          compiler;       // The compiler environment we use
    JIT &               jit;            // The JIT compiler (LLVM CRAP)
    JITModule           module;         // The module we are compiling
    Context_p           context;        // Context in which we compile
    Tree_p              source;         // The source of the program to compile
    Types_p             types;          // Type inferences for this unit
    value_map           globals;        // Global definitions in the unit
    compiled_map        compiled;       // Already compiled functions
    closure_set         clotypes;       // Closure types

    friend class        CompilerPrototype;
    friend class        CompilerFunction;
    friend class        CompilerEval;
    friend class        CompilerExpression;

public:
    CompilerUnit(Compiler &compiler, Scope *scope, Tree *source);
    ~CompilerUnit();

public:
    // Top-level compilation for the whole unit
    eval_fn             Compile();
    bool                TypeAnalysis();

    // Global values (defined at the unit level)
    Value_p             Global(Tree *tree);
    void                Global(Tree *tree, Value_p value);

    // Cache of already compiled functions
    Function_p &        Compiled(Scope *, RewriteCandidate *, const Values &);
    Function_p &        CompiledUnbox(Type_p type);
    Function_p &        CompiledClosure(Scope *, Tree *expr);

    // Closure types management
    bool                IsClosureType(Type_p type);
    void                AddClosureType(Type_p type);

private:
    // Import all runtime functions
#define UNARY(Name)
#define BINARY(Name)
#define CAST(Name)
#define ALIAS(Name, Arity, Original)
#define SPECIAL(Name, Arity, Code)
#define EXTERNAL(Name, RetTy, ...)      Function_p  Name;
#include "compiler-primitives.tbl"
};

XL_END

RECORDER_DECLARE(compiler_unit);

#endif // COMPILER_UNIT_H
