#ifndef COMPILER_FAST_H
#define COMPILER_FAST_H
// ****************************************************************************
//  compiler-fast.h                                                  XL project
// ****************************************************************************
//
//   File Description:
//
//     "Fast" compiler, used for O1 code generation.
//
//     This compiler has no type inference and no boxing/unboxing.
//     In other words, trees are represented at run-time exactly as they
//     are represented at compile-time.
//
//     This compiler is the one used by Tao3D
//
//
// ****************************************************************************
//   (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
// ****************************************************************************
//   This file is part of XL.
//
//   XL is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Foobar is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with XL.  If not, see <https://www.gnu.org/licenses/>.
// ****************************************************************************

#include "compiler.h"
#include "llvm-crap.h"


XL_BEGIN
// ============================================================================
//
//    A "fast" compiler that uses the "fast" compiled unit
//
// ============================================================================

struct O1CompileUnit;
typedef Tree * (*adapter_fn) (eval_fn, Scope *src, Tree *self, Tree **args);
typedef std::map<Tree *, JIT::Value_p>  value_map;
typedef std::set<Tree *>                data_set;
typedef std::map<Name_p, Tree_p>        captures;       // Symbol capture table
typedef std::map<text, eval_fn>         call_map;       // Pre-compiled calls
typedef std::map<uint, adapter_fn>      adapter_map;    // Array adapters
typedef std::map<uint, eval_fn>         closure_map;    // Closure adapters

struct FastCompilerInfo;
struct CompileAction;


struct FastCompiler : Compiler
// ----------------------------------------------------------------------------
//   Interface for the fast compiler
// ----------------------------------------------------------------------------
{
    FastCompiler(kstring name, unsigned opts, int argc, char **argv);
    ~FastCompiler();

    // Interpreter interface
    Tree *                      Evaluate(Scope *,
                                         Tree *source) override;
    Tree *                      TypeCheck(Scope *,
                                          Tree *type,
                                          Tree *val) override;

    // Interface formerly in struct Symbol
    Tree *                      Compile(Scope *scope,
                                        Tree *s, O1CompileUnit &,
                                        bool nullIfBad = false,
                                        bool keepOtherConstants = false,
                                        bool noDataForms = false);
    eval_fn                     CompileAll(Scope *scope,
                                           Tree *s,
                                           bool nullIfBad = false,
                                           bool keepAlternatives = true,
                                           bool noDataForms = false);
    Tree *                      CompileCall(Scope *scope,
                                            text callee,
                                            TreeList &args,
                                            bool call=true);
    adapter_fn                  ArrayToArgsAdapter(uint numtrees);
    eval_fn                     ClosureAdapter(uint numtrees);

    static FastCompilerInfo *   Info(Tree *tree, bool create = false);
    static JIT::Function_p      TreeFunction(Tree *tree);
    static void                 SetTreeFunction(Tree *tree,
                                                JIT::Function_p function);
    static JIT::Function_p      TreeClosure(Tree *tree);
    static void                 SetTreeClosure(Tree *tree,
                                               JIT::Function_p closure);
    static eval_fn              TreeCode(Tree *tree);
    static void                 SetTreeCode(Tree *tree, eval_fn code);

private:
    call_map                    calls;
    adapter_map                 adapters;
    closure_map                 closures;
};



// ============================================================================
//
//    Old compilation unit
//
// ============================================================================

