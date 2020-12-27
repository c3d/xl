// *****************************************************************************
// interpreter.cpp                                                    XL project
// *****************************************************************************
//
// File description:
//
//    An interpreter for XL that does not rely on LLVM at all
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

#include "interpreter.h"
#include "gc.h"
#include "info.h"
#include "errors.h"
#include "renderer.h"
#include "builtins.h"
#include "runtime.h"
#include "save.h"

#include <cmath>
#include <algorithm>

RECORDER(interpreter,   32, "Interpreter class evaluation of XL code");
RECORDER(eval,          32, "Primary evaluation entry point");
RECORDER(bindings,      32, "Bindings");
RECORDER(typecheck,     32, "Type checks");


XL_BEGIN
// ============================================================================
//
//   Options
//
// ============================================================================

namespace Opt
{
NaturalOption   stackDepth("stack_depth",
                           "Maximum stack depth for interpreter",
                           1000, 25, 25000);
}



// ============================================================================
//
//   Static functions defined in this file
//
// ============================================================================

static Tree *evaluate(Scope *scope, Tree *value);
static Tree *typecheck(Scope *scope, Tree *type, Tree *value);
static bool  initialize();



// ============================================================================
//
//   Interpeter main entry points
//
// ============================================================================

Interpreter::Interpreter()
// ----------------------------------------------------------------------------
//   Constructor for interpreter
// ----------------------------------------------------------------------------
{
    record(interpreter, "Created interpreter %p", this);
}


Interpreter::~Interpreter()
// ----------------------------------------------------------------------------
//   Destructor for interpreter
// ----------------------------------------------------------------------------
{
    record(interpreter, "Destroyed interpreter %p", this);
}


void Interpreter::Initialize()
// ----------------------------------------------------------------------------
//    Initialize all the names, e.g. xl_nil, and all builtins
// ----------------------------------------------------------------------------
{
    initialize();
}



// ============================================================================
//
//    Parameter binding
//
// ============================================================================

inline bool Bindings::Do(Natural *what)
// ----------------------------------------------------------------------------
//   The pattern contains an natural: check we have the same
// ----------------------------------------------------------------------------
{
    Evaluate();
    if (Natural *ival = test->AsNatural())
        if (ival->value == what->value)
            return true;
    Ooops("Value $1 does not match pattern value $2", test, what);
    return false;
}


inline bool Bindings::Do(Real *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    Evaluate();
    if (Real *rval = test->AsReal())
        if (rval->value == what->value)
            return true;
    Ooops("Value $1 does not match pattern value $2", test, what);
    return false;
}


inline bool Bindings::Do(Text *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    Evaluate();
    if (Text *tval = test->AsText())
        if (tval->value == what->value)         // Do delimiters matter?
            return true;
    Ooops("Value $1 does not match pattern value $2", test, what);
    return false;
}


inline bool Bindings::Do(Name *what)
// ----------------------------------------------------------------------------
//   The pattern contains a name: bind it as a closure, no evaluation
// ----------------------------------------------------------------------------
{
    // The test value may have been evaluated. If so, use evaluated value
    auto found = cache.find(test);
    if (found != cache.end())
        test = (*found).second;

    // If there is already a binding for that name, value must match
    // This covers both a pattern with 'pi' in it and things like 'X+X'
    if (Tree *bound = argContext.Bound(what))
    {
        Evaluate();
        bool result = Tree::Equal(bound, test);
        record(interpreter, "Arg check %t vs %t: %+s",
               test, bound, result ? "match" : "failed");
        if (!result)
            Ooops("Value $1 does not match named value $2", test, what);
        return result;
    }

    // Otherwise, bind test value to name
    Bind(what, test);
    return true;
}


bool Bindings::Do(Block *what)
// ----------------------------------------------------------------------------
//   The pattern contains a block: look inside
// ----------------------------------------------------------------------------
{
    return what->child->Do(this);
}


bool Bindings::Do(Prefix *what)
// ----------------------------------------------------------------------------
//   The pattern contains prefix: check that the left part matches
// ----------------------------------------------------------------------------
{
    bool retrying = true;

    while (retrying)
    {
        retrying = false;

        // The test itself should be a prefix
        if (Prefix *pfx = test->AsPrefix())
        {
            // If we call 'sin X' and match 'sin 3', check if names match
            if (Name *name = what->left->AsName())
            {
                if (Name *testName = pfx->left->AsName())
                {
                    if (name->value == testName->value)
                    {
                        test = pfx->right;
                        return what->right->Do(this);
                    }
                    else
                    {
                        Ooops("Prefix name $1 does not match $2",
                              name, testName);
                        return false;
                    }
                }
            }

            // For other cases, we must go deep inside each prefix to check
            test = pfx->left;
            if (!what->left->Do(this))
                return false;
            test = pfx->right;
            if (what->right->Do(this))
                return true;
        }

        // Normal prefix matching failed: try to evaluate test
        if (Evaluate())
            if (test->AsPrefix())
                retrying = true;
    }

    // Mismatch
    Ooops("Prefix $1 does not match $2", what, test);
    return false;
}


