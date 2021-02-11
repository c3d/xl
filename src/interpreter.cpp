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

Interpreter::Interpreter(Context &context)
// ----------------------------------------------------------------------------
//   Constructor for interpreter
// ----------------------------------------------------------------------------
{
    record(interpreter, "Created interpreter %p", this);
    InitializeContext(context);
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

inline bool Bindings::Do(Natural *what)
// ----------------------------------------------------------------------------
//   The pattern contains an natural: check we have the same
// ----------------------------------------------------------------------------
{
    StripBlocks();
    if (test->Kind() != NATURAL)
        MustEvaluate();
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
    StripBlocks();
    if (test->Kind() != REAL)
        MustEvaluate();
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
    StripBlocks();
    if (test->Kind() != TEXT)
        MustEvaluate();
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
    StripBlocks();

    // The test value may have been evaluated. If so, use evaluated value
    if (Tree *cached = cache.Cached(test))
        test = cached;

    // Check if we are testing e.g. [getpid as integer32] vs. [getpid]
    if (!defined)
    {
        if (Name *name = test->AsName())
        {
            if (name->value == what->value)
            {
                defined = what;
                return true;
            }
        }
        Ooops("Value $1 does not match name $2", test, what);
        return false;
    }

    // If there is already a binding for that name, value must match
    // This covers both a pattern with 'pi' in it and things like 'X+X'
    if (Tree *bound = argContext.Bound(what, false))
    {
        MustEvaluate();
        bool result = Tree::Equal(bound, test);
        record(bindings, "Arg check %t vs %t: %+s",
               test, bound, result ? "match" : "failed");
        if (!result)
            Ooops("Value $1 does not match named value $2", test, what);
        return result;
    }

    // If some earlier evaluation failure, abort here
    if (!test)
        return false;

    // Otherwise, bind test value to name
    Bind(what, test);
    return true;
}


bool Bindings::Do(Block *what)
// ----------------------------------------------------------------------------
//   The pattern contains a block: look inside
// ----------------------------------------------------------------------------
{
    StripBlocks();

    // Deal with the case of a metablock: evaluate expression inside
    if (Tree *expr = what->IsMetaBox())
    {
        MustEvaluate();
        expr = Evaluate(declContext.Symbols(), expr);
        return Tree::Equal(test, expr);
    }

    return what->child->Do(this);
}


bool Bindings::Do(Prefix *what)
// ----------------------------------------------------------------------------
//   The pattern contains prefix: check that the left part matches
// ----------------------------------------------------------------------------
{
    StripBlocks();

    bool retrying = true;
    Tree_p uneval = test;

    while (retrying)
    {
        retrying = false;

        // If we have a lambda prefix, match the value
        if (Name *name = IsLambda(what))
        {
            if (defined)
                Ooops("Lambda form $1 nested inside pattern $2", what, defined);
            defined = what;
            cache.Cache(test, test); // Bind value as is
            bool result = name->Do(this);

            // Since a top-level lambda is a "catch all, evaluate outside
            if (result)
            {
                Scope *evalScope = evalContext.PopScope();
                Scope *argScope = argContext.Symbols();
                argScope->Reparent(evalScope);
            }
            return result;
        }

        // The test itself should be a prefix
        if (Prefix *pfx = test->AsPrefix())
        {
            // Case of [until Condition loop Body]: define [until]
            if (defined && defined->AsInfix())
                defined = nullptr;

            // Check prefix left first, which may set 'defined' to name
            test = pfx->left;
            if (!what->left->Do(this))
                return false;
            test = pfx->right;
            if (what->right->Do(this))
                return true;
        }

        // Normal prefix matching failed: try to evaluate test
        if (MustEvaluate())
            if (test->AsPrefix())
                retrying = true;
    }

    // Mismatch
    Ooops("Prefix $1 does not match $2", what, uneval);
    return false;
}


bool Bindings::Do(Postfix *what)
// ----------------------------------------------------------------------------
//   The pattern contains posfix: check that the right part matches
// ----------------------------------------------------------------------------
{
    StripBlocks();

    bool retrying = true;
    Tree_p uneval = test;

    while (retrying)
    {
        retrying = false;

        // The test itself should be a postfix
        if (Postfix *pfx = test->AsPostfix())
        {
            // Check postfix right first, which maye set 'defined' to name
            test = pfx->right;
            if (!what->right->Do(this))
                return false;
            test = pfx->left;
            if (what->left->Do(this))
                return true;
        }

        // Normal postfix matching failed: try to evaluate test
        if (MustEvaluate())
            if (test->AsPostfix())
                retrying = true;
    }

    // All other cases are a mismatch
    Ooops("Postfix $1 does not match $2", what, uneval);
    return false;
}


bool Bindings::Do(Infix *what)
// ----------------------------------------------------------------------------
//   The complicated case: various definitions
// ----------------------------------------------------------------------------
{
    StripBlocks();

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
                Tree *wtype = EvaluateType(what->right);
                Tree *ttype = EvaluateType(cast->right);
                if (wtype != ttype || HadErrors())
                {
                    Ooops("Type $2 of $1", cast->right, cast);
                    Ooops("does not match type $2 of $1", what->right, what);
                    return false;
                }

                // Process the lambda name
                defined = what;
                return declared->Do(this);
            }
        }

        // Need to evaluate the type on the right
        Tree *want = EvaluateType(what->right);
        if (IsError(want))
            return false;

        // Type check value against type
        if (outermost)
        {
            type = want;
        }
        else
        {
            Tree *checked = TypeCheck(EvaluationScope(), want, test);
            if (!checked)
            {
                Ooops("Value $1 does not belong to type $2", test, want);
                return false;
            }
            test = checked;
        }

        // Need to match the left part with the converted value
        if (!outermost && !defined)
            defined = what;
        return what->left->Do(this);
    }

    // If nothing defined yet, we are it
    if (!defined)
        defined = what;

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
        MustEvaluate();
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


