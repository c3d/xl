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

#include "interpreter.h"
#include "gc.h"
#include "info.h"
#include "errors.h"
#include "renderer.h"
#include "builtins.h"
#include "runtime.h"
#include "save.h"
#include "native.h"

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



// ============================================================================
//
//    Parameter binding
//
// ============================================================================

Tree *Bindings::Do(Natural *what)
// ----------------------------------------------------------------------------
//   The pattern contains an natural: check we have the same
// ----------------------------------------------------------------------------
{
    StripBlocks();
    if (test->Kind() != NATURAL)
        MustEvaluate();
    if (Natural *ival = test->AsNatural())
        if (ival->value == what->value)
            return what;
    Ooops("Value $1 does not match pattern value $2", test, what);
    return nullptr;
}


Tree *Bindings::Do(Real *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    StripBlocks();
    if (test->Kind() != REAL)
        MustEvaluate();
    if (Real *rval = test->AsReal())
        if (rval->value == what->value)
            return what;
    if (Natural *nval = test->AsNatural())
        if (nval->value == what->value)
            return what;
    Ooops("Value $1 does not match pattern value $2", test, what);
    return nullptr;
}


Tree *Bindings::Do(Text *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    StripBlocks();
    if (test->Kind() != TEXT)
        MustEvaluate();
    if (Text *tval = test->AsText())
        if (tval->value == what->value)         // Do delimiters matter?
            return what;
    Ooops("Value $1 does not match pattern value $2", test, what);
    return nullptr;
}


Tree *Bindings::Do(Name *what)
// ----------------------------------------------------------------------------
//   The pattern contains a name: bind it as a closure, no evaluation
// ----------------------------------------------------------------------------
{
    // Check if we are testing e.g. [getpid as integer32] vs. [getpid]
    if (!defined)
    {
        StripBlocks();
        if (Name *name = test->AsName())
        {
            if (name->value == what->value)
            {
                defined = what;
                return what;
            }
        }
        Ooops("Value $1 does not match name $2", test, what);
        return nullptr;
    }

    // If there is already a binding for that name, value must match
    // This covers both a pattern with 'pi' in it and things like 'X+X'
    if (Tree *bound = argContext.Bound(what, false))
    {
        MustEvaluate();
        Tree_p result = Tree::Equal(bound, test) ? bound : nullptr;
        record(bindings, "Arg check %t vs %t: %t", test, bound, result);
        if (!result)
            Ooops("Value $1 does not match named value $2", test, what);
        return result;
    }

    // If some earlier evaluation failure, abort here
    if (!test)
        return nullptr;

    // The test value may have been evaluated. If so, use evaluated value
    if (Tree *cached = cache.Cached(test))
        test = cached;

    // Otherwise, bind test value to name
    return Bind(what, test);
}


Tree *Bindings::Do(Block *what)
// ----------------------------------------------------------------------------
//   The pattern contains a block: look inside
// ----------------------------------------------------------------------------
{
    // Deal with the case of a metablock: evaluate expression inside
    if (Tree *expr = what->IsMetaBox())
    {
        MustEvaluate();
        if (IsNil(expr))
            return test ? nullptr : expr;
        expr = Evaluate(declContext.Symbols(), expr);
        return Tree::Equal(test, expr) ? expr : nullptr;
    }

    return what->child->Do(this);
}


Tree *Bindings::Do(Prefix *what)
// ----------------------------------------------------------------------------
//   The pattern contains prefix: check that the left part matches
// ----------------------------------------------------------------------------
{
    StripBlocks();
    Tree *input = test;

    // If we have a lambda prefix, match the value
    if (Name *name = IsLambda(what))
    {
        if (defined)
            Ooops("Lambda form $1 nested inside pattern $2", what, defined);
        defined = what;
        cache.Cache(test, test); // Bind value as is
        Tree *result = name->Do(this);
        return result;
    }

    // Check if we have a variable binding
    if (Tree *binding = IsVariableBinding(what))
    {
        Tree *bound = Evaluate(EvaluationScope(), test, VARIABLE);
        if (!bound)
        {
            Ooops("Unable to bind variable $1 to $2", what, test);
           return nullptr;
        }
        Infix *rewrite = IsVariableDefinition(bound);
        if (!rewrite)
        {
            Ooops("Binding variable $1 to non-variable $2", what, bound);
            return nullptr;
        }
        if (Name *borrowed = Borrowed(rewrite))
        {
            Ooops("Cannot bind $2 to variable $1", binding, rewrite->left);
            Ooops("Already bound as variable to $1", borrowed);
            return nullptr;
        }
        if (Name *name = binding->AsName())
        {
            Borrow(name, rewrite);
            return Bind(name, bound);
        }
        if (Infix *typecast = binding->AsInfix())
        {
            Tree_p want = EvaluateType(typecast->right);
            Infix *vartype = IsTypeAnnotation(rewrite->left);
            Tree_p have = EvaluateType(vartype->right);
            Name *name = vartype->left->AsName();
            if (!Tree::Equal(want, have))
            {
                Ooops("Parameter Type $2 for $1 does not match", what, want);
                Ooops("argument type $2 for $1", name, have);
                return nullptr;
            }
            Borrow(name, rewrite);
            return Bind(name, bound);
        }
    }

    // The test itself should be a prefix, otherwise evaluate
    Prefix *pfx = test->AsPrefix();
    if (!pfx)
    {
        MustEvaluate();
        pfx = test->AsPrefix();
    }

    if (Prefix *pfx = test->AsPrefix())
    {

        // Check prefix left first, which may set 'defined' to name
        defined = nullptr;
        test = pfx->left;
        if (Tree_p left = what->left->Do(this))
        {
            defined = what;
            test = pfx->right;
            if (Tree_p right = what->right->Do(this))
            {
                if (left != pfx->left || right != pfx->right)
                    pfx = new Prefix(pfx, left, right);
                return pfx;
            }
        }
    }

    // Mismatch
    Ooops("Prefix $1 does not match $2", what, input);
    return nullptr;
}


