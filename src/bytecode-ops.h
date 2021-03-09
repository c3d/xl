// ****************************************************************************
//  bytecode-ops.h                                                   XL project
// ****************************************************************************
//
//   File Description:
//
//     Definition of the bytecodes used in bytecode.cpp
//
//     Put them in a separate file mostly for convenience:
//     This is not a regular header file to be included by multiple sources.
//
//
//
//
//
// ****************************************************************************
//   (C) 2021 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
// ****************************************************************************
//   This file is part of XL.
//
//   XL is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   XL is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with XL.  If not, see <https://www.gnu.org/licenses/>.
// ****************************************************************************


// ============================================================================
//
//   Opcodes
//
// ============================================================================

static void evaluate(RunState &state)
// ----------------------------------------------------------------------------
//   Evaluate the top of the stack
// ----------------------------------------------------------------------------
{
    Tree_p expr = state.Pop();
    Scope_p scope = state.EvaluationScope();
    Tree_p result = evaluate(scope, expr);
    state.Push(result);
}


static void transfer(RunState &state)
// ----------------------------------------------------------------------------
//   Transfer evaluation to the item at the top of stack
// ----------------------------------------------------------------------------
{
    Tree_p target = state.Top();
    Bytecode *bytecode = target ? target->GetInfo<Bytecode>() : nullptr;
    state.transfer = bytecode;
    state.pc = ~0U;
};


static void cast(RunState &state)
// ----------------------------------------------------------------------------
//   Cast the value to a static type
// ----------------------------------------------------------------------------
{
    Tree_p value = state.Pop();
    Tree_p type = state.Constant();
    Tree_p result = typecheck(state.scope, type, value);
    state.Push(result);
}



static void error(RunState &state)
// ----------------------------------------------------------------------------
//   Emit an error with the given arguments
// ----------------------------------------------------------------------------
{
    Tree_p error = state.Constant();
    state.Push(error);
    evaluate(state);
}


static void exit(RunState &state)
// ----------------------------------------------------------------------------
//   Terminate execution
// ----------------------------------------------------------------------------
{
    state.pc |= ~0ULL;
    state.bytecode = nullptr;
}


static void check_statement(RunState &state)
// ----------------------------------------------------------------------------
//   If we have an error on top of stack, exit, otherwise check we have nil
// ----------------------------------------------------------------------------
{
    Tree_p check = state.Pop();
    if (IsError(check))
        exit(state);
    if (check)
        record(opcode_error, "Non-nil result %t for a statement", check);
}


static void branch(RunState &state)
// ----------------------------------------------------------------------------
//   Terminate execution
// ----------------------------------------------------------------------------
{
    state.pc = state.Jump();
}


static void error_exit(RunState &state)
// ----------------------------------------------------------------------------
//   Emit an error with the given arguments
// ----------------------------------------------------------------------------
{
    error(state);
    exit(state);
}


static void constant(RunState &state)
// ----------------------------------------------------------------------------
//   Deposit a constant on the stack
// ----------------------------------------------------------------------------
{
    Tree_p value = state.Constant();
    state.Push(value);
}


static void dup(RunState &state)
// ----------------------------------------------------------------------------
//   Duplicate the top of stack
// ----------------------------------------------------------------------------
{
    state.Push(state.Top());
}


static void drop(RunState &state)
// ----------------------------------------------------------------------------
//   Drop the top of the stack
// ----------------------------------------------------------------------------
{
    state.Pop();
}


static void swap(RunState &state)
// ----------------------------------------------------------------------------
//   Swap the two top levels of the stack
// ----------------------------------------------------------------------------
{
    Tree_p x = state.Pop();
    Tree_p y = state.Pop();
    state.Push(x);
    state.Push(y);
}


static void make_variable(RunState &state)
// ----------------------------------------------------------------------------
//   Turn something into a variable type
// ----------------------------------------------------------------------------
{
    Tree_p type = state.Top();
    if (!IsVariableType(type))
    {
        type = new Prefix(xl_variable, type, type->Position());
        state.Set(type);
    }
}