bool Bindings::MustEvaluate()
// ----------------------------------------------------------------------------
//   Evaluate 'test', ensuring that each bound arg is evaluated at most once
// ----------------------------------------------------------------------------
{
    Tree *evaluated = Evaluate(EvaluationScope(), test);
    bool changed = test != evaluated;
    test = evaluated;
    return changed;
}


Tree *Bindings::Evaluate(Scope *scope, Tree *expr)
// ----------------------------------------------------------------------------
//   Helper evaluation function for recursive evaluation
// ----------------------------------------------------------------------------
{
    Tree_p result = cache.Cached(expr);
    if (!result)
    {
        record(bindings, "Evaluating %t in %t", expr, scope);
        result = Interpreter::DoEvaluate(scope, expr,
                                         Interpreter::EXPRESSION, cache);
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
    Tree *cast = cache.CachedTypeCheck(type, value);
    if (!cast)
    {
        cast = Interpreter::DoTypeCheck(scope, type, value, cache);
        cache.TypeCheck(type, value, cast);
    }
    if (IsError(cast))
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

        // Convert null pointers to xl_nil
        if (!result)
            result = xl_nil;
    }

    // Otherwise, insert a type check in the declaration context
    Tree_p cast = TypeCheck(declContext.Symbols(), type, result);
    if (!cast)
        Ooops("The returned value $1 is not a $2", result, type);
    return cast;
}


