#ifndef COMPILER_UNIT_H
#define COMPILER_UNIT_H
// ****************************************************************************
//  compiler-unit.h                                                XL project
// ****************************************************************************
//
//   File Description:
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

#include "compiler.h"
#include "compiler-args.h"
#include "compiler-parms.h"
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
    ModuleID            module;         // The module we are compiling
    Context_p           context;        // Context in which we compile
    Tree_p              source;         // The source of the program to compile
    Types_p             types;          // Type inferences for this unit
    value_map           globals;        // Global definitions in the unit
    compiled_map        compiled;       // Already compiled functions
    closure_set         clotypes;       // Closure types

    friend class        CompilerFunction;
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