Tree *Bindings::Do(Postfix *what)
// ----------------------------------------------------------------------------
//   The pattern contains posfix: check that the right part matches
// ----------------------------------------------------------------------------
{
    StripBlocks();
    Tree *input = test;

    // The test itself should be a postfix, otherwise evaluate
    Postfix *pfx = test->AsPostfix();
    if (!pfx)
    {
        MustEvaluate();
        pfx = test->AsPostfix();
    }

    // The test itself should be a postfix
    if (Postfix *pfx = test->AsPostfix())
    {
        // Check postfix right first, which maye set 'defined' to name
        defined = nullptr;
        test = pfx->right;
        if (Tree_p right = what->right->Do(this))
        {
            defined = what;
            test = pfx->left;
            if (Tree_p left = what->left->Do(this))
            {
                if (left != pfx->left || right != pfx->right)
                    pfx = new Postfix(pfx, left, right);
                return pfx;
            }
        }
    }

    // All other cases are a mismatch
    Ooops("Postfix $1 does not match $2", what, input);
    return nullptr;
}


Tree *Bindings::Do(Infix *what)
// ----------------------------------------------------------------------------
//   The complicated case: various definitions
// ----------------------------------------------------------------------------
{
    Tree *input = test;

    // Check if we have a type annotation, like [X:natural]
    if (IsTypeAnnotation(what))
    {
        bool outermost = test == self;

        // Check if we test [A+B as integer] against [lambda N as integer]
        if (Name *declared = IsTypeCastDeclaration(what))
        {
            if (Infix *cast = IsTypeCast(test))
            {
                test = cast->left;
                Tree_p wtype = EvaluateType(what->right);
                Tree_p ttype = EvaluateType(cast->right);
                if (wtype != ttype || HadErrors())
                {
                    Ooops("Type $2 of $1", cast->right, cast);
                    Ooops("does not match type $2 of $1", what->right, what);
                    return nullptr;
                }

                // Process the lambda name
                defined = what;
                if (Tree_p result = declared->Do(this))
                    return result;
            }

            // Not a type cast - Give up
            Ooops("Cannot match type cast $1 against $2", what, input);
            return nullptr;
        }

        // Need to evaluate the type on the right
        Tree_p want = EvaluateType(what->right);
        if (!want || HadErrors() || IsError(want))
            return nullptr;

        // Type check value against type
        if (outermost)
        {
            type = want;
        }
        else
        {
            Tree_p checked = TypeCheck(EvaluationScope(), want, test);
            if (!checked)
            {
                Ooops("Value $1 does not belong to type $2", test, want);
                return nullptr;
            }
            test = checked;
        }

        // Need to match the left part with the converted value
        if (!outermost && !defined)
            defined = what;
        return what->left->Do(this);
    }

    // If nothing defined yet, we are it
    defined = what;

    // Check if we have a guard clause
    if (IsPatternCondition(what))
    {
        // It must pass the rest (need to bind values first)
        Tree_p left = what->left->Do(this);
        if (!left)
            return nullptr;

        // Here, we need to evaluate in the local context, not eval one
        Tree_p check = EvaluateGuard(what->right);
        if (check == xl_true)
            return left;
        else if (check != xl_false)
            Ooops ("Invalid guard clause, $1 is not a boolean", check);
        else
            Ooops("Guard clause $1 is not verified", what->right);
        return nullptr;
    }

    // In all other cases, we need an infix with matching name
    StripBlocks();
    Infix *ifx = test->AsInfix();
    if (!ifx || ifx->name != what->name)
    {
        // Try to get an infix by evaluating what we have
        MustEvaluate();
        ifx = test->AsInfix();
    }
    if (ifx)
    {
        if (ifx->name != what->name)
        {
            Ooops("Infix names for $1 and $2 don't match", what, ifx);
            return nullptr;
        }

        test = ifx->left;
        if (Tree_p left = what->left->Do(this))
        {
            test = ifx->right;
            if (Tree_p right = what->right->Do(this))
            {
                if (left != ifx->left || right != ifx->right)
                    ifx = new Infix(ifx, left, right);
                return ifx;
            }
        }
    }

    // Mismatch
    Ooops("Infix $1 does not match $2", what, input);
    return nullptr;
}


bool Bindings::MustEvaluate()
// ----------------------------------------------------------------------------
//   Evaluate 'test', ensuring that each bound arg is evaluated at most once
// ----------------------------------------------------------------------------
{
    Tree_p evaluated = Evaluate(EvaluationScope(), test);

    // For matching purpose, we need the value inside closures
    while (Scope *scope = Context::IsClosure(evaluated))
    {
        Prefix *closure = (Prefix *) (Tree *) evaluated;
        evaluated = Evaluate(scope, closure->right);
        if (!evaluated)
            evaluated = LastErrorAsErrorTree();
    }

    bool changed = test != evaluated;
    test = evaluated;

    return changed;
}


Tree *Bindings::Evaluate(Scope *scope, Tree *expr, EvaluationMode mode)
// ----------------------------------------------------------------------------
//   Helper evaluation function for recursive evaluation
// ----------------------------------------------------------------------------
{
    Tree_p result = cache.Cached(expr);
    if (!result)
    {
        record(bindings, "Evaluating %t in %t mode %u",
               expr, scope, mode);
        result = Interpreter::DoEvaluate(scope, expr, mode, cache);
        cache.Cache(expr, result);
        cache.Cache(result, result);
        record(bindings, "Evaluate %t = new %t", test, result);
    }
    else
    {
        record(bindings, "Evaluate %t = old %t", test, result);
    }

    return result;
}


Tree *Bindings::EvaluateType(Tree *type)
// ----------------------------------------------------------------------------
//   Evaluate a type expression (in declaration context)
// ----------------------------------------------------------------------------
{
    Tree_p result = Evaluate(DeclarationScope(), type);
    return result;
}