void Bindings::Bind(Name *name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a new binding in the current context, remember left and right
// ----------------------------------------------------------------------------
//   We wrap the input argument in a closure to get the proper context on eval
{
    record(bindings, "Binding %t = %t", name, value);
    if (!cache.Cached(value))
        value = evalContext.Enclose(value);
    Rewrite *rewrite = argContext.Define(name, value);
    bindings.push_back(rewrite);
    cache.Cache(name, value);
}


void Bindings::Unwrap()
// ----------------------------------------------------------------------------
//   Unwrap all arguments before calling a builtin or native function
// ----------------------------------------------------------------------------
{
    for (auto &rewrite : Rewrites())
    {
        Tree_p &value = rewrite->right;
        while (Scope *scope = Context::IsClosure(value))
        {
            Prefix *closure = (Prefix *) (Tree *) value;
            value = Evaluate(scope, closure->right);
            if (!value)
                value = LastErrorAsErrorTree();
        }
    }
}



// ============================================================================
//
//   Evaluation entry point
//
// ============================================================================

static Tree *eval(Scope   *evalScope,
                  Scope   *declScope,
                  Tree    *expr,
                  Rewrite *decl,
                  void    *ec,
                  bool     variable)
// ----------------------------------------------------------------------------
//   Bind 'self' to the pattern in 'decl', and if successful, evaluate
// ----------------------------------------------------------------------------
{
    Errors errors;
    errors.Log(Error("Pattern $1 does not match $2:",
                     decl->left, expr), true);

    EvaluationCache &cache = *((EvaluationCache *) ec);
    Tree *pattern = decl->Pattern();

    // If the pattern is a name, directly return the value or fail
    if (Name *name = pattern->AsName())
    {
        if (Name *xname = expr->AsName())
            if (name->value == xname->value)
                return
                    variable
                    ? decl
                    : IsSelf(decl->right)
                    ? name
                    : Interpreter::DoEvaluate(evalScope, decl->right,
                                              Interpreter::STATEMENT, cache);
        return nullptr;
    }

    // Bind argument to parameters
    Bindings bindings(evalScope, declScope, expr, cache);
    if (!pattern->Do(bindings))
        return nullptr;

    // Successfully bound - Return variable declaration in a closure
    if (variable)
        return bindings.Enclose(decl);

    // Evaluate the body in arguments
    Tree *body = decl->right;

    // Check if we have a builtin
    if (Prefix *prefix = body->AsPrefix())
    {
        if (Text *name = IsBuiltin(prefix))
        {
            Interpreter::builtin_fn callee = Interpreter::builtins[name->value];
            if (!callee)
            {
                Ooops("Nonexistent builtin $1", name);
                return nullptr;
            }

            // Need to unwrap all arguments
            bindings.Unwrap();

            Tree_p result = callee(bindings);
            result = bindings.ResultTypeCheck(result, true);
            return result;
        }
        if (Text *name = IsNative(prefix))
        {
            Native *native = Interpreter::natives[name->value];
            if (!native)
            {
                Ooops("Nonexistent native function $1", name);
                return nullptr;
            }

            // Need to unwrap all arguments
            bindings.Unwrap();

            Tree_p result = native->Call(bindings);
            result = bindings.ResultTypeCheck(result, true);
            return result;
        }
    }

    // Check if we evaluate as self
    if (IsSelf(body))
        return expr;

    // Otherwise evaluate the body in the binding arguments scope
    EvaluationCache bodyCache;
    Scope *bodyScope = bindings.ArgumentsScope();
    Tree_p result = Interpreter::DoEvaluate(bodyScope, body,
                                            Interpreter::STATEMENT, bodyCache);
    result = bindings.ResultTypeCheck(result, false);
    return result;
}


static Tree *variable(Scope   *evalScope,
                      Scope   *declScope,
                      Tree    *expr,
                      Rewrite *decl,
                      void    *ec)
// ----------------------------------------------------------------------------
//    Lookup a variable - Return the associated rewrite
// ----------------------------------------------------------------------------
{
    return eval(evalScope, declScope, expr, decl, ec, true);
}


static Tree *constant(Scope   *evalScope,
                      Scope   *declScope,
                      Tree    *expr,
                      Rewrite *decl,
                      void    *ec)
// ----------------------------------------------------------------------------
//    Lookup a constant - Return the associated value
// ----------------------------------------------------------------------------
{
    return eval(evalScope, declScope, expr, decl, ec, false);
}


static Tree_p doDot(Context &context,
                    Infix *infix,
                    EvaluationCache &cache)
// ----------------------------------------------------------------------------
//   Process [X.Y]
// ----------------------------------------------------------------------------
{
    Context child(&context);
    Scope *symbols = child.Symbols();
    Tree_p name = infix->left;
    Tree_p value = infix->right;
    Tree *where = Interpreter::DoEvaluate(symbols, name,
                                          Interpreter::STATEMENT, cache);
    if (Scope *scope = where->As<Scope>())
        return Interpreter::DoEvaluate(scope->Inner(), value,
                                       Interpreter::LOCAL, cache);
    else
        Ooops("No scope in $1 (evaluated as $2)", name, where);
    return nullptr;
}


static Tree_p doAssignment(Scope *scope,
                           Infix *infix,
                           Interpreter::Evaluation mode,
                           EvaluationCache &cache)
// ----------------------------------------------------------------------------
//   Process assignments like [X := Y]
// ----------------------------------------------------------------------------
{
    // Evaluate the target as a variable
    Tree_p decl = Interpreter::DoEvaluate(scope, infix->left,
                                          Interpreter::VARIABLE, cache);
    Tree *to = decl;
    if (!to)
    {
        Ooops("Cannot assign to $1", infix->left);
        return to;
    }

    // Strip closure for expressions like [A(1) := 3]
    while (Closure *closure = to->As<Closure>())
        to = closure->Value();

    // Get the original definition
    Rewrite_p rewrite = to->As<Rewrite>();
    if (!rewrite)
    {
        Ooops("Left of assignment, $1, is not a variable", to);
        return rewrite;
    }

    // Evaluate the assigned value
    Tree_p value = Interpreter::DoEvaluate(scope, infix->right,
                                           Interpreter::EXPRESSION, cache);
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
    return mode == Interpreter::VARIABLE ? decl : value;
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
        Tree_p left = Interpreter::DoEvaluate(scope, expr,
                                              Interpreter::MAYFAIL, cache);
        if (!left || left == expr)
            return nullptr;
        prefix = new Prefix(prefix, left, prefix->right);
        Tree_p result = Interpreter::DoEvaluate(scope, prefix,
                                                Interpreter::EXPRESSION, cache);
        return result;
    }
    return nullptr;
}