static void make_matching(RunState &state)
// ----------------------------------------------------------------------------
//   Turn something into a pattern matching type
// ----------------------------------------------------------------------------
{
    Tree_p type = state.Top();
    if (!IsPatternMatchingType(type))
    {
        type = new Prefix(xl_matching, type, type->Position());
        state.Set(type);
    }
}


static void get_scope(RunState &state)
// ----------------------------------------------------------------------------
//   Push the current scope on the stack
// ----------------------------------------------------------------------------
{
    state.Push(state.EvaluationScope());
}


static void get_super(RunState &state)
// ----------------------------------------------------------------------------
//   Push the enclosing scope on the stack
// ----------------------------------------------------------------------------
{
    state.Push(state.EvaluationScope()->Enclosing());
}


static void get_self(RunState &state)
// ----------------------------------------------------------------------------
//   Push self on the state
// ----------------------------------------------------------------------------
{
    state.Push(state.Self());
}


static void set_scope(RunState &state)
// ----------------------------------------------------------------------------
//   Set the scope to what is on the stack
// ----------------------------------------------------------------------------
{
    Tree_p tree = state.Pop();
    Scope_p scope = tree->As<Scope>();
    if (!scope)
        record(opcode_error, "set_scope received non-scope %t", tree);
    state.scope = scope;
}


static void enter(RunState &state)
// ----------------------------------------------------------------------------
//   Enter a nested scope
// ----------------------------------------------------------------------------
{
    Context context(state.scope);
    state.scope = context.CreateScope();
    state.locals = state.frame;
}


static void call(RunState &state)
// ----------------------------------------------------------------------------
//    Call the top of stack
// ----------------------------------------------------------------------------
{
    size_t size = state.FrameSize();
    XL_ASSERT(size <= state.stack.size());

    // Save bytecode, program counter and current frame
    Bytecode *saveBC = state.bytecode;
    opaddr_t  savePC = state.pc;
    opaddr_t  locals = state.locals;

    // Get bytecode for what we want to call
    Tree_p callee = state.Pop();
    Bytecode *bytecode = compile(state.EvaluationScope(), callee);

    // Set the frame for the callee
    state.frame = state.stack.size();
    state.locals = state.frame - size;

    // Run the target code
    bytecode->Run(state);

    // Restore caller's state
    state.pc = savePC;
    state.bytecode = saveBC;
    state.locals = locals;
    state.frame = locals + 1;

    // If callee did not consume its arguments, cleanup
    auto &stack = state.stack;
    Tree_p result = state.Pop();
    stack.erase(stack.begin() + state.frame, stack.end());
    state.Push(result);
}


static void assign(RunState &state)
// ----------------------------------------------------------------------------
//   Perform an assignment
// ----------------------------------------------------------------------------
{
    Tree_p value = state.Pop();
    Tree_p variable = state.Pop();
    Rewrite_p rewrite = variable->As<Rewrite>();
    if (rewrite)
    {
        rewrite->right = value;
    }
    else
    {
        Error error("Cannot assign to non-variable $1", variable);
        state.Error(error);
    }
}


static void local(RunState &state)
// ----------------------------------------------------------------------------
//   Fetch a local value from the state and put it on top of stack
// ----------------------------------------------------------------------------
{
    Tree_p value = state.Local();
    state.Push(value);
}


static void store(RunState &state)
// ----------------------------------------------------------------------------
//   Save top of stack to a new local
// ----------------------------------------------------------------------------
{
    Tree_p value = state.Top();
    state.Save(value);
}


static void load(RunState &state)
// ----------------------------------------------------------------------------
//   Save top of stack to a new local
// ----------------------------------------------------------------------------
{
    Tree_p value = state.Saved();
    state.Push(value);
}


static void init_value(RunState &state)
// ----------------------------------------------------------------------------
//   Initialize a named constant
// ----------------------------------------------------------------------------
{
    Rewrite_p rewrite = state.Rewrite();
    Tree_p value = state.Pop();
    rewrite->right = value;
}


// ============================================================================
//
//    Checks - Branch to target in case of failure
//
// ============================================================================

static void check_type(RunState &state)
// ----------------------------------------------------------------------------
//   Check a type at runtime
// ----------------------------------------------------------------------------
{
    Tree_p type = state.Constant();
    opaddr_t target = state.Jump();
    Tree_p value = state.Pop();
    Tree_p result = typecheck(state.scope, type, value);
    if (result)
    {
        state.Push(result);
        return;
    }
    state.pc = target;
}