struct O1CompileUnit
// ----------------------------------------------------------------------------
//  A compilation unit, which typically corresponds to an expression
// ----------------------------------------------------------------------------
{
    O1CompileUnit(FastCompiler &,
                  Scope *, Tree *source,
                  TreeList parms, bool closure);
    ~O1CompileUnit();

    bool                IsForwardCall()         { return entrybb == nullptr; }
    eval_fn             Finalize(bool topLevel);

    enum { knowAll = -1, knowLocals = 1, knowValues = 2 };

    JIT::Value_p        NeedStorage(Tree *tree, Tree *source = nullptr);
    bool                IsKnown(Tree *tree, uint which = knowAll);
    JIT::Value_p        Known(Tree *tree, uint which = knowAll );

    // Return the address of a pointer to the tree
    JIT::Constant_p     ConstantInteger(Integer *what);
    JIT::Constant_p     ConstantReal(Real *what);
    JIT::Constant_p     ConstantText(Text *what);
    JIT::Constant_p     ConstantTree(Tree *what);

    JIT::Value_p        NeedLazy(Tree *subexpr, bool allocate = true);
    JIT::Value_p        MarkComputed(Tree *subexpr, JIT::Value_p value);
    JIT::BasicBlock_p   BeginLazy(Tree *subexpr);
    void                EndLazy(Tree *subexpr, JIT::BasicBlock_p skip);

    JIT::BasicBlock_p   NeedTest();
    JIT::Value_p        Left(Tree *);
    JIT::Value_p        Right(Tree *);
    JIT::Value_p        Copy(Tree *src, Tree *dst, bool markDone=true);
    JIT::Value_p        Invoke(Tree *subexpr, Tree *callee, TreeList args);
    JIT::Value_p        CallEvaluate(Tree *);
    JIT::Value_p        CallFillBlock(Block *);
    JIT::Value_p        CallFillPrefix(Prefix *);
    JIT::Value_p        CallFillPostfix(Postfix *);
    JIT::Value_p        CallFillInfix(Infix *);
    JIT::Value_p        CallInteger2Real(Tree *cast, Tree *integer);
    JIT::Value_p        CallArrayIndex(Tree *self, Tree *l, Tree *r);
    JIT::Value_p        CreateClosure(Tree *callee,
                                      TreeList &parms,
                                      TreeList &args,
                                      JIT::Function_p);
    JIT::Value_p         CallTypeError(Tree *what);

    JIT::BasicBlock_p   TagTest(Tree *code, unsigned tag);
    JIT::BasicBlock_p   IntegerTest(Tree *code, longlong value);
    JIT::BasicBlock_p   RealTest(Tree *code, double value);
    JIT::BasicBlock_p   TextTest(Tree *code, text value);
    JIT::BasicBlock_p   ShapeTest(Tree *code, Tree *other);
    JIT::BasicBlock_p   InfixMatchTest(Tree *code, Infix *ref);
    JIT::BasicBlock_p   TypeTest(Tree *code, Tree *type);

public:
    FastCompiler &      compiler;       // The compiler environment we use
    Context             symbols;        // The symbols for this compilation unit
    Tree_p              source;         // The original source we compile

    JIT::Function_p     function;       // Function we generate
    JITBlock            code;           // Instruction builder for code
    JITBlock            data;           // Instruction builder for data

    JIT::BasicBlock_p   entrybb;        // Entry point for that code
    JIT::BasicBlock_p   exitbb;         // Exit point for that code
    JIT::BasicBlock_p   failbb;         // Where we go if tests fail
    JIT::Value_p        scopePtr;       // Storage for scope pointer

    value_map           value;          // Map tree -> LLVM value
    value_map           storage;        // Map tree -> LLVM alloca space
    value_map           computed;       // Map tree -> LLVM "computed" flag
    data_set            dataForm;       // Data expressions we don't evaluate

private:
    // Import all runtime functions
#define UNARY(Name)
#define BINARY(Name)
#define CAST(Name)
#define ALIAS(Name, Arity, Original)
#define SPECIAL(Name, Arity, Code)
#define EXTERNAL(Name, RetTy, ...)      JIT::Function_p  Name;
#include "compiler-primitives.tbl"
 };


struct ExpressionReduction
// ----------------------------------------------------------------------------
//   Record compilation state around a specific expression reduction
// ----------------------------------------------------------------------------
{
    ExpressionReduction(CompileAction &compile, Tree *source);
    ~ExpressionReduction();

    void                NewForm();
    void                Succeeded();
    void                Failed();

public:
    CompileAction &     compile;         // Compile action for this expression
    Tree *              source;         // Tree we build (mostly for debugging)

    JIT::Value_p        storage;        // Storage for expression value
    JIT::Value_p        computed;       // Flag telling if value was computed

    JIT::BasicBlock_p   savedfailbb;    // Saved location of failbb

    JIT::BasicBlock_p   entrybb;        // Entry point to subcase
    JIT::BasicBlock_p   savedbb;        // Saved position before subcase
    JIT::BasicBlock_p   successbb;      // Successful completion of expression

    value_map           savedvalue;     // Saved compile unit value map

    Tree_p              returnType;     // The return type for the expression
    unsigned            matches;        // Number of forms that matched
};



// ============================================================================
//
//    Compilation actions
//
// ============================================================================