Tree *Interpreter::Evaluate(Scope *scope, Tree *expr)
// ----------------------------------------------------------------------------
//    Evaluate 'what', finding the final, non-closure result
// ----------------------------------------------------------------------------
{
    EvaluationCache cache;
    return DoEvaluate(scope, expr, STATEMENT, cache);
}


Tree *Interpreter::TypeCheck(Scope *scope, Tree *type, Tree *value)
// ----------------------------------------------------------------------------
//   Check if 'value' matches 'type' in the given context
// ----------------------------------------------------------------------------
{
    EvaluationCache cache;
    return DoTypeCheck(scope, type, value, cache);
}


Tree *Interpreter::DoEvaluate(Scope *scope,
                              Tree *expr,
                              Evaluation mode,
                              EvaluationCache &cache)
// ----------------------------------------------------------------------------
//   Internal evaluator - Short circuits a few common expressions
// ----------------------------------------------------------------------------
{
    static uint depth = 0;
    const char *modename[EVALUATION_MODES] =
    {
        "statement",
        "expression",
        "may fail",
        "variable",
        "local",
    };
    record(eval, ">%t %+s in %p", expr, modename[mode], scope);

    // Check if there was some error, if so don't keep looking
    if (HadErrors())
    {
        record(eval, "<Aborting %t", Errors::Aborting());
        return Errors::Aborting();
    }

    Tree_p result = expr;
    Context context(scope);

    // Check if we have instructions to process. If not, return declarations
    if (mode == STATEMENT)
    {
        scope = context.CreateScope();
        RewriteList inits;
        bool hasInstructions = context.ProcessDeclarations(expr, inits);
        if (scope->IsEmpty())
            scope = scope->Enclosing();
        if (!DoInitializers(scope, inits, cache))
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
        RewriteList inits;
        if (Scope *closure = context.ProcessScope(prefix->left, inits))
        {
            if (!DoInitializers(closure, inits, cache))
            {
                record(eval, "<Initializers failed for scope %t", prefix->left);
                return nullptr;
            }
            scope = closure;
            expr = prefix->right;
            result = expr;
            context.SetSymbols(scope);
            goto retry;
        }

        // Filter out declaration such as [extern foo(bar)]
        if (IsDefinition(prefix))
        {
            record(eval, "<Prefix %t is a definition", prefix);
            return prefix;
        }

        // Filter out import statements (processed during
        if (Name *import = prefix->left->AsName())
        {
            if (eval_fn callback = MAIN->Declarator(import->value))
            {
                record(eval, "<Import statement %t callback %p",
                       prefix, callback);
                return callback(scope, prefix);
            }
        }
    }

    // Short-circuit evaluation of sequences and declarations
    if (Infix *infix = expr->AsInfix())
    {
        // Process sequences, trying to avoid deep recursion
        if (IsSequence(infix))
        {
            Tree *left = DoEvaluate(scope, infix->left, EXPRESSION, cache);
            if (left != infix->left)
                result = left;
            if (HadErrors() || IsError(left))
            {
                record(eval, "<Errors evaluating first in sequence %t = %t",
                       infix->left, left);
                return left;
            }
            expr = infix->right;
            goto retry;
        }

        // Skip definitions, initalizer have been done above
        if (IsDefinition(infix))
        {
            record(eval, "<Infix %t is a definition", infix);
            return result;
        }

        // Evaluate [X.Y]
        if (IsDot(infix))
        {
            Tree_p scoped = doDot(context, infix, cache);
            record(eval, "<Scoped %t is %t", infix, scoped);
            return scoped;
        }

        // Evaluate assignments such as [X := Y]
        else if (IsAssignment(infix))
        {
            Tree_p assigned = doAssignment(scope, infix, mode, cache);
            record(eval, "<Assignment %t is %t", infix, assigned);
            return assigned;
        }
    }

    // Check stack depth during evaluation
    Save<uint>  save(depth, depth+1);
    if (depth > Opt::stackDepth)
    {
        Errors::Abort(Ooops("Stack depth exceeded evaluating $1", expr));
        record(eval, "<Stack depth exceeded evaluating %t", expr);
        return expr;
    }

    // All other cases: lookup in symbol table
    Context::lookup_fn lookup = mode == VARIABLE ? variable : constant;
    Errors errors;
    errors.Log(Error("Unable to evaluate $1:", expr), true);
    if (Tree *found = context.Lookup(expr, lookup, &cache, mode != LOCAL))
        result = found;
    else if (expr->IsConstant())
        result = expr;
    else if (Prefix *prefix = expr->AsPrefix())
        result = doLatePrefix(scope, prefix, cache);
    else
        result = nullptr;

    // Clear errors if we were successful
    if (!result && mode != MAYFAIL)
        Ooops("Nothing matches $1", expr);
    else if (!Errors::Aborting())
        errors.Clear();

    record(eval, "<%t %+s result %t",
           expr, result ? "succeeded" : "failed", result);

    // Run garbage collector if needed
    GarbageCollector::SafePoint();

    return result;
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


bool Interpreter::DoInitializers(Scope *scope,
                                 RewriteList &inits,
                                 EvaluationCache &cache)
// ----------------------------------------------------------------------------
//   Process all initializers in order
// ----------------------------------------------------------------------------
{
    for (auto rw : inits)
    {
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
        rw->right = init;
    }
    return true;
}



// ============================================================================
//
//    Generate the functions for builtins
//
// ============================================================================

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
    if (!value)                                                 \
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
    if (!left)                                                  \
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
    Tree *value = args[0]; (value);                             \
    Tree *type  = N##_type; (type);                             \
    Body;                                                       \
}

#include "builtins.tbl"



// ============================================================================
//
//   Initialize names and types like xl_nil or natural_type
//
// ============================================================================

void Interpreter::InitializeBuiltins()
// ----------------------------------------------------------------------------
//    Initialize all the names, e.g. xl_nil, and all builtins
// ----------------------------------------------------------------------------
//    This has to be done before the first context is created, because
//    scopes are initialized with xl_nil
{
#define NAME(N)                 xl_##N = new Name(#N);
#define TYPE(N, Body)           N##_type = new Name(#N);
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
//    Initialize all the names, e.g. xl_nil, and all builtins
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
