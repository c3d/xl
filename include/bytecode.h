#ifndef BYTECODE_H
#define BYTECODE_H
// *****************************************************************************
// bytecode.h                                                        XL project
// *****************************************************************************
//
// File description:
//
//     A bytecode-based translation of XL programs
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
// (C) 2015-2021, Christophe de Dinechin <christophe@dinechin.org>
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


RECORDER_DECLARE(bytecode);
RECORDER_DECLARE(typecheck);
RECORDER_DECLARE(opcode);
RECORDER_DECLARE(opcode_run);
RECORDER_DECLARE(opcode_error);

XL_BEGIN

struct Bytecode;
struct RunState;

typedef size_t  opaddr_t;
typedef int     branch_t;

typedef std::vector<Rewrite_p>          RewriteList;
typedef std::vector<Name_p>             NameList;
typedef std::map<Tree_p, unsigned>      EvaluatedSet;
typedef void (*opcode_fn)(RunState &);


class BytecodeEvaluator : public Evaluator
// ----------------------------------------------------------------------------
//   Base class for all ways to evaluate an XL tree
// ----------------------------------------------------------------------------
{
public:
    BytecodeEvaluator();
    virtual ~BytecodeEvaluator();

    Tree *Evaluate(Scope *, Tree *source) override;
    Tree *TypeCheck(Scope *, Tree *type, Tree *value) override;

public:
    static std::map<text, opcode_fn>    builtins;
    static std::map<text, opcode_fn>    natives;
    static std::map<text, opcode_fn>    types;

    static void InitializeBuiltins();
    static void InitializeContext(Context &context);
};


// ============================================================================
//
//   Instruction opcodes and evaluation stack
//
// ============================================================================

struct RunState
// ----------------------------------------------------------------------------
//   The state of the program during evaluation
// ----------------------------------------------------------------------------
{
    RunState(Scope *scope, Tree *expr)
        : stack(), scope(scope), bytecode(), transfer(),
          pc(0), args(0), error()
    {
        Push(expr);
    }

    void        Push(Tree *value)       { stack.push_back(value); }
    Tree_p      Pop();
    Tree_p      Top();
    void        Set(Tree *top)          { stack.back() = top; }
    size_t      Depth()                 { return stack.size(); }
    Tree_p      Self();
    Scope_p     EvaluationScope()       { return scope; }
    Scope_p     DeclarationScope();
    opaddr_t    Jump();
    Tree *      Local();
    Tree *      Constant();
    Rewrite *   Rewrite();
    void        Error(Tree *msg)        { error = msg; }
    Tree *      Error()                 { return error; }

    typedef std::vector<size_t> Frames;

    TreeList    stack;                  // Evaluation stack and parameters
    Scope_p     scope;                  // Current evaluation scope
    Bytecode *  bytecode;               // Bytecode currently executing
    Bytecode *  transfer;               // Bytecode to transfer to
    opaddr_t    pc;                     // Program counter in the bytcode
    opaddr_t    args;                   // Number of arguments and variables
    Tree_p      error;                  // Error messages
};


inline Tree_p RunState::Pop()
// ----------------------------------------------------------------------------
//   Return the last item on the stack and remove it
// ----------------------------------------------------------------------------
{
    if (!stack.size())
    {
        record(opcode_error,
               "Popping from empty stack while evaluating %t", Self());
        return nullptr;
    }
    Tree_p result = stack.back();
    stack.pop_back();
    return result;
}


inline Tree_p RunState::Top()
// ----------------------------------------------------------------------------
//   Return the last item on the stack without removing it
// ----------------------------------------------------------------------------
{
    if (!stack.size())
    {
        record(opcode_error,
               "Getting top from empty stack while evaluating %t", Self());
        return nullptr;
    }
    return stack.back();
}

XL_END

#endif // BYTECODE_H