Tree *Bindings::EvaluateGuard(Tree *guard)
// ----------------------------------------------------------------------------
//   Evaluate a guard condition (in arguments context)
// ----------------------------------------------------------------------------
{
    return Evaluate(ArgumentsScope(), guard);
}


Tree *Bindings::TypeCheck(Scope *scope, Tree *type, Tree *value)
// ----------------------------------------------------------------------------
//   Check if we pass the test for the type
// ----------------------------------------------------------------------------
{
    if (Tree *base = IsVariableType(type))
    {
        if (Rewrite *rewrite = value->As<Rewrite>())
        {
            if (Infix *vardef = IsTypeAnnotation(rewrite->left))
            {
                if (Tree::Equal(base, vardef->right))
                    return rewrite;
            }
        }
        return nullptr;
    }
    Tree_p cast = cache.CachedTypeCheck(type, value);
    if (!cast)
    {
        cast = Interpreter::DoTypeCheck(scope, type, value, cache);
        cache.TypeCheck(type, value, cast);
    }
    if (type != XL::value_type && IsError(cast))
        cast = nullptr;
    return cast;
}


Tree *Bindings::ResultTypeCheck(Tree *result, bool special)
// ----------------------------------------------------------------------------
//   Check the result against the expected type if any
// ----------------------------------------------------------------------------
{
    // If no type specified, just return the result as is
    if (!type || !result)
        return result;

    // For builtin and C declarations
    if (special)
    {
        // Convert [integer] to [boolean] as needed
        if (type == boolean_type)
            if (Natural *ival = result->AsNatural())
                return (ival->value) ? xl_true : xl_false;
    }

    // Otherwise, insert a type check in the bound arguments context
    Tree_p cast = TypeCheck(ArgumentsScope(), type, result);
    if (!cast)
        Ooops("The returned value $1 is not a $2", result, type);
    return cast;
}