static void check_input_type(RunState &state)
// ----------------------------------------------------------------------------
//   Check the type of an input argument
// ----------------------------------------------------------------------------
{
    check_type(state);
}


static void check_result_type(RunState &state)
// ----------------------------------------------------------------------------
//   Check the type of an argument
// ----------------------------------------------------------------------------
{
    check_type(state);
}


static void check_init_type(RunState &state)
// ----------------------------------------------------------------------------
//   Check the type of an initializer
// ----------------------------------------------------------------------------
{
    check_type(state);
}


static void check_typecast(RunState &state)
// ----------------------------------------------------------------------------
//   Evaluate a type cast, i.e. [X as Y]
// ----------------------------------------------------------------------------
{
    check_type(state);
}


static void check_guard(RunState &state)
// ----------------------------------------------------------------------------
//   Check if the guard condition is true, or nullify the stack
// ----------------------------------------------------------------------------
{
    opaddr_t target = state.Jump();
    Tree *condition = state.Pop();
    if (condition == xl_true)
        return;
    state.pc = target;
}


static void check_same(RunState &state)
// ----------------------------------------------------------------------------
//   Check if the two tops levels are equal, leave one copy if it matches
// ----------------------------------------------------------------------------
{
    Tree_p test = state.Pop();
    Tree_p expr = state.Top();
    opaddr_t target = state.Jump();
    if (Tree::Equal(expr, test))
        return;
    state.pc = target;
}


static void check_natural(RunState &state)
// ----------------------------------------------------------------------------
//   Check if we match a natural constant
// ----------------------------------------------------------------------------
{
    Tree *value = state.Pop();
    Natural *natural = value->AsNatural();
    Natural *match = state.Constant()->AsNatural();
    opaddr_t target = state.Jump();
    if (natural && natural->value == match->value)
        return;
    state.pc = target;
}


static void check_real(RunState &state)
// ----------------------------------------------------------------------------
//   Check if we match a real constant
// ----------------------------------------------------------------------------
{
    Tree *value = state.Pop();
    Real *real = value->AsReal();
    Real *match = state.Constant()->AsReal();
    opaddr_t target = state.Jump();
    if (real && real->value == match->value)
        return;
    Natural *nat = value->AsNatural();
    if (nat && nat->value == match->value)
        return;
    state.pc = target;
}


static void check_text(RunState &state)
// ----------------------------------------------------------------------------
//   Check if we match a text constant
// ----------------------------------------------------------------------------
{
    Tree *top = state.Top();
    Text *text = top->AsText();
    Text *match = state.Constant()->AsText();
    opaddr_t target = state.Jump();
    if (text && text->value == match->value)
        return;
    state.pc = target;
}


static void check_infix(RunState &state)
// ----------------------------------------------------------------------------
//   Check if we have an infix, and if so, split it into two components
// ----------------------------------------------------------------------------
//   We have [infix], we will have either [right, left], or [nullptr]
{
    Infix *reference = state.Constant()->AsInfix();
    Tree_p value = state.Pop();
    Infix *infix = value->AsInfix();
    opaddr_t target = state.Jump();
    if (infix && infix->name == reference->name)
        return;
    state.pc = target;
}


static void check_borrow(RunState &state)
// ----------------------------------------------------------------------------
//    Borrow a variable declaration
// ----------------------------------------------------------------------------
{
    Tree *top = state.Top();
    opaddr_t target = state.Jump();
    Rewrite *rewrite = top->As<Rewrite>();
    if (rewrite)
        return;
    state.pc = target;
}


static void check_typed_borrow(RunState &state)
// ----------------------------------------------------------------------------
//    Borrow a variable declaration and check its type
// ----------------------------------------------------------------------------
{
    Tree *top = state.Top();
    Tree *type = state.Constant();
    opaddr_t target = state.Jump();
    Rewrite *rewrite = top->As<Rewrite>();
    if (rewrite)
    {
        Infix *vartype = IsTypeAnnotation(rewrite->left);
        Tree_p have = vartype->right;
        if (Tree::Equal(type, have))
            return;
    }
    state.pc = target;
}