struct DeclarationAction
// ----------------------------------------------------------------------------
//   Record data and rewrite declarations in the input tree
// ----------------------------------------------------------------------------
{
    DeclarationAction (Scope *scope): symbols(scope) { symbols.CreateScope(); }

    typedef Tree *value_type;
    Tree *      Do(Tree *what);
    Tree *      Do(Block *what);
    Tree *      Do(Prefix *what);
    Tree *      Do(Infix *what);

    void        EnterRewrite(Tree *defined, Tree *definition);

    Context     symbols;
};


struct ArgumentMatch
// ----------------------------------------------------------------------------
//   Check if a tree matches the form of the left of a rewrite
// ----------------------------------------------------------------------------
{
    ArgumentMatch (CompileAction &compile, Tree *test,
                   Scope *symbols, Scope *decl, bool data):
        compile(compile),
        symbols(symbols), declContext(decl), argContext(decl),
        test(test), defined(nullptr),
        parms(), args(),
        data(data)
    {
        argContext.CreateScope();
    }

    typedef Tree *value_type;
    Tree *      Do(Tree *what);
    Tree *      Do(Integer *what);
    Tree *      Do(Real *what);
    Tree *      Do(Text *what);
    Tree *      Do(Name *what);
    Tree *      Do(Prefix *what);
    Tree *      Do(Postfix *what);
    Tree *      Do(Infix *what);
    Tree *      Do(Block *what);

    // Compile a tree
    Tree *      Compile(Tree *source, bool noData);
    Tree *      CompileValue(Tree *source, bool noData);
    Tree *      CompileClosure(Tree *source);

public:
    CompileAction &compile;     // Action in which we are compiling
    Context     symbols;        // Context in which we evaluate values
    Scope_p     declContext;    // Symbols in which the rewrite was declared
    Context     argContext;     // Symbols where we declare arguments
    Tree_p      test;           // Tree we test
    Tree_p      defined;        // Tree beind defined, e.g. 'sin' in 'sin X'
    TreeList    parms;          // Formatl parameters in defined form
    TreeList    args;           // Arguments (values passed for parameters)
    bool        data;           // Is a data form
};


struct CompileAction
// ----------------------------------------------------------------------------
//   Compute the input tree in the given compiled unit
// ----------------------------------------------------------------------------
{
    CompileAction (Scope *s, O1CompileUnit &,
                   bool nullIfBad, bool keepAlt, bool noDataForms);

    typedef Tree *value_type;
    Tree *      Do(Tree *what);
    Tree *      Do(Integer *what);
    Tree *      Do(Real *what);
    Tree *      Do(Text *what);
    Tree *      Do(Name *what);
    Tree *      Do(Prefix *what);
    Tree *      Do(Postfix *what);
    Tree *      Do(Infix *what);
    Tree *      Do(Block *what);

    // Evaluation of names is lazy, except in sequences where it's forced
    Tree *      Do(Name *what, bool forceEval);

    // Build code selecting among rewrites in current context
    Tree *      Rewrites(Tree *what);

    // Build code for children
    Tree *      RewriteChildren(Tree *what);

    // Compile a rewrite
    Tree *      CompileRewrite(Scope *, Tree *what,
                               TreeList &parms, TreeList &args);

public:
    Context       symbols;
    O1CompileUnit &unit;
    bool          nullIfBad;
    bool          keepAlternatives;
    bool          noDataForms;
    char          debugRewrites;
};


struct EnvironmentScan
// ----------------------------------------------------------------------------
//   Collect variables in the tree that are imported from environment
// ----------------------------------------------------------------------------
{
    EnvironmentScan (Scope *s): symbols(s) {}

    typedef Tree *value_type;
    Tree *      Do(Tree *what);
    Tree *      Do(Name *what);
    Tree *      Do(Prefix *what);
    Tree *      Do(Postfix *what);
    Tree *      Do(Infix *what);
    Tree *      Do(Block *what);

public:
    Context     symbols;        // Symbols in which we test
    captures    captured;       // Captured symbols
};


struct EvaluateChildren
// ----------------------------------------------------------------------------
//   Build a clone of a tree, evaluating its children
// ----------------------------------------------------------------------------
{
    EvaluateChildren(CompileAction &compile): compile(compile) {}
    ~EvaluateChildren()                         {}

    typedef Tree *value_type;
    Tree *Do(Integer *what);
    Tree *Do(Real *what);
    Tree *Do(Text *what);
    Tree *Do(Name *what);
    Tree *Do(Prefix *what);
    Tree *Do(Postfix *what);
    Tree *Do(Infix *what);
    Tree *Do(Block *what);

public:
    CompileAction &compile;
};
XL_END

#endif // COMPILER_FAST_H
