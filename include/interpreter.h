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
typedef std::map<Tree_p, Tree_p>        EvaluationCache;


class Interpreter : public Evaluator
// ----------------------------------------------------------------------------
//   Base class for all ways to evaluate an XL tree
// ----------------------------------------------------------------------------
{
public:
    Interpreter();
    virtual ~Interpreter();

    Tree *              Evaluate(Scope *, Tree *source) override;
    Tree *              TypeCheck(Scope *, Tree *type, Tree *value) override;

    typedef Tree *(*builtin_fn)(Bindings &bindings);
    static std::map<text, builtin_fn> builtins;
    static void Initialize();
};


struct Bindings
// ----------------------------------------------------------------------------
//   Structure used to record bindings
// ----------------------------------------------------------------------------
{
    typedef bool value_type;

    Bindings(Scope *evalScope, Scope *declScope,
             Tree *expr, EvaluationCache &cache)
        : evalContext(evalScope),
          declContext(declScope),
          argContext(&evalContext, test->Position()),
          self(expr),
          test(expr),
          cache(cache),
          type(nullptr),
          arguments()
    {}

    // Tree::Do interface
    bool  Do(Natural *what);
    bool  Do(Real *what);
    bool  Do(Text *what);
    bool  Do(Name *what);
    bool  Do(Prefix *what);
    bool  Do(Postfix *what);
    bool  Do(Infix *what);
    bool  Do(Block *what);

    // Evaluation and binding of values
    bool  Evaluate();
    Tree *EvaluateType(Tree *type);
    Tree *EvaluateGuard(Tree *guard);
    void  Bind(Name *name, Tree *value);

    // Return the local bindings
    Scope *EvaluationScope()    { return evalContext.Symbols(); }
    Scope *DeclarationScope()   { return declContext.Symbols(); }
    Scope *ArgumentsScope()     { return argContext.Symbols(); }
    Tree  *Self()               { return self; }

private:
    Context             evalContext;
    Context             declContext;
    Context             argContext;
    Tree_p              self;
    Tree_p              test;
    EvaluationCache &   cache;

public:
    Tree_p              type;
    TreeList            arguments;
};

XL_END

RECORDER_DECLARE(interpreter);
RECORDER_DECLARE(eval);
RECORDER_DECLARE(bindings);
RECORDER_DECLARE(typecheck);

#endif // INTERPRETER_H
