#ifndef INTERPRETER_H
#define INTERPRETER_H
// *****************************************************************************
// interpreter.h                                                      XL project
// *****************************************************************************
//
// File description:
//
//     A fully interpreted mode for XL, that does not rely on LLVM at all
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2015-2020, Christophe de Dinechin <christophe@dinechin.org>
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

#include "tree.h"
#include "context.h"
#include "evaluator.h"


XL_BEGIN

struct Opcode;
struct Bindings;
struct Native;
struct EvaluationCache;
typedef std::vector<Rewrite_p>          RewriteList;


class Interpreter : public Evaluator
// ----------------------------------------------------------------------------
//   Base class for all ways to evaluate an XL tree
// ----------------------------------------------------------------------------
{
public:
    Interpreter();
    virtual ~Interpreter();

    Tree *Evaluate(Scope *, Tree *source) override;
    Tree *TypeCheck(Scope *, Tree *type, Tree *value) override;

    // Evaluate all closures to get the final result
    static Tree *Unwrap(Tree *tree, EvaluationCache &cache);

public:
    enum Evaluation
    {
        SEQUENCE,               // Sequence of statements - Process declarations
        STATEMENT,              // Statements in a sequence
        EXPRESSION,             // Evaluate sub-expressions in a statement
        MAYFAIL,                // Return nullptr on error
        VARIABLE,               // Return a variable reference
        LOCAL,                  // Don't lookup parent scopes
        NAMED,                  // Only lookup names
        EVALUATION_MODES        // Number of evaluations modes
    };
    static Tree *DoEvaluate(Scope *scope,
                            Tree *expr,
                            Evaluation mode,
                            EvaluationCache &cache);
    static Tree *DoTypeCheck(Scope *scope,
                             Tree *type,
                             Tree *value,
                             EvaluationCache &cache);
    static bool DoInitializers(Initializers &inits,
                               EvaluationCache &cache);

    typedef Tree *(*builtin_fn)(Bindings &bindings);
    static std::map<text, builtin_fn> builtins;
    static std::map<text, Native *>   natives;

    static void InitializeBuiltins();
    static void InitializeContext(Context &context);
};


struct EvaluationCache
// ----------------------------------------------------------------------------
//   Ensure that a given expression is only evaluated once for a given pattern
// ----------------------------------------------------------------------------
{
    Tree *Cached(Tree *expr)
    {
        while (expr)
        {
            auto found = values.find(expr);
            if (found != values.end())
                return (*found).second;
            if (Block *block = expr->AsBlock())
                expr = block->child;
            else
                expr = nullptr;
        }
        return nullptr;
    }

    Tree *Cache(Tree *expr, Tree *val)
    {
        return values[expr] = val;
    }

    Tree *CachedTypeCheck(Tree *type, Tree *expr)
    {
        auto found = types.find(std::make_pair(type,expr));
        if (found != types.end())
            return (*found).second;
        return nullptr;
    }

    void TypeCheck(Tree *type, Tree *expr, Tree *cast)
    {
        types[std::make_pair(type,expr)] = cast;
    }

private:
    std::map<Tree_p, Tree_p>                    values;
    std::map<std::pair<Tree_p,Tree_p>, Tree_p>  types;
};


struct Bindings
// ----------------------------------------------------------------------------
//   Structure used to record bindings
// ----------------------------------------------------------------------------
{
    typedef Tree *value_type;

    Bindings(Scope *evalScope, Scope *declScope,
             Tree *expr, EvaluationCache &cache)
        : evalContext(evalScope),
          declContext(declScope),
          argContext(&declContext, expr->Position()),
          self(expr),
          test(expr),
          cache(cache),
          type(nullptr),
          bindings()
    {}

    // Tree::Do interface, build the matched value
    Tree *Do(Natural *what);
    Tree *Do(Real *what);
    Tree *Do(Text *what);
    Tree *Do(Name *what);
    Tree *Do(Prefix *what);
    Tree *Do(Postfix *what);
    Tree *Do(Infix *what);
    Tree *Do(Block *what);

    // Evaluation and binding of values
    void  StripBlocks();
    bool  MustEvaluate();
    Tree *Evaluate(Scope *scope, Tree *expr, bool named = false);
    Tree *EvaluateType(Tree *type);
    Tree *EvaluateGuard(Tree *guard);
    Tree *TypeCheck(Scope *scope, Tree *type, Tree *expr);
    Tree *ResultTypeCheck(Tree *result, bool special = false);
    Tree *ResultType()                  { return type; }

    Tree *Bind(Name *name, Tree *value);
    Rewrite *Binding(unsigned n)        { return bindings[n]; }
    Tree *Argument(unsigned n, bool unwrap = true);
    Tree *Unevaluated(unsigned n)       { return Argument(n, false); }
    Tree *NamedTree(unsigned n);
    RewriteList &Rewrites()             { return bindings; }
    size_t Size()                       { return bindings.size(); }
    Tree *operator[](unsigned n)        { return Argument(n); }

    // Return the local bindings
    Scope *EvaluationScope()            { return evalContext.Symbols(); }
    Scope *DeclarationScope()           { return declContext.Symbols(); }
    Scope *ArgumentsScope()             { return argContext.Symbols(); }
    Tree  *Self()                       { return self; }
    Tree  *Enclose(Tree *val)           { return argContext.Enclose(val); }

private:
    Context             evalContext;
    Context             declContext;
    Context             argContext;
    Tree_p              self;
    Tree_p              test;
    EvaluationCache &   cache;

    // Output of the process
    Tree_p              defined;
    Tree_p              type;
    RewriteList         bindings;
};


inline void Bindings::StripBlocks()
// ----------------------------------------------------------------------------
//   Strip all blocks in a test value
// ----------------------------------------------------------------------------
{
    while (Block *b = test->AsBlock())
        test = b->child;
}

XL_END

RECORDER_DECLARE(interpreter);
RECORDER_DECLARE(eval);
RECORDER_DECLARE(bindings);
RECORDER_DECLARE(typecheck);

#endif // INTERPRETER_H
