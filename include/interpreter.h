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

public:
    static Tree *       EvaluateClosure(Context *context, Tree *code);
    static Tree *       Instructions(Context_p context, Tree_p what);

public:
    static Tree *       IsClosure(Tree *value, Context_p *scope);
    static Tree *       MakeClosure(Context *context, Tree *value);

    static Opcode *     SetInfo(Infix *decl, Opcode *opcode);
    static Opcode *     OpcodeInfo(Infix *decl);
};


// ============================================================================
//
//    Closure management (keeping scoping information with values)
//
// ============================================================================

struct ClosureInfo : Info
// ----------------------------------------------------------------------------
//   Mark a given Prefix as a closure
// ----------------------------------------------------------------------------
{};


inline Tree *Interpreter::IsClosure(Tree *tree, Context_p *context)
// ----------------------------------------------------------------------------
//   Check if something is a closure, if so set scope and/or context
// ----------------------------------------------------------------------------
{
    if (Scope *closure = tree->AsPrefix())
    {
        if (Scope *scope = Enclosing(closure))
        {
            if (closure->GetInfo<ClosureInfo>())
            {
                // We normally have a scope on the left
                if (context)
                    *context = new Context(scope);
                return closure->right;
            }
        }
    }
    return nullptr;
}


inline Tree *Interpreter::MakeClosure(Context *ctx, Tree *value)
// ----------------------------------------------------------------------------
//   Create a closure encapsulating the current context
// ----------------------------------------------------------------------------
{
    Context_p context = ctx;

retry:
    kind valueKind = value->Kind();

    if (valueKind >= NAME || context->HasRewritesFor(valueKind))
    {
        if (valueKind == NAME)
        {
            if (Tree *bound = context->Bound(value))
            {
                if (Tree *inside = IsClosure(bound, &context))
                {
                    if (value != inside)
                    {
                        value = inside;
                        goto retry;
                    }
                }
                if (value != bound)
                {
                    value = bound;
                    goto retry;
                }
            }
        }

        if (valueKind != PREFIX || !value->GetInfo<ClosureInfo>())
        {
            Scope *scope = context->Symbols();
            value = new Prefix(scope, value);

            ClosureInfo *closureMarker = new ClosureInfo;
            value->SetInfo(closureMarker);
        }
    }
    return value;
}

XL_END

RECORDER_DECLARE(interpreter);
RECORDER_DECLARE(interpreter_typecheck);

#endif // INTERPRETER_H