bool Bindings::Do(Postfix *what)
// ----------------------------------------------------------------------------
//   The pattern contains posfix: check that the right part matches
// ----------------------------------------------------------------------------
{
    bool retrying = true;

    while (retrying)
    {
        retrying = false;

        // The test itself should be a postfix
        if (Postfix *pfx = test->AsPostfix())
        {

            // If we call 'X!' and match '3!', check if names match
            if (Name *name = what->right->AsName())
            {
                if (Name *testName = pfx->right->AsName())
                {
                    if (name->value == testName->value)
                    {
                        test = pfx->left;
                        return what->left->Do(this);
                    }
                    else
                    {
                        Ooops("Postfix name $1 does not match $2",
                              name, testName);
                        return false;
                    }
                }
            }

            // For other cases, we must go deep inside each prefix to check
            test = pfx->right;
            if (!what->right->Do(this))
                return false;
            test = pfx->left;
            if (what->left->Do(this))
                return true;
        }

        // Normal postfix matching failed: try to evaluate test
        if (Evaluate())
            if (test->AsPostfix())
                retrying = true;
    }

    // All other cases are a mismatch
    Ooops("Postfix $1 does not match $2", what, test);
    return false;
}


bool Bindings::Do(Infix *what)
// ----------------------------------------------------------------------------
//   The complicated case: various declarations
// ----------------------------------------------------------------------------
{
    // Check if we have a type annotation, like [X:natural]
    if (IsTypeAnnotation(what))
    {
        // Need to match the left part with the test
        if (!what->left->Do(this))
            return false;

        // Need to evaluate the type on the right
        Tree *want = EvaluateType(what->right);

        // Type check value against type
        Tree *checked = typecheck(EvaluationScope(), want, test);
        if (checked == xl_false || checked == xl_nil)
            checked = nullptr;
        if (!checked)
        {
            Ooops("Value $1 does not belong to type $2", test, type);
            return false;
        }
        type = want;
        return true;
    }

    // Check if we have a guard clause
    if (IsPatternCondition(what))
    {
        // It must pass the rest (need to bind values first)
        if (!what->left->Do(this))
            return false;

        // Here, we need to evaluate in the local context, not eval one
        Tree *check = EvaluateGuard(what->right);
        if (check == xl_true)
            return true;
        else if (check != xl_false)
            Ooops ("Invalid guard clause, $1 is not a boolean", check);
        else
            Ooops("Guard clause $1 is not verified", what->right);
        return false;
    }

    // In all other cases, we need an infix with matching name
    Infix *ifx = test->AsInfix();
    if (!ifx || ifx->name != what->name)
    {
        // Try to get an infix by evaluating what we have
        Evaluate();
        ifx = test->AsInfix();
    }
    if (ifx)
    {
        if (ifx->name != what->name)
        {
            Ooops("Infix names for $1 and $2 don't match", what, ifx);
            return false;
        }

        test = ifx->left;
        if (!what->left->Do(this))
            return false;
        test = ifx->right;
        if (what->right->Do(this))
            return true;
    }

    // Mismatch
    Ooops("Infix $1 does not match $2", what, test);
    return false;
}


bool Bindings::Evaluate()
// ----------------------------------------------------------------------------
//   Evaluate 'test', ensuring that each bound arg is evaluated at most once
// ----------------------------------------------------------------------------
{
    Tree *evaluated = cache[test];
    if (!evaluated)
    {
        evaluated = evaluate(EvaluationScope(), test);
        cache[test] = evaluated;
        record(bindings, "Test %t = new %t", test, evaluated);
    }
    else
    {
        record(bindings, "Test %t = old %t", test, evaluated);
    }
    bool result = test != evaluated;
    test = evaluated;
    return result;
}


Tree *Bindings::EvaluateType(Tree *type)
// ----------------------------------------------------------------------------
//   Evaluate a type expression (in declaration context)
// ----------------------------------------------------------------------------
{
    return evaluate(DeclarationScope(), type);
}


Tree *Bindings::EvaluateGuard(Tree *guard)
// ----------------------------------------------------------------------------
//   Evaluate a guard condition (in arguments context)
// ----------------------------------------------------------------------------
{
    return evaluate(ArgumentsScope(), guard);
}