Tree *Bindings::Bind(Name *name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a new binding in the current context, remember left and right
// ----------------------------------------------------------------------------
//   We wrap the input argument in a closure to get the proper context on eval
{
    record(bindings, "Binding %t = %t", name, value);
    if (!cache.Cached(value))
        value = evalContext.Enclose(value);
    Rewrite_p rewrite = argContext.Define(name, value);
    bindings.push_back(rewrite);
    cache.Cache(name, value);
    return value;
}


Tree *Bindings::Argument(unsigned n, bool unwrap)
// ----------------------------------------------------------------------------
//   Unwrap all arguments before calling a builtin or native function
// ----------------------------------------------------------------------------
{
    Tree_p value = Binding(n)->right;
    while (Scope *scope = Context::IsClosure(value))
    {
        Prefix *closure = (Prefix *) (Tree *) value;
        value = closure->right;
        if (unwrap)
        {
            value = Evaluate(scope, value);
            if (!value)
                value = LastErrorAsErrorTree();
        }
    }
    return value;
}


Tree *Bindings::NamedTree(unsigned n)
// ----------------------------------------------------------------------------
//    Evaluate the given argument without evaluting bodies
// ----------------------------------------------------------------------------
{
    Tree_p value = Binding(n)->right;
    while (Scope *scope = Context::IsClosure(value))
    {
        Prefix *closure = (Prefix *) (Tree *) value;
        value = Evaluate(scope, closure->right, NAMED);
        if (!value)
            value = LastErrorAsErrorTree();
    }
    return value;
}


struct BorrowedInfo : Info
// ----------------------------------------------------------------------------
//   Marker that there is a variabe binding
// ----------------------------------------------------------------------------
{
    BorrowedInfo(Name *name): name(name) {}
    Name_p name;
};


void Bindings::Borrow(Name *name, Infix *rewrite)
// ----------------------------------------------------------------------------
//   Mark a variable as borrowed
// ----------------------------------------------------------------------------
{
    BorrowedInfo *info = new BorrowedInfo(name);
    rewrite->SetInfo(info);
    borrowed.push_back(rewrite);
    record(bindings, "Borrow %t as %t", rewrite, name);
}


Name *Bindings::Borrowed(Infix *rewrite)
// ----------------------------------------------------------------------------
//   Check if we already borrowed that variable
// ----------------------------------------------------------------------------
{
    BorrowedInfo *info = rewrite->GetInfo<BorrowedInfo>();
    record(bindings, "Borrowed %t is %t", rewrite, info ? info->name : nullptr);
    if (info)
        return info->name;
    return nullptr;
}


Bindings::Bindings(Tree *expr, EvaluationCache &cache)
// ----------------------------------------------------------------------------
//   Bindings constructor
// ----------------------------------------------------------------------------
    : evalContext(),
      declContext(),
      argContext(),
      self(expr),
      test(expr),
      cache(cache),
      type(nullptr),
      bindings(),
      borrowed()
{
    record(bindings, ">Created bindings %p for %t", this, expr);
}


Bindings::~Bindings()
// ----------------------------------------------------------------------------
//   When removing bindings, unmark all the borrowed items
// ----------------------------------------------------------------------------
{
    Unborrow();
    record(bindings, "<Destroyed bindings %p", this);
}


void Bindings::Set(Rewrite *decl, Scope *evalScope, Scope *declScope)
// ----------------------------------------------------------------------------
//   Check a particular declaration
// ----------------------------------------------------------------------------
{
    record(bindings, "Set %t for scope %t decl %t", self,evalScope,declScope);
    evalContext.SetSymbols(evalScope);
    declContext.SetSymbols(declScope);
    argContext.SetSymbols(declScope);
    argContext.CreateScope();
    rewrite = decl;
    test = self;
    type = nullptr;
    bindings.clear();
    Unborrow();
}


void Bindings::Unborrow()
// ----------------------------------------------------------------------------
//   Unborrow whatever we borrowed in the current bindings
// ----------------------------------------------------------------------------
{
    for (auto b : borrowed)
    {
        bool purged = b->Purge<BorrowedInfo>();
        record(bindings, "Purging %t %+s", b, purged ? "succeeded" : "failed");
    }
    borrowed.clear();
}



// ============================================================================
//
//   Evaluation entry points
//
// ============================================================================

Tree *Interpreter::Evaluate(Scope *scope, Tree *expr)
// ----------------------------------------------------------------------------
//    Evaluate 'what', finding the final, non-closure result
// ----------------------------------------------------------------------------
{
    EvaluationCache cache;
    Tree_p result = DoEvaluate(scope, expr, SEQUENCE, cache);
    result = Unwrap(result, cache);
    return result;
}


Tree *Interpreter::TypeCheck(Scope *scope, Tree *type, Tree *value)
// ----------------------------------------------------------------------------
//   Check if 'value' matches 'type' in the given context
// ----------------------------------------------------------------------------
{
    EvaluationCache cache;
    return DoTypeCheck(scope, type, value, cache);
}


Tree *Interpreter::Unwrap(Tree *expr, EvaluationCache &cache)
// ----------------------------------------------------------------------------
//   Evaluate all closures inside a tree
// ----------------------------------------------------------------------------
{
    while (expr)
    {
        switch(expr->Kind())
        {
        case NATURAL:
        case REAL:
        case TEXT:
        case NAME:
            return expr;
        case INFIX:
        {
            Infix *infix = (Infix *) expr;
            Tree *left = Unwrap(infix->left, cache);
            Tree *right = Unwrap(infix->right, cache);
            if (left != infix->left || right != infix->right)
                infix = new Infix(infix, left, right);
            return infix;
        }
        case PREFIX:
        {
            Prefix *prefix = (Prefix *) expr;
            if (Scope *scope = Context::IsClosure(expr))
            {
                Prefix *closure = (Prefix *) (Tree *) expr;
                expr = DoEvaluate(scope, closure->right, SEQUENCE, cache);
                continue;
            }
            Tree *left = Unwrap(prefix->left, cache);
            Tree *right = Unwrap(prefix->right, cache);
            if (left != prefix->left || right != prefix->right)
                prefix = new Prefix(left, right);
            return prefix;
        }
        case POSTFIX:
        {
            Postfix *postfix = (Postfix *) expr;
            Tree *left = Unwrap(postfix->left, cache);
            Tree *right = Unwrap(postfix->right, cache);
            if (left != postfix->left || right != postfix->right)
                postfix = new Postfix(left, right);
            return postfix;
        }
        case BLOCK:
        {
            Block *block = (Block *) expr;
            if (Scope *scope = expr->As<Scope>())
                return scope;
            expr = block->child;
        }
        }
    }
    return expr;
}


// ============================================================================
//
//    Helper functions for the interpreter
//
// ============================================================================

static Tree *typeCheck(Scope *scope,
                       Tree *result,
                       Tree *type,
                       EvaluationCache &cache,
                       bool special)
// ----------------------------------------------------------------------------
//   Check the result against the expected type if any
// ----------------------------------------------------------------------------
{
    // If no type specified, just return the result as is
    if (!type || !result)
        return result;

    // For builtin and C declarations
    if (special)
    {
        // Convert [integer] to [boolean] as needed
        if (type == boolean_type)
            if (Natural *ival = result->AsNatural())
                return (ival->value) ? xl_true : xl_false;
    }

    // Otherwise, insert a type check in the bound arguments context
    Tree_p cast = Interpreter::DoTypeCheck(scope, type, result, cache);
    if (!cast)
        Ooops("The returned value $1 is not a $2", result, type);
    return cast;
}


static Tree_p doPatternMatch(Scope *scope,
                             Tree *pattern,
                             Tree *expr,
                             EvaluationCache &cache)
// ----------------------------------------------------------------------------
//   Match the pattern in a scope
// ----------------------------------------------------------------------------
{
    Tree_p cast;
    while (Closure *closure = expr->As<Closure>())
        expr = closure->Value();
    Bindings bindings(expr, cache);
    Context args(bindings.ArgumentsScope());
    Rewrite *rewrite = args.Define(pattern, xl_self);
    bindings.Set(rewrite, scope, scope);
    Tree *matched = pattern->Do(bindings);
    if (matched)
        cast = bindings.Enclose(matched);
    record(typecheck, "Expression %t as matching %t is %t matched %t",
           expr, pattern, cast, matched);
    return cast;
}


static Tree_p doDot(Context &context,
                    Infix *infix,
                    EvaluationCache &cache)
// ----------------------------------------------------------------------------
//   Process [X.Y]
// ----------------------------------------------------------------------------
{
    Scope *symbols = context.Symbols();
    Tree_p name = infix->left;
    Tree_p value = infix->right;
    Tree *where = Interpreter::DoEvaluate(symbols, name, NAMED, cache);
    if (!where)
        return nullptr;
    Scope *scope = where->As<Scope>();
    if (!scope)
    {
        if (Closure *closure = where->As<Closure>())
        {
            if (Closure *inner = closure->Value()->As<Closure>())
                scope = inner->CapturedScope();
            else
                scope = closure->CapturedScope();
        }
    }
    if (!scope)
    {
        Context context;
        scope = context.CreateScope();
        Initializers inits;
        bool hasInstructions = context.ProcessDeclarations(where, inits);
        if (scope->IsEmpty())
            scope = scope->Enclosing();
        if (hasInstructions)
        {
            record(eval, "Scoped value %t has instructions", where);
            scope = nullptr;
        }
        else if (!Interpreter::DoInitializers(inits, cache))
        {
            record(eval, "Initializer failed in dot for %t", where);
            scope = nullptr;
        }
    }
    if (scope)
        return Interpreter::DoEvaluate(scope, value, LOCAL, cache);
    else
        Ooops("No scope in $1 (evaluated as $2)", name, where);
    return nullptr;
}


static Tree_p doAssignment(Scope *scope,
                           Infix *infix,
                           EvaluationMode mode,
                           EvaluationCache &cache)
// ----------------------------------------------------------------------------
//   Process assignments like [X := Y]
// ----------------------------------------------------------------------------
{
    // Evaluate the target as a variable
    Tree_p decl = Interpreter::DoEvaluate(scope, infix->left, VARIABLE, cache);
    Tree *to = decl;
    if (!to)
    {
        Ooops("Cannot assign to $1", infix->left);
        return to;
    }

    // Get the original definition
    Rewrite_p rewrite = to->As<Rewrite>();
    if (!rewrite)
    {
        Ooops("Left of assignment, $1, is not a variable", to);
        return rewrite;
    }

    // Evaluate the assigned value
    Tree_p value = infix->right;
    value = Interpreter::DoEvaluate(scope, value, EXPRESSION, cache);
    if (!value)
        return value;                   // We should already have an error

    // Check the type fo the pattern
    Tree_p pattern = rewrite->Pattern();
    Tree_p type = AnnotatedType(pattern);
    if (!type)
    {
        Ooops("Cannot assign to constant $1", rewrite->Pattern());
        return nullptr;
    }

    // The type was already evaluated, so can check it right away
    Tree_p cast = Interpreter::DoTypeCheck(scope, type, value, cache);
    if (!cast)
    {
        Ooops("Assigned value $1 does not match type $2", infix->right, type);
        if (value != infix->right)
            Ooops("The assigned value evaluated to $1", value);
        return nullptr;
    }

    // We passed all the check: update the value in the symbol table
    rewrite->right = value;

    // Return the assigned value or the variable
    return mode == VARIABLE ? decl : value;
}


static Tree_p doLatePrefix(Scope *scope,
                           Prefix *prefix,
                           EvaluationCache &cache)
// ----------------------------------------------------------------------------
//   Evaluate a prefix if it was not found in the symbol table
// ----------------------------------------------------------------------------
{
    Tree *expr = prefix->left;
    kind exprk = expr->Kind();
    if (exprk == NAME || exprk == BLOCK)
    {
        record(eval, "Retrying prefix %t", prefix);
        Tree_p left = Interpreter::DoEvaluate(scope, expr, MAYFAIL, cache);
        if (!left || left == expr)
            return nullptr;
        prefix = new Prefix(prefix, left, prefix->right);
        Tree_p result = Interpreter::DoEvaluate(scope,prefix,EXPRESSION,cache);
        return result;
    }
    return nullptr;
}


static Tree_p doLateName(Scope *scope, Name *name, EvaluationCache &cache)
// ----------------------------------------------------------------------------
//   Process late names like 'super' and 'context'
// ----------------------------------------------------------------------------
{
    if (IsSuper(name))
        return scope->Enclosing();
    if (IsContext(name))
        return scope;
    return nullptr;
}



// ============================================================================
//
//    Evaluation engine
//
// ============================================================================

static Tree *eval(Scope   *evalScope,
                  Scope   *declScope,
                  Tree    *expr,
                  Rewrite *decl,
                  void    *bnds)
// ----------------------------------------------------------------------------
//   Bind 'self' to the pattern in 'decl', and if successful, evaluate
// ----------------------------------------------------------------------------
{
    Errors errors;
    errors.Log(Error("Pattern $1 does not match $2:",
                     decl->left, expr), true);

    // Bind argument to parameters
    Bindings &bindings = *((Bindings *) bnds);
    Tree *pattern = decl->Pattern();
    XL_ASSERT(bindings.Self() == expr);
    bindings.Set(decl, evalScope, declScope);
    Tree_p matched = pattern->Do(bindings);
    return matched;
}


Tree *Interpreter::DoEvaluate(Scope *inputScope,
                              Tree *inputExpr,
                              EvaluationMode mode,
                              EvaluationCache &cache)
// ----------------------------------------------------------------------------
//   Internal evaluator - Short circuits a few common expressions
// ----------------------------------------------------------------------------
{
    // Check stack depth during evaluation
    static uint depth = 0;
    Save<uint>  save(depth, depth+1);
    if (depth > Opt::stackDepth)
    {
        Errors::Abort(Ooops("Stack depth exceeded evaluating $1", inputExpr));
        record(eval, "Stack depth exceeded evaluating %t", inputExpr);
        return nullptr;
    }

    const char *modename[EVALUATION_MODES] =
    {
        "sequence",
        "statement",
        "expression",
        "may fail",
        "variable",
        "local",
        "named"
    };
    Tree_p type;
    Scope_p scope = inputScope;
    Tree_p expr = inputExpr;
    Tree_p result;
    record(eval, ">%t %+s in %p", expr, modename[mode], scope);

rescope:
    // Check if there was some error, if so don't keep looking
    if (HadErrors())
    {
        record(eval, "<Aborting %t", Errors::Aborting());
        return Errors::Aborting();
    }

    // Check if we have instructions to process. If not, return declarations
    Context context(scope);
    if (mode == SEQUENCE || mode == VARIABLE)
    {
        scope = context.CreateScope();
        Initializers inits;
        bool hasInstructions = context.ProcessDeclarations(expr, inits);
        if (scope->IsEmpty())
            scope = context.PopScope();
        if (!DoInitializers(inits, cache))
        {
            record(eval, "<Initializer failed for %t", expr);
            return nullptr;
        }
        if (!hasInstructions)
        {
            record(eval, "<Return scope %t", scope);
            return scope;
        }
    }

next:
    switch (expr->Kind())
    {
    case BLOCK:
    {
        // Short-circuit evaluation of blocks
        Block *block = (Block *) expr.Pointer();
        expr = block->child;
        goto next;
    }

    case NAME:
    {
        // Special-case nil
        Name *name = (Name *) expr.Pointer();
        if (name->value == "nil")
        {
            record(eval, "<Evaluated nil");
            return nullptr;
        }
        break;
    }

    // Short-circuit evaluation of sequences and declarations
    case INFIX:
    {
        Infix *infix = (Infix *) expr.Pointer();

        // Process sequences, trying to avoid deep recursion
        if (IsSequence(infix))
        {
            cache.Clear();
            Tree_p left = DoEvaluate(scope, infix->left, STATEMENT, cache);
            if (left && left != infix->left)
                result = left;
            if (HadErrors() || IsError(left))
            {
                record(eval, "<Errors evaluating first in sequence %t = %t",
                       infix->left, left);
                return left;
            }
            expr = infix->right;
            if (mode != VARIABLE)
                mode = STATEMENT;
            cache.Clear();
            goto next;
        }

        // Skip definitions, initalizer have been done above
        if (IsDefinition(infix))
        {
            if (Name *name = infix->left->AsName())
            {
                if (!IsSelf(infix->right))
                {
                    Context bodyCtx(&context);
                    Tree_p value = DoEvaluate(bodyCtx.Symbols(),
                                              infix->right, SEQUENCE, cache);
                    Rewrite *rewrite = context.Reference(name, false);
                    rewrite->right = value;
                    record(eval, "Evaluated name %t as %t", name, value);
                }
            }

            record(eval, "<Infix %t is a definition, returning %t",
                   infix, result);
            return result;
        }

        // Evaluate [X.Y]
        if (IsDot(infix))
        {
            Tree_p scoped = doDot(context, infix, cache);
            record(eval, "<Scoped %t is %t", infix, scoped);
            return scoped;
        }

        // Check [X as matching(P)]
        if (IsTypeCast(infix))
        {
            Tree_p want = DoEvaluate(scope, infix->right, EXPRESSION, cache);
            if (!want)
            {
                record(eval, "<Matching type: no type for %t", infix->right);
                return nullptr;
            }
            infix->right = want;
            if (Tree *matching = IsPatternMatchingType(want))
            {
                Tree_p cast = doPatternMatch(context, matching,
                                             infix->left, cache);
                record(eval, "<Pattern match %t matching %t = %t",
                       infix->left, matching, cast);
                return cast;
            }
        }

        // Evaluate assignments such as [X := Y]
        if (IsAssignment(infix))
        {
            Tree_p assigned = doAssignment(scope, infix, mode, cache);
            record(eval, "<Assignment %t is %t", infix, assigned);
            return assigned;
        }
        break;
    }

    // Short-circuit evaluation of scope, quotes and [matching X]
    case PREFIX:
    {
        Prefix *prefix = (Prefix *) expr.Pointer();
        // Check if the left is a block containing only declarations
        // This deals with [(X is 3) (X + 1)], i.e. closures
        Initializers inits;
        if (Scope_p closure = context.ProcessScope(prefix->left, inits))
        {
            if (!DoInitializers(inits, cache))
            {
                record(eval, "<Initializers failed for scope %t", prefix->left);
                return nullptr; // REVISIT: Should be an error?
            }
            scope = closure;
            expr = prefix->right;
            if (mode != VARIABLE)
                mode = SEQUENCE;
            goto rescope;
        }

        // Check if we have variable type
        if (IsVariableType(prefix))
        {
            prefix->right = DoEvaluate(scope, prefix->right, EXPRESSION, cache);
            record(eval, "<Variable type %t", prefix);
            return prefix;
        }

        // Check if we are evaluating a pattern matching type
        if (Tree *matching = IsPatternMatchingType(prefix))
        {
            result = xl_parse_tree(scope, matching);
            if (result == matching)
            {
                prefix->left = xl_matching;
                result = prefix;
            }
            else
            {
                result = new Prefix(prefix, xl_matching, result);
            }
            record(eval, "<Type matching %t", result);
            return result;
        }

        // Check 'with' type declarations
        if (IsWithDeclaration(prefix))
        {
            record(eval, "<Prefix %t is a with type clause", prefix);
            return nullptr;
        }

        // Filter out declaration such as [extern foo(bar)]
        if (IsDefinition(prefix))
        {
            record(eval, "<Prefix %t is a definition", prefix);
            return prefix;
        }

        // Process [quote X] - REVISIT: Should just be a C function now?
        if (Tree *quoted = IsQuote(prefix))
        {
            result = xl_parse_tree(scope, quoted);
            record(eval, "<Quote %t", result);
            return result;
        }

        // Filter out import statements (processed also during parsing)
        if (Name *import = prefix->left->AsName())
        {
            if (eval_fn callback = MAIN->Importer(import->value))
            {
                record(eval, "<Import statement %t callback %p",
                       prefix, callback);
                return callback(scope, prefix);
            }
        }

        // Check if the left is a qualified scope
        // This turns statement [IO.print "Hello"] into [IO.{print "Hello"}]
        if (mode == STATEMENT || mode == SEQUENCE)
        {
            if (Infix *qualified = IsDot(prefix->left))
            {
                Tree *qualifier = qualified->left;
                Tree *name = qualified->right;
                Tree *args = prefix->right;
                Prefix_p inner = new Prefix(prefix, name, args);
                Infix_p outer = new Infix(qualified, qualifier, inner);
                Tree_p scoped = doDot(context, outer, cache);
                record(eval, "<Scoped statement %t is %t", outer, scoped);
                return scoped;
            }
        }
        break;
    }

    default: break;
    } // switch(Kind())


    // Here, we have eliminated all the special cases, and must lookup
    Errors errors;
    errors.Log(Error("Unable to evaluate $1:", inputExpr), true);
    Context::LookupMode lm = Context::RECURSIVE;
    if (mode == LOCAL)
        lm = Context::SINGLE_SCOPE;
    EvaluationCache lookupCache;
    Bindings bindings(expr, lookupCache);
    result = context.Lookup(expr, eval, &bindings, lm);

    // If we could not find the result with direct lookup, check backups
    if (result)
    {
        errors.Clear();
    }
    else
    {
        if (Tree *error = Errors::Aborting())
        {
            record(eval, "<Aborting %t evaluating %t", error, expr);
            return nullptr;
        }

        if (expr->IsConstant())
            result = expr;
        else if (Prefix *prefix = expr->AsPrefix())
            result = doLatePrefix(scope, prefix, cache);
        else if (Name *name = expr->AsName())
            result = doLateName(scope, name, cache);

        if (result)
        {
            record(eval, "<Special case %t", expr);
            return result;
        }

       if (mode == MAYFAIL)
       {
           record(eval, "<May fail %t", expr);
           return nullptr;
       }
        Ooops("Nothing matches $1", expr);
        record(eval, "<Failed to find a match for %t", expr);
        return nullptr;
    }

    // Check if we need to get the value from a variable value
    if (mode != VARIABLE)
        if (Infix *def = IsVariableDefinition(result))
            result = def->right;

    Rewrite *decl = bindings.Declaration();

    // Evaluate the body in arguments
    Tree *body = decl->right;

    // Successfully bound - Return variable declaration in a closure
    if (mode == VARIABLE)
    {
        if (IsVariableDefinition(decl))
        {
            record(eval, "<Declaration is a variable definition %t", decl);
            return decl;
        }
        if (IsVariableDefinition(body))
        {
            record(eval, "<Body is a variable definition %t", body);
            return body;
        }
    }

    // Check if we have a builtin
    bool special = false;
    if (Prefix *prefix = body->AsPrefix())
    {
        if (Text *name = IsBuiltin(prefix))
        {
            Interpreter::builtin_fn callee = Interpreter::builtins[name->value];
            if (!callee)
            {
                Ooops("Nonexistent builtin $1", name);
                record(eval, "<Nonexistent builtin %t", name);
                return nullptr;
            }

            result = callee(bindings);
            special = true;
        }
        else if (Text *name = IsNative(prefix))
        {
            Native *native = Interpreter::natives[name->value];
            if (!native)
            {
                Ooops("Nonexistent native function $1", name);
                record(eval, "<Nonexistent native function %t", name);
                return nullptr;
            }

            result = native->Call(bindings);
            special = true;
        }
        if (special)
        {
            result = bindings.ResultTypeCheck(result, special);
            record(eval, "<Special %t", result);
            return result;
        }
    }

    // Check if we evaluate as self
    if (IsSelf(body))
    {
        result = bindings.ResultTypeCheck(result);
        record(eval, "<Self %t", result);
        return result;
    }

    // Check if we are simply trying to lookup
    else if (mode == NAMED)
    {
        result = body;
        record(eval, "<Named %t", result);
        return result;
    }

    // Otherwise evaluate the body in the binding arguments scope
    scope = bindings.ArgumentsScope();

    // Since lambdas are catch-all, we need to evaluate their bodies
    // in a context where the lambda is not visible
    Tree *pattern = decl->Pattern();
    Scope *declScope = bindings.DeclarationScope();
    if (IsLambda(pattern))
        scope->Reparent(declScope->Enclosing());

    // Check if the body returns a matching type
    // If so, we need to evaluate in a scope where that pattern is defined
    type = bindings.ResultType();
    if (mode != VARIABLE)
        mode = SEQUENCE;
    if (type)
    {
        if (Tree *matching = IsPatternMatchingType(type))
        {
            result = doPatternMatch(scope, matching, body, cache);
            record(eval, "<Pattern matching cast to %t is %t", type, result);
            return result;
        }
        if (IsVariableType(type))
            mode = VARIABLE;
    }

    // Check if we returned a variable
    if (Rewrite *vardef = body->As<Rewrite>())
    {
        if (IsVariableDefinition(vardef))
        {
            result = (mode == VARIABLE) ? vardef : vardef->right;
            record(eval, "<Variable definition %t is %t", vardef->left, result);
            return result;
        }
    }

    // Loop and try again
    cache.Clear();
    expr = body;
    GarbageCollector::SafePoint();
    goto rescope;
}


Tree *Interpreter::DoTypeCheck(Scope *scope,
                               Tree *type,
                               Tree *value,
                               EvaluationCache &cache)
// ----------------------------------------------------------------------------
//   Implementation of type checking in interpreter
// ----------------------------------------------------------------------------
{
    record(typecheck, "Check %t against %t in scope %t",
           value, type, scope);
    Tree_p test = new Infix("as", value, type, value->Position());
    Tree_p result = DoEvaluate(scope, test, MAYFAIL, cache);
    record(typecheck, "Checked %t against %t in scope %t, got %t",
           value, type, scope, result);
    return result;
}


bool Interpreter::DoInitializers(Initializers &inits, EvaluationCache &cache)
// ----------------------------------------------------------------------------
//   Process all initializers in order
// ----------------------------------------------------------------------------
{
    for (auto &i : inits)
    {
        Scope *scope = i.scope;
        Rewrite_p &rw = i.rewrite;
        Tree_p init = rw->right;
        Infix_p typedecl = rw->left->AsInfix();
        Tree_p type = DoEvaluate(scope, typedecl->right, EXPRESSION, cache);
        if (!type)
        {
            Ooops("Invalid type $1 for declaration of $2",
                  typedecl->right, typedecl->left);
            return false;
        }
        // Only evaluate type once
        typedecl->right = type;

        // Cast initialized value to this type
        Tree_p cast = DoTypeCheck(scope, type, init, cache);
        if (!cast)
        {
            Ooops("Initializer $1 does not match type $2",
                  init, type);
            return false;
        }
        rw->right = cast;
    }
    return true;
}



// ============================================================================
//
//    Generate the functions for builtins
//
// ============================================================================

#define R_INT(x)                                                        \
        {                                                               \
            Natural *result = new Natural((x), self->Position());       \
            result->tag |= signbit;                                     \
            return result;                                              \
        }
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
#define DIV0            if (RIGHT == 0) return Ooops("Divide $1 by zero", self)

#define UNARY_OP(Name, ReturnType, ArgType, Body)               \
static Tree *builtin_unary_##Name(Bindings &args)               \
{                                                               \
    Tree *self = args.Self();                                   \
    Tree *result = self;                                        \
    if (args.Size() != 1)                                       \
        return Ooops("Invalid number of arguments "             \
                     "for unary builtin " #Name                 \
                     " in $1", result);                         \
    ArgType *value  = args[0]->As<ArgType>();                   \
    ulong signbit = value->tag & (1 << Tree::SIGNBIT);          \
    if (!value && signbit == signbit)                           \
        return Ooops("Argument $1 is not a " #ArgType           \
                     " in builtin " #Name, args[0]);            \
    Body;                                                       \
}


#define BINARY_OP(Name, ReturnType, LeftType, RightType, Body)  \
static Tree *builtin_binary_##Name(Bindings &args)              \
{                                                               \
    Tree *self = args.Self();                                   \
    if (args.Size() != 2)                                       \
        return Ooops("Invalid number of arguments "             \
                     "for binary builtin " #Name                \
                     " in $1", self);                           \
    LeftType *left  = args[0]->As<LeftType>();                  \
    ulong signbit = left->tag & (1 << Tree::SIGNBIT);           \
    if (!left && signbit == signbit)                            \
        return Ooops("First argument $1 is not a " #LeftType    \
                     " in builtin " #Name, args[0]);            \
    RightType *right = args[1]->As<RightType>();                \
    if (!right)                                                 \
        return Ooops("Second argument $1 is not a " #RightType  \
                     " in builtin " #Name, args[1]);            \
    Body;                                                       \
}


#define NAME(N)                                                 \
Name_p xl_##N;                                                  \
static Tree *builtin_name_##N(Bindings &args)                   \
{                                                               \
    if (!xl_##N)                                                \
        xl_##N = new Name(#N,                                   \
                          args.Self()->Position());             \
    return xl_##N;                                              \
}


#define TYPE(N, Body)                                           \
Name_p N##_type;                                                \
static Tree *builtin_type_##N(Bindings &args)                   \
{                                                               \
    if (!N##_type)                                              \
        N##_type = new Name(#N,                                 \
                            args.Self()->Position());           \
    return N##_type;                                            \
}                                                               \
                                                                \
static Tree *builtin_typecheck_##N(Bindings &args)              \
{                                                               \
    if (args.Size() != 1)                                       \
        return Ooops("Invalid number of arguments "             \
                     "for " #N " typecheck"                     \
                     " in $1", args.Self());                    \
    Tree_p type  = N##_type;                                    \
    record(typecheck, "Builtin typecheck %t as %t = " #N,       \
           args.Unevaluated(0), type);                          \
    Body;                                                       \
}

#define TYPE_VALUE      (args[0])
#define TYPE_TREE       (args.Unevaluated(0))
#define TYPE_CLOSURE    (args.Binding(0)->right)
#define TYPE_NAMED      (args.NamedTree(0))
#define TYPE_VARIABLE   (args.Binding(0))

#include "builtins.tbl"



// ============================================================================
//
//   Initialize names and types like xl_self or natural_type
//
// ============================================================================

void Interpreter::InitializeBuiltins()
// ----------------------------------------------------------------------------
//    Initialize all the names, e.g. xl_self, and all builtins
// ----------------------------------------------------------------------------
//    This has to be done before the first context is created, because
//    scopes are initialized with xl_self
{
#define NAME(N)         xl_##N = new Name(#N);
#define TYPE(N, Body)   N##_type = new Name(#N);
#define UNARY_OP(Name, ReturnType, LeftTYpe, Body)
#define BINARY_OP(Name, ReturnType, LeftTYpe, RightType, Body)

#include "builtins.tbl"
}



// ============================================================================
//
//    Initialize top-level context
//
// ============================================================================

std::map<text, Interpreter::builtin_fn> Interpreter::builtins;
std::map<text, Native *> Interpreter::natives;

#define NORM2(x) Normalize(x), x

void Interpreter::InitializeContext(Context &context)
// ----------------------------------------------------------------------------
//    Initialize all the names, e.g. xl_self, and all builtins
// ----------------------------------------------------------------------------
//    Load the outer context with the type check operators referring to
//    the builtins.
{
#define UNARY_OP(Name, ReturnType, LeftTYpe, Body)                      \
    builtins[#Name] = builtin_unary_##Name;

#define BINARY_OP(Name, RetTy, LeftTy, RightTy, Body)                   \
    builtins[#Name] = builtin_binary_##Name;

#define NAME(N)                                                         \
    builtins[#N] = builtin_name_##N;                                    \
    context.Define(xl_##N, xl_self);

#define TYPE(N, Body)                                                   \
    builtins[#N] = builtin_type_##N;                                    \
    builtins[#N"_typecheck"] = builtin_typecheck_##N;                   \
                                                                        \
    Infix *pattern_##N =                                                \
        new Infix("as",                                                 \
                  new Prefix(xl_lambda,                                 \
                             new Name(NORM2("Value"))),                 \
                  N##_type);                                            \
    Prefix *value_##N =                                                 \
        new Prefix(xl_builtin,                                          \
                   new Text(#N"_typecheck"));                           \
    context.Define(N##_type, xl_self);                                  \
    context.Define(pattern_##N, value_##N);

#include "builtins.tbl"

    Name_p C = new Name(NORM2("C"));
    for (Native *native = Native::First(); native; native = native->Next())
    {
        record(native, "Found %t", native->Shape());
        kstring symbol = native->Symbol();
        natives[symbol] = native;
        Tree *shape = native->Shape();
        Prefix *body = new Prefix(C, new Text(symbol));
        context.Define(shape, body);
    }

    if (RECORDER_TRACE(symbols_sort))
        context.Dump(std::cerr, context.Symbols(), false);
}

XL_END