void Bindings::Bind(Name *name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a new binding in the current context, remember left and right
// ----------------------------------------------------------------------------
//   We wrap the input argument in a closure to get the proper context on eval
{
    record(bindings, "Binding %t = %t", name, value);
    value = evalContext.Closure(value);
    arguments.push_back(value);
    argContext.Define(name, value);
}



// ============================================================================
//
//   Evaluation entry point
//
// ============================================================================

static Tree *evalLookup(Scope   *evalScope,
                        Scope   *declScope,
                        Tree    *expr,
                        Rewrite *decl,
                        void    *ec)
// ----------------------------------------------------------------------------
//   Bind 'self' to the pattern in 'decl', and if successful, evaluate
// ----------------------------------------------------------------------------
{
    EvaluationCache &cache = *((EvaluationCache *) ec);
    Bindings bindings(evalScope, declScope, expr, cache);
    Tree *pattern = decl->Pattern();
    if (!pattern->Do(bindings))
        return nullptr;

    // Successfully bound: evaluate the body in arguments
    Tree *body = decl->right;

    // Check if we have a builtin
    if (Prefix *builtin = IsBuiltin(body))
    {
        Name *name = builtin->right->AsName();
        if (!name)
            return Error("Malformed builtin $1", builtin);
        Interpreter::builtin_fn callback = Interpreter::builtins[name->value];
        if (!callback)
            return Error("Nonexistent builtin $1", name);
        Tree *result = callback(bindings);
        return result;
    }

    // Check if we evaluate as self
    if (IsSelf(body))
        return expr;

    // Otherwise evaluate the body in the binding arguments scope
    return evaluate(bindings.ArgumentsScope(), body);
}


static Tree *evaluate(Context &context, Tree *expr)
// ----------------------------------------------------------------------------
//   Internal evaluator - Short circuits a few common expressions
// ----------------------------------------------------------------------------
{
    Tree_p result = expr;

retry:
    // Short-circuit evaluation of blocks
    if (Block *block = expr->AsBlock())
    {
        expr = block->child;
        result = expr;
        goto retry;
    }

    // Short-circuit evaluation of sequences and declarations
    if (Prefix *prefix = expr->AsPrefix())
    {
        // Check if the left is a block containing only declarations
        // This deals with [(X is 3) (X + 1)], i.e. closures
        if (Scope *scope = context.ProcessScope(prefix->left))
            return evaluate(scope, prefix->right);

        // Filter out declaration such as [extern foo(bar)]
        if (IsDeclaration(prefix))
            return prefix;
    }

    // Short-circuit evaluation of sequences and declarations
    if (Infix *infix = expr->AsInfix())
    {
        // Process sequences, trying to avoid deep recursion
        if (IsSequence(infix))
        {
            Tree *left = evaluate(context, infix->left);
            if (left != infix->left)
                result = left;
            if (IsError(left))
                return left;
            expr = infix->right;
            goto retry;
        }

        // Skip declarations
        if (IsDeclaration(infix))
            return result;

        // Evaluate X.Y
        if (IsDot(infix))
            if (Scope *scope = context.ProcessScope(infix->left))
                return evaluate(scope->Inner(), infix->right);
    }

    // All other cases: lookup in symbol table
    static uint depth = 0;
    Save<uint>  save(depth, depth+1);
    if (depth > Opt::stackDepth)
        return Error("Stack depth exceeded evaluating $1", expr);

    EvaluationCache cache;
    if (Tree *found = context.Lookup(expr, evalLookup, &cache, true))
        result = found;
    else
        result = (Tree *) Error("No form matching $1", expr);
    return result;
}


static Tree *evaluate(Scope *scope, Tree *expr)
// ----------------------------------------------------------------------------
//   Helper evaluation function for recursive evaluation
// ----------------------------------------------------------------------------
{
    record(eval, "Evaluating %t in %t", expr, scope);
    Context context(scope);
    Tree_p result = expr;

    if (context.ProcessDeclarations(expr))
        result = evaluate(context, expr);

    record(eval, "Evaluated %t in %t as %t", expr, scope, result);
    return result;
}


static Tree *typecheck(Scope *scope, Tree *type, Tree *value)
// ----------------------------------------------------------------------------
//   Check if we pass the test for the type
// ----------------------------------------------------------------------------
{
    record(typecheck, "Check %t against %t in scope %t",
           value, type, scope);
    Tree *test = new Infix("∈", value, type, value->Position());
    Tree *checked = evaluate(scope, test);
    record(typecheck, "Checked %t against %t in scope %t, got %t",
           value, type, scope, checked);
    return checked;
}


Tree *Interpreter::Evaluate(Scope *scope, Tree *expr)
// ----------------------------------------------------------------------------
//    Evaluate 'what', finding the final, non-closure result
// ----------------------------------------------------------------------------
{
    return evaluate(scope, expr);
}


Tree *Interpreter::TypeCheck(Scope *scope, Tree *type, Tree *value)
// ----------------------------------------------------------------------------
//   Check if 'value' matches 'type' in the given context
// ----------------------------------------------------------------------------
{
    return typecheck(scope, type, value);
}



// ============================================================================
//
//    Generate the builtins
//
// ============================================================================

std::map<text, Interpreter::builtin_fn> Interpreter::builtins;

#define R_INT(x)        return new Natural((x), self->Position())
#define R_REAL(x)       return new Real((x), self->Position())
#define R_TEXT(x)       return new Text((x), self->Position())
#define R_BOOL(x)       return (x) ? xl_true : xl_false
#define LEFT            (left->value)
#define RIGHT           (right->value)
#define VALUE           (value->value)
#define LEFT_B          (LEFT == "true")
#define RIGHT_B         (RIGHT == "true")
#define VALUE_B         (VALUE == "true")
#define SIGNED          (longlong)
#define UNSIGNED        (ulonglong)
#define SLEFT           SIGNED LEFT
#define SRIGHT          SIGNED RIGHT
#define SVALUE          SIGNED VALUE
#define ULEFT           UNSIGNED LEFT
#define URIGHT          UNSIGNED RIGHT
#define UVALUE          UNSIGNED VALUE
#define DIV0            if (RIGHT == 0) return Error("Divide $1 by zero", self)

#define UNARY_OP(Name, ReturnType, ArgType, Body)               \
static Tree *builtin_unary_##Name(Bindings &bindings)           \
{                                                               \
    TreeList &args = bindings.arguments;                        \
    Tree *self = bindings.Self();                               \
    Tree *result = self;                                        \
    if (args.size() != 1)                                       \
        return Error("Invalid number of arguments "             \
                     "for unary builtin " #Name                 \
                     " in $1", result);                         \
    ArgType *value  = args[0]->As<ArgType>();                   \
    if (!value)                                                 \
        return Error("Argument $1 is not a " #ArgType           \
                     " in builtin " #Name, args[0]);            \
    Body;                                                       \
}


#define BINARY_OP(Name, ReturnType, LeftType, RightType, Body)  \
static Tree *builtin_binary_##Name(Bindings &bindings)          \
{                                                               \
    TreeList &args = bindings.arguments;                        \
    Tree *self = bindings.Self();                               \
    if (args.size() != 2)                                       \
        return Error("Invalid number of arguments "             \
                     "for binary builtin " #Name                \
                     " in $1", self);                           \
    LeftType *left  = args[0]->As<LeftType>();                  \
    if (!left)                                                  \
        return Error("First argument $1 is not a " #LeftType    \
                     " in builtin " #Name, args[0]);            \
    RightType *right = args[1]->As<RightType>();                \
    if (!right)                                                 \
        return Error("Second argument $1 is not a " #RightType  \
                     " in builtin " #Name, args[1]);            \
    Body;                                                       \
}


#define NAME(N)                                                 \
Name_p xl_##N;                                                  \
static Tree *builtin_name_##N(Bindings &bindings)               \
{                                                               \
    if (!xl_##N)                                                \
        xl_##N = new Name(#N,                                   \
                          bindings.Self()->Position());         \
    return xl_##N;                                              \
}


#define TYPE(N, Body)                                           \
Name_p N##_type;                                                \
static Tree *builtin_type_##N(Bindings &bindings)               \
{                                                               \
    if (!N##_type)                                              \
        N##_type = new Name(#N,                                 \
                               bindings.Self()->Position());    \
    return N##_type;                                            \
}                                                               \
                                                                \
static Tree *builtin_typecheck_##N(Bindings &bindings)          \
{                                                               \
    TreeList &args = bindings.arguments;                        \
    if (args.size() != 2)                                       \
        return Error("Invalid number of arguments "             \
                     "for " #N " typecheck"                     \
                     " in $1", bindings.Self());                \
    Tree *value = args[0]; (value);                             \
    Tree *type  = args[1]; (type);                              \
    Body;                                                       \
}

#include "builtins.tbl"



#define UNARY_OP(Name, ReturnType, LeftTYpe, Body)              \
Interpreter::builtins[#Name] = builtin_unary_##Name;

#define BINARY_OP(Name, ReturnType, LeftTYpe, RightType, Body)  \
Interpreter::builtins[#Name] = builtin_binary_##Name;

#define NAME(N)                                                 \
Interpreter::builtins[#N] = builtin_name_##N;                   \
xl_##N = new Name(#N);

#define TYPE(N, Body)                                           \
Interpreter::builtins[#N] = builtin_type_##N;                   \
Interpreter::builtins[#N"_check"] = builtin_typecheck_##N;      \
N##_type = new Name(#N);


static bool initialize()
// ----------------------------------------------------------------------------
//   Initialize the builtins
// ----------------------------------------------------------------------------
{
#include "builtins.tbl"
    return true;
}

XL_END
