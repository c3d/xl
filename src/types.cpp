// *****************************************************************************
// types.cpp                                                          XL project
// *****************************************************************************
//
// File description:
//
//     The type system in XL
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010, Catherine Burvelle <catherine@taodyne.com>
// (C) 2009-2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2011-2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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

#include "types.h"
#include "tree.h"
#include "runtime.h"
#include "errors.h"
#include "options.h"
#include "save.h"
#include "compiler-args.h"
#include "cdecls.h"
#include "renderer.h"
#include "basics.h"

#include <iostream>

XL_BEGIN

// ============================================================================
//
//    Type allocation and unification algorithms (hacked Damas-Hilney-Milner)
//
// ============================================================================

uint Types::id= 0;

Types::Types(Scope *scope)
// ----------------------------------------------------------------------------
//   Constructor for top-level type inferences
// ----------------------------------------------------------------------------
    : context(new Context(scope)),
      types(),
      unifications(),
      rcalls(),
      declaration(false),
      codegen(false)
{
    // Pre-assign some types
    types[xl_true] = boolean_type;
    types[xl_false] = boolean_type;
    record(types, "Created Types %p for scope %t", this, scope);
}


Types::Types(Scope *scope, Types *parent)
// ----------------------------------------------------------------------------
//   Constructor for "child" type inferences, i.e. done within a parent
// ----------------------------------------------------------------------------
    : context(new Context(scope)),
      types(parent->types),
      unifications(parent->unifications),
      rcalls(parent->rcalls),
      boxed(parent->boxed),
      declaration(false),
      codegen(false)
{
    context->CreateScope();
    scope = context->CurrentScope();
    record(types, "Created child Types %p scope %t", this, scope);
}


Types::~Types()
// ----------------------------------------------------------------------------
//    Destructor - Nothing to explicitly delete, but useful for debugging
// ----------------------------------------------------------------------------
{
    record(types, "Deleted Types %p", this);
}


Tree *Types::TypeAnalysis(Tree *program)
// ----------------------------------------------------------------------------
//   Perform all the steps of type inference on the given program
// ----------------------------------------------------------------------------
{
    // Once this is done, record all type information for the program
    record(types, "Type analysis for %t in %p", program, this);
    Tree *result = Type(program);
    record(types, "Type for %t in %p is %t", program, this, result);
    codegen = true;

    // Dump debug information if approriate
    if (RECORDER_TRACE(types_ids))
        DumpTypes();
    if (RECORDER_TRACE(types_unifications))
        DumpUnifications();
    if (RECORDER_TRACE(types_calls))
        DumpRewriteCalls();
    if (RECORDER_TRACE(types_boxing))
        DumpMachineTypes();

    return result;
}


Tree *Types::BaseType(Tree *type)
// ----------------------------------------------------------------------------
//   Return the base type associated to the input type
// ----------------------------------------------------------------------------
{
    // Check if we have a type as input and if there is a base type for it
    auto it = unifications.find(type);
    if (it != unifications.end())
        return it->second;
    return type;
}


Tree *Types::KnownType(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the type for the expression if it's already known
// ----------------------------------------------------------------------------
{
    auto it = types.find(expr);
    Tree *type = it != types.end() ? it->second : nullptr;
    record(types_ids, "Existing type for %t in %p is %t", expr, this, type);
    return type;
}


Tree *Types::Type(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the type associated with a given expression
// ----------------------------------------------------------------------------
{
    // Check type, make sure we don't destroy a nullptr type
    Tree *type = KnownType(expr);
    if (type)
        return type;
    if (codegen)
        Ooops("Internal error: No type for $1", expr);
    type = expr->Do(this);
    type = AssignType(expr, type);
    record(types_ids, "Created type for %t in %p is %t", expr, this, type);
    return type;
}


Tree *Types::ValueType(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the type associated with something known to be a value
// ----------------------------------------------------------------------------
{
    Save<bool> save(declaration, false);
    return Type(expr);
}


Tree *Types::DeclarationType(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the type associated with something known to be a declaration
// ----------------------------------------------------------------------------
{
    Save<bool> save(declaration, true);
    return Type(expr);
}


Tree *Types::CodegenType(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the type associated during code generation
// ----------------------------------------------------------------------------
{
    codegen = true;
    return Type(expr);
}


rcall_map & Types::TypesRewriteCalls()
// ----------------------------------------------------------------------------
//   Returns the list of rewrite calls for this
// ----------------------------------------------------------------------------
{
    record(types_calls, "Number of rewrites for %p is %u", this, rcalls.size());
    return rcalls;
}


RewriteCalls *Types::HasRewriteCalls(Tree *what)
// ----------------------------------------------------------------------------
//   Check if we have rewrite calls for this specific tree
// ----------------------------------------------------------------------------
{
    auto it = rcalls.find(what);
    RewriteCalls *result = (it != rcalls.end()) ? it->second : nullptr;
    record(types_calls, "In %p, calls for %t are %p (%u entries)",
           this, what, result, result ? result->candidates.size() : 0);
    return result;
}


Context *Types::TypesContext()
// ----------------------------------------------------------------------------
//   Returns the context where we evaluated the types
// ----------------------------------------------------------------------------
{
    return context;
}


Scope *Types::TypesScope()
// ----------------------------------------------------------------------------
//   Returns the scope where we evaluated the types
// ----------------------------------------------------------------------------
{
    return context->CurrentScope();
}


Tree *Types::Do(Integer *what)
// ----------------------------------------------------------------------------
//   Annotate an integer tree with its value
// ----------------------------------------------------------------------------
{
    return DoConstant(what, INTEGER);
}


Tree *Types::Do(Real *what)
// ----------------------------------------------------------------------------
//   Annotate a real tree with its value
// ----------------------------------------------------------------------------
{
    return DoConstant(what, REAL);
}


Tree *Types::Do(Text *what)
// ----------------------------------------------------------------------------
//   Annotate a text tree with its own value
// ----------------------------------------------------------------------------
{
    return DoConstant(what, TEXT);
}


Tree *Types::DoConstant(Tree *what, kind k)
// ----------------------------------------------------------------------------
//   All constants have themselves as type, and evaluate normally
// ----------------------------------------------------------------------------
{
    Tree *type = what;
    if (context->HasRewritesFor(k))
    {
        type = Evaluate(what);
    }
    else
    {
        type = TypeOf(what);
        type = AssignType(what, type);
    }
    return type;
}


Tree *Types::Do(Name *what)
// ----------------------------------------------------------------------------
//   Assign an unknown type to a name
// ----------------------------------------------------------------------------
{
    Tree *type = nullptr;
    if (declaration)
    {
        type = TypeOf(what);
        if (type)
            context->Define(what, what);
        return type;
    }

    Scope_p scope;
    Rewrite_p rw;
    Tree *body = context->Bound(what, true, &rw, &scope);
    if (body && body != what)
    {
        Tree *defined = RewriteDefined(rw->left);
        if (defined != what)
        {
            if (scope != context->CurrentScope())
                captured[what] = defined;
            text label;
            if (RewriteCategory(rw, defined, label) == Types::Decl::NORMAL)
                type = Type(body);
            else
                type = Type(defined);
            if (RewriteCalls *rc = HasRewriteCalls(defined))
                rcalls[what] = rc;
            else if (RewriteCalls *rc = HasRewriteCalls(body))
                rcalls[what] = rc;
        }
        else
        {
            type = Evaluate(what);
        }
    }
    else
    {
        type = Evaluate(what);
    }

    if (type && rw && rw->left != rw->right)
    {
        Tree *decl = rw->left;
        Tree *def = RewriteDefined(decl);
        if (def != what)
        {
            Tree *rwtype = TypeOfRewrite(rw);
            if (!rwtype)
                return nullptr;
            type = AssignType(decl, type);
            if (def != decl)
                type = AssignType(def, type);
        }
    }

    return type;
}


Tree *Types::Do(Prefix *what)
// ----------------------------------------------------------------------------
//   Assign an unknown type to a prefix and then to its children
// ----------------------------------------------------------------------------
{
    // Deal with bizarre declarations
    if (Name *name = what->left->AsName())
    {
        if (name->value == "extern")
        {
            CDeclaration *cdecl = what->GetInfo<CDeclaration>();
            if (!cdecl)
            {
                Ooops("No C declaration for $1", what);
                return nullptr;
            }
            Tree *type = TypeOfRewrite(cdecl->rewrite);
            return type;
        }
    }

    // For other cases, regular declaration
    return Evaluate(what);
}


Tree *Types::Do(Postfix *what)
// ----------------------------------------------------------------------------
//   Assign an unknown type to a postfix and then to its children
// ----------------------------------------------------------------------------
{
    // No special forms for postfix, try to look it up
    return Evaluate(what);
}


Tree *Types::Do(Infix *what)
// ----------------------------------------------------------------------------
//   Assign type to infix forms
// ----------------------------------------------------------------------------
//   We deal with the following special forms:
//   - [X;Y]: a statement sequence, type is the type of last statement
//   - [X:T] and [X as T]: a type declaration, assign the type T to X
//   - [X is Y]: A declaration, assign a type [type X => type Y]
{
    // For a sequence, both sub-expressions must succeed individually.
    // The type of the sequence is the type of the last statement
    if (what->name == "\n" || what->name == ";")
        return Statements(what, what->left, what->right);

    // Case of [X : T] : Set type of [X] to [T] and unify [X:T] with [X]
    if (what->name == ":" || what->name == "as")
        return TypeDeclaration(what);

    // Case of [X is Y]: Analysis if any will be done during
    if (what->name == "is")
        return TypeOfRewrite(what);

    // For all other cases, evaluate the infix
    return Evaluate(what);
}


Tree *Types::Do(Block *what)
// ----------------------------------------------------------------------------
//   A block evaluates either as itself, or as its child
// ----------------------------------------------------------------------------
{
    Tree *type = Evaluate(what, true);
    if (!type)
    {
        type = Type(what->child);
        if (RewriteCalls *rc = HasRewriteCalls(what->child))
            rcalls[what] = rc;
        type = AssignType(what, type);
    }
    return type;
}


Tree *Types::AssignType(Tree *expr, Tree *type)
// ----------------------------------------------------------------------------
//   Set the type of the expression to be 'type'
// ----------------------------------------------------------------------------
{
    if (type)
    {
        auto it = types.find(expr);
        if (it != types.end())
        {
            Tree *existing = it->second;
            type = Unify(existing, type);
        }
        types[expr] = type;
    }
    return type;
}


Tree *Types::TypeOf(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the type of the expr as a [type X] expression
// ----------------------------------------------------------------------------
{
    // Check if we know a type for this expression, if so return it
    auto it = types.find(expr);
    if (it != types.end())
        return it->second;

    TreePosition pos = expr->Position();
    Tree *type = expr;
    switch(expr->Kind())
    {
    case INTEGER:
        return integer_type;
    case REAL:
        return real_type;
    case TEXT:
        return ((Text *) expr)->IsCharacter() ? character_type : text_type;

    case NAME:
    {
         // Need to build type name by default
        type = nullptr;

        // Lookup original name
        Name *name = (Name *) expr;
        if (name->value == "self")
            if (Tree *declared = context->DeclaredForm(expr))
                if (declared != expr)
                    type = TypeOf(declared);
        break;
    }

    case BLOCK:
        type = TypeOf(((Block *) expr)->child);
        break;

    case PREFIX:
        // Case of [X is C name] or [X is builtin Op]
        if (Prefix *prefix = type->AsPrefix())
        {
            if (Name *name = prefix->left->AsName())
            {
                if (name->value == "C" || name->value == "builtin")
                    type = nullptr;
            }
        }
        /* fall-through */

    case INFIX:
    case POSTFIX:
        if (type)
        {
            TreePosition pos = type->Position();
            type = MakeTypesExplicit(type);
            type = new Prefix(new Name("type", pos), type, pos);
        }
        break;
    }

    // For other names, assign a new generic type name, e.g. #A, #B, #C
    if (!type)
    {
        ulong v = id++;
        text  name;
        do
        {
            name = char('A' + v % 26) + name;
            v /= 26;
        } while (v);
        type = new Name("#" + name, pos);
    }

    // Otherwise, return [type X] and assign it to this expr
    types[expr] = type;
    return type;
}


Tree *Types::MakeTypesExplicit(Tree *expr)
// ----------------------------------------------------------------------------
//   Make the types explicit in a tree shape
// ----------------------------------------------------------------------------
//   For example, if we have [X,Y], based on current known types, we may
//   rewrite this as [X:#A, Y:integer].
{
    switch(expr->Kind())
    {
    case INTEGER:
    case REAL:
    case TEXT:
        return expr;

    case NAME:
    {
        // Replace name with reference type to minimize size of lookup tables
        if (Tree *def = context->DeclaredForm(expr))
            if (Name *name = def->AsName())
                expr = name;

        Tree *type = Type(expr);
        Tree *result = new Infix(":", expr, type, expr->Position());
        return result;
    }

    case BLOCK:
    {
        Block *block = (Block *) expr;
        Tree *child = MakeTypesExplicit(block->child);
        if (child != block->child)
            block = new Block(block, child);
        return block;
    }

    case PREFIX:
    {
        Prefix *prefix = (Prefix *) expr;
        Tree *left = prefix->left->AsName()
            ? (Tree *) prefix->left
            : MakeTypesExplicit(prefix->left);
        Tree *right = MakeTypesExplicit(prefix->right);
        if (left != prefix->left || right != prefix->right)
            prefix = new Prefix(prefix, left, right);
        return prefix;
    }

    case POSTFIX:
    {
        Postfix *postfix = (Postfix *) expr;
        Tree *left = MakeTypesExplicit(postfix->left);
        Tree *right = postfix->right->AsName()
            ? (Tree *) postfix->right
            : MakeTypesExplicit(postfix->right);
        if (left != postfix->left || right != postfix->right)
            postfix = new Postfix(postfix, left, right);
        return postfix;
    }

    case INFIX:
    {
        Infix *infix = (Infix *) expr;
        if (infix->name == ":" || infix->name == "as")
        {
            Tree *right = EvaluateType(infix->right);
            if (right != infix->right)
                infix = new Infix(infix, infix->left, right);
            return infix;
        }
        if (infix->name == "when")
        {
            Tree *left = MakeTypesExplicit(infix->left);
            if (left != infix->left)
                infix = new Infix(infix, left, infix->right);
            return infix;
        }
        Tree *left = MakeTypesExplicit(infix->left);
        Tree *right = MakeTypesExplicit(infix->right);
        if (left != infix->left || right != infix->right)
            infix = new Infix(infix, left, right);
        return infix;
    }
    }
    return NULL; // Get rid of bogus GCC 8.3.1 warning "Control reaches end"
}


Tree *Types::TypeDeclaration(Rewrite *decl)
// ----------------------------------------------------------------------------
//   Explicitely define the type for an expression
// ----------------------------------------------------------------------------
{
    Tree *declared = decl->left;
    Tree *type = EvaluateType(decl->right);
    Tree *declt = TypeOf(declared);
    record(types_ids, "Declaration %t declared %t type %t in %p",
           decl, declared, type, this);
    type = Join(declt, type);
    return declt;
}


Tree *Types::TypeOfRewrite(Rewrite *what)
// ----------------------------------------------------------------------------
//   Assign an [A => B] type to a rewrite
// ----------------------------------------------------------------------------
//   Here, we are processing [X is Y]
//   There are three special cases we want to deal with:
//   - [X is builtin Op]: Check that X has types and that Op exists
//   - [X is C function]: Check that X has types and that function exists
//   - [X is self]: assign [type X] to X
//
//   In other cases, we unify the type of [X] with that of [Y] and
//   return the type [type X => type Y] for the [X is Y] expression.
{
    record(types_calls, "Processing rewrite %t in %p", what, this);

    // Evaluate types for the declaration and the body in a new context
    context->CreateScope(what->Position());
    Tree *decl = what->left;
    Tree *init = what->right;
    Tree *declt = DeclarationType(decl);
    Tree *initt = ValueType(init);
    context->PopScope();

    // Create a [type Decl => type Init] type
    if (!declt || !initt)
    {
        record(types_calls, "Failed type for %t declt=%t initt=%t",
               what, declt, initt);
        return nullptr;
    }

    // Unify with the type of the right hand side
    initt = Unify(declt, initt);
    if (!initt)
        return nullptr;

    // If the left side has a complex shape, e.g. [X+Y], return [DT => I]
    Tree *type = new Infix("=>", declt, initt, what->Position());
    type = AssignType(what, type);

    record(types_calls, "Rewrite for %t is %t (%t => %t)",
           what, type, decl, init);
    return type;
}


Tree *Types::Statements(Tree *expr, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//   Return the type of a combo statement, skipping declarations
// ----------------------------------------------------------------------------
{
    Tree *lt = Type(left);
    if (!lt)
        return lt;

    Tree *rt = Type(right);
    if (!rt)
        return rt;

    // Check if right term is a declaration, otherwise return that
    Tree *type = rt;
    if (IsRewriteType(rt) && !IsRewriteType(lt))
        type = lt;
    type = AssignType(expr, type);
    return type;
}


static Tree *lookupRewriteCalls(Scope *evalScope, Scope *sc,
                                Tree *what, Infix *entry, void *i)
// ----------------------------------------------------------------------------
//   Used to check if RewriteCalls pass
// ----------------------------------------------------------------------------
{
    RewriteCalls *rc = (RewriteCalls *) i;
    return rc->Check(sc, what, entry);
}


Tree *Types::Evaluate(Tree *what, bool mayFail)
// ----------------------------------------------------------------------------
//   Find candidates for the given expression and infer types from that
// ----------------------------------------------------------------------------
{
    record(types_calls, "Evaluating %t in types %p", what, this);
    if (declaration)
        return TypeOf(what);

    // Test if we are already trying to evaluate this particular form
    rcall_map::iterator found = rcalls.find(what);
    bool recursive = found != rcalls.end();
    if (recursive)
        // Need to assign a type name, will be unified by outer Evaluate()
        return TypeOf(what);

    // Identify all candidate rewrites in the current context
    RewriteCalls_p rc = new RewriteCalls(this);
    rcalls[what] = rc;
    uint count = 0;
    Errors errors;
    errors.Log (Error("Unable to evaluate $1:", what), true);
    context->Lookup(what, lookupRewriteCalls, rc);

    // If we have no candidate, this is a failure
    count = rc->candidates.size();
    if (count == 0)
    {
        if (declaration || !mayFail)
            return TypeOf(what);
        return nullptr;
    }
    errors.Clear();
    errors.Log(Error("Unable to check types in $1 because", what), true);

    // The resulting type is the union of all candidates
    Tree *type = rc->candidates[0]->type;
    for (uint i = 1; i < count; i++)
    {
        Tree *ctype = rc->candidates[i]->type;
        type = UnionType(type, ctype);
    }
    type = AssignType(what, type);
    return type;
}


static Tree *lookupType(Scope *evalScope, Scope *sc,
                        Tree *what, Infix *entry, void *i)
// ----------------------------------------------------------------------------
//   Used to check if a
// ----------------------------------------------------------------------------
{
    if (Name *test = what->AsName())
        if (Name *decl = entry->left->AsName())
            if (test->value == decl->value)
                return decl;
    return nullptr;
}


Tree *Types::EvaluateType(Tree *type)
// ----------------------------------------------------------------------------
//   Find candidates for the given expression and infer types from that
// ----------------------------------------------------------------------------
{
    record(types_calls, "Evaluating type %t in types %p", type, this);
    Tree *found = context->Lookup(type, lookupType, nullptr);
    if (found)
        type = Join(type, found);
    return type;
}


Tree *Types::Unify(Tree *t1, Tree *t2)
// ----------------------------------------------------------------------------
//   Unify two type forms
// ----------------------------------------------------------------------------
// Unification happens almost as "usual" for Algorithm W, except for how
// we deal with XL "shape-based" type constructors, e.g. [type P] where
// P is a pattern like [X:integer, Y:real]
{
    // Check if already unified
    if (t1 == t2)
        return t1;

    // Check if one of the sides had a type error
    if (!t1 || !t2)
        return nullptr;

    // Strip out blocks in type specification, i.e. [T] == [(T)]
    if (Block *b1 = t1->AsBlock())
        return Unify(b1->child, t2);
    if (Block *b2 = t2->AsBlock())
        return Unify(t1, b2->child);

    // Check if we have a unification for this type
    auto i1 = unifications.find(t1);
    if (i1 != unifications.end())
        return Unify((*i1).second, t2);
    auto i2 = unifications.find(t2);
    if (i2 != unifications.end())
        return Unify(t1, (*i2).second);

    // Lookup type names, replace them with their value
    t1 = DeclaredTypeName(t1);
    t2 = DeclaredTypeName(t2);
    if (t1 == t2)
        return t1;

    // Success if t1 covers t2 or t2 covers t1
    if (TypeCoversType(t1, t2))
        return Join(t2, t1);
    if (TypeCoversType(t2, t1))
        return Join(t1, t2);

    // Check type patterns, i.e. [type X] as in [type(X:integer, Y:real)]
    if (Tree *pat1 = IsTypeOf(t1))
    {
        // If we have [foo X:#A] and [#A], unify to #A
        if (IsGeneric(t2) && TreePatternDependsOn(pat1, t2))
            return Join(t1, t2);

        // If we have two type patterns, they must be structurally identical
        if (Tree *pat2 = IsTypeOf(t2))
            if (TreePatternsMatch(pat1, pat2))
                return Join(t1, t2);

        // Match a type pattern with another value
        if (TreePatternMatchesValue(pat1, t2))
            return Join(t2, t1);
    }
    if (Tree *pat2 = IsTypeOf(t2))
    {
        // If we have [foo X:#A] and [#A], unify to #A
        if (IsGeneric(t1) && TreePatternDependsOn(pat2, t1))
            return Join(t2, t1);
        if (TreePatternMatchesValue(pat2, t1))
            return Join(t1, t2);
    }

    // If either is a generic, unify with the other
    if (IsGeneric(t1))
        return Join(t1, t2);
    if (IsGeneric(t2))
        return Join(t2, t1);

    // Check functions [X => Y]
    if (Infix *r1 = IsRewriteType(t1))
    {
        if (Infix *r2 = IsRewriteType(t2))
        {
            Tree *ul = Unify(r1->left, r2->left);
            Tree *ur = Unify(r1->right, r2->right);
            if (!ul || !ur)
                return nullptr;
            if (ul == r1->left && ur == r1->right)
                return Join(r2, r1);
            if (ul == r2->left && ur == r2->right)
                return Join(r1, r2);
            Tree *type = new Infix("=>", ul, ur, r1->Position());
            type = Join(r1, type);
            type = Join(r2, type);
            return type;
        }
    }

    // TODO: Range versus range?

    // None of the above: fail
    return TypeError(t1, t2);
}


Tree *Types::IsTypeOf(Tree *type)
// ----------------------------------------------------------------------------
//   Check if type is a type pattern, i.e. type ( ... )
// ----------------------------------------------------------------------------
{
    if (Prefix *pfx = type->AsPrefix())
    {
        if (Name *tname = pfx->left->AsName())
        {
            if (tname->value == "type")
            {
                Tree *pattern = pfx->right;
                if (Block *block = pattern->AsBlock())
                    pattern = block->child;
                return pattern;
            }
        }
    }

    return nullptr;
}


Infix *Types::IsRewriteType(Tree *type)
// ----------------------------------------------------------------------------
//   Check if type is a rewrite type, i.e. something like [X => Y]
// ----------------------------------------------------------------------------
{
    if (Infix *infix = type->AsInfix())
        if (infix->name == "=>")
            return infix;

    return nullptr;
}


Infix *Types::IsRangeType(Tree *type)
// ----------------------------------------------------------------------------
//   Check if type is a range type, i.e. [X..Y] with [X] and [Y] constant
// ----------------------------------------------------------------------------
{
    if (Infix *infix = type->AsInfix())
    {
        if (infix->name == "..")
        {
            kind l = infix->left->Kind();
            kind r = infix->right->Kind();
            if (l == r && (l == INTEGER || l == REAL || l == TEXT))
                return infix;
        }
    }

    return nullptr;
}


Infix *Types::IsUnionType(Tree *type)
// ----------------------------------------------------------------------------
//   Check if type is a union type, i.e. something like [integer|real]
// ----------------------------------------------------------------------------
{
    if (Infix *infix = type->AsInfix())
        if (infix->name == "|")
            return infix;

    return nullptr;
}


Tree *Types::Join(Tree *old, Tree *replace)
// ----------------------------------------------------------------------------
//   Replace the old type with the new one
// ----------------------------------------------------------------------------
{
    // Deal with error cases
    if (!old || !replace)
        return nullptr;
    if (old == replace)
        return old;

    // Replace the type in the types map
    for (auto &t : types)
    {
        Tree *joined = JoinedType(t.second, old, replace);
        if (joined != t.second)
        {
            Tree *original = t.second;
            t.second = joined;
            Join(original, joined);
        }
    }

    // Replace the type in the 'unifications' map
    for (auto &u : unifications)
        if (u.second == old)
            u.second = replace;
    unifications.erase(replace);
    unifications[old] = replace;

    // Replace the type in the rewrite calls
    for (auto &r : rcalls)
        for (RewriteCandidate *rc : r.second->candidates)
            if (rc->type == old)
                rc->type = replace;

    return replace;
}


RECORDER_DEFINE(joined_type, 64, "Joined types");

Tree *Types::JoinedType(Tree *type, Tree *old, Tree *replace)
// ----------------------------------------------------------------------------
//   Build a type after joining, in case that's necessary
// ----------------------------------------------------------------------------
{
    record(joined_type, "Replace %t with %t in %t", old, replace, type);
    XL_REQUIRE (type != NULL);
    XL_REQUIRE (old != NULL);
    XL_REQUIRE (replace != NULL);

    if (type == old || type == replace)
        return replace;

    switch (type->Kind())
    {
    case INTEGER:
    case REAL:
    case TEXT:
    case NAME:
        return type;

    case BLOCK:
    {
        Block *block = (Block *) type;
        Tree *child = JoinedType(block->child, old, replace);
        XL_ASSERT (child);
        if (child != block->child)
            return new Block(block, child);
        return block;
    }

    case PREFIX:
    {
        Prefix *prefix = (Prefix *) type;
        Tree *left = JoinedType(prefix->left, old, replace);
        Tree *right = JoinedType(prefix->right, old, replace);
        XL_ASSERT (left && right);
        if (left != prefix->left || right != prefix->right)
            return new Prefix(prefix, left, right);
        return prefix;
    }

    case POSTFIX:
    {
        Postfix *postfix = (Postfix *) type;
        Tree *left = JoinedType(postfix->left, old, replace);
        Tree *right = JoinedType(postfix->right, old, replace);
        XL_ASSERT (left && right);
        if (left != postfix->left || right != postfix->right)
            return new Postfix(postfix, left, right);
        return postfix;
    }

    case INFIX:
    {
        Infix *infix = (Infix *) type;
        Tree *left = JoinedType(infix->left, old, replace);
        Tree *right = JoinedType(infix->right, old, replace);
        XL_ASSERT (left && right);
        if (left != infix->left || right != infix->right)
            return new Infix(infix, left, right);
        return infix;
    }

    }
    return NULL; // Get rid of bogus GCC 8.3.1 warning "Control reaches end"
}


Tree *Types::UnionType(Tree *t1, Tree *t2)
// ----------------------------------------------------------------------------
//    Create the union of two types
// ----------------------------------------------------------------------------
{
    if (t1 == t2)
        return t1;
    if (!t1 || !t2)
        return nullptr;

    if (TypeCoversType(t1, t2))
        return t1;
    if (TypeCoversType(t2, t1))
        return t2;

    // TODO: Union of range types and constants

    return new Infix("|", t1, t2, t1->Position());

}


bool Types::TypeCoversConstant(Tree *type, Tree *cst)
// ----------------------------------------------------------------------------
//    Check if a type covers a constant or range
// ----------------------------------------------------------------------------
{
    // Deal with type errors
    if (!type || !cst)
        return false;

    // If the type is something like 0..3, set 'range' to that range
    Infix *range = IsRangeType(type);

    // Check if we match against some sized type, otherwise force type
    if (Integer *icst = cst->AsInteger())
    {
        if (type == integer_type   || type == unsigned_type   ||
            type == integer8_type  || type == unsigned8_type  ||
            type == integer16_type || type == unsigned16_type ||
            type == integer32_type || type == unsigned32_type ||
            type == integer64_type || type == unsigned64_type)
            return true;
        if (range)
            if (Integer *il = range->left->AsInteger())
                if (Integer *ir = range->right->AsInteger())
                    return
                        icst->value >= il->value &&
                        icst->value <= ir->value;
        return false;
    }

    if (Real *rcst = cst->AsReal())
    {
        if (type == real_type   ||
            type == real64_type ||
            type == real32_type)
            return true;
        if (range)
            if (Real *rl = range->left->AsReal())
                if (Real *rr = range->right->AsReal())
                    return
                        rcst->value >= rl->value &&
                        rcst->value <= rr->value;
        return false;
    }

    if (Text *tcst = cst->AsText())
    {
        bool isChar = tcst->IsCharacter();
        if ((isChar  && type == character_type) ||
            (!isChar && type == text_type))
            return true;

        if (range)
            if (Text *tl = range->left->AsText())
                if (Text *tr = range->right->AsText())
                    return
                        tl->IsCharacter() == isChar &&
                        tr->IsCharacter() == isChar &&
                        tcst->value >= tl->value &&
                        tcst->value <= tr->value;
        return false;
    }
    return false;
}


bool Types::TypeCoversType(Tree *top, Tree *bottom)
// ----------------------------------------------------------------------------
//   Check if the top type covers all values in the bottom type
// ----------------------------------------------------------------------------
{
    // Deal with type errors
    if (!top || !bottom)
        return false;

    // Quick exit when types are the same or the tree type is used
    if (top == bottom)
        return true;
    if (top == tree_type)
        return true;
    if (Infix *u = IsUnionType(top))
        if (TypeCoversType(u->left, bottom) || TypeCoversType(u->right, bottom))
            return true;
    if (TypeCoversConstant(top, bottom))
        return true;

    // Failed to match type
    return false;
}


bool Types::TreePatternsMatch(Tree *t1, Tree *t2)
// ----------------------------------------------------------------------------
//   Check if two patterns describe the same tree shape
// ----------------------------------------------------------------------------
//   Patterns have to match exactly, i.e. [X:integer] matches [X:integer]
//   but neither [Y:integer] nor [X:0]
{
    if (!t1 || !t2)
        return false;
    if (t1 == t2)
        return true;

    switch(t1->Kind())
    {
    case INTEGER:
        if (Integer *x1 = t1->AsInteger())
            if (Integer *x2 = t2->AsInteger())
                return x1->value == x2->value;
        return false;

    case REAL:
        if (Real *x1 = t1->AsReal())
            if (Real *x2 = t2->AsReal())
                return x1->value == x2->value;
        return false;

    case TEXT:
        if (Text *x1 = t1->AsText())
            if (Text *x2 = t2->AsText())
                return x1->value == x2->value;
        return false;

    case NAME:
        // We don't attempt to allow renames. Names must match, it's simpler.
        if (Name *x1 = t1->AsName())
            if (Name *x2 = t2->AsName())
                return x1->value == x2->value;
        return false;

    case INFIX:
        if (Infix *x1 = t1->AsInfix())
            if (Infix *x2 = t2->AsInfix())
                if (x1->name == x2->name &&
                    TreePatternsMatch(x1->left, x2->left) &&
                    TreePatternsMatch(x1->right, x2->right))
                    return true;
        return false;

    case PREFIX:
        if (Prefix *x1 = t1->AsPrefix())
            if (Prefix *x2 = t2->AsPrefix())
                return
                    TreePatternsMatch(x1->left, x2->left) &&
                    TreePatternsMatch(x1->right, x2->right);

        return false;

    case POSTFIX:
        if (Postfix *x1 = t1->AsPostfix())
            if (Postfix *x2 = t2->AsPostfix())
                return
                    TreePatternsMatch(x1->left, x2->left) &&
                    TreePatternsMatch(x1->right, x2->right);

        return false;

    case BLOCK:
        if (Block *x1 = t1->AsBlock())
            if (Block *x2 = t2->AsBlock())
                return
                    x1->opening == x2->opening &&
                    x1->closing == x2->closing &&
                    TreePatternsMatch(x1->child, x2->child);

        return false;

    }

    return false;
}


bool Types::TreePatternMatchesValue(Tree *pat, Tree *val)
// ----------------------------------------------------------------------------
//   Check if the pattern matches some tree value
// ----------------------------------------------------------------------------
{
    if (!pat || !val)
        return false;
    if (pat == val)             // May occur as [type X] vs [X]
        return true;

    switch(pat->Kind())
    {
    case INTEGER:
        if (Integer *x1 = pat->AsInteger())
            if (Integer *x2 = val->AsInteger())
                return x1->value == x2->value;
        return false;

    case REAL:
        if (Real *x1 = pat->AsReal())
            if (Real *x2 = val->AsReal())
                return x1->value == x2->value;
        return false;

    case TEXT:
        if (Text *x1 = pat->AsText())
            if (Text *x2 = val->AsText())
                return x1->value == x2->value;
        return false;

    case NAME:
        // A name at that stage is a variable, so we match
        // PROBLEM: matching X+X will match twice?
        return Unify(pat, Type(val));

    case INFIX:
        if (Infix *x1 = pat->AsInfix())
        {
            // Check if the pattern is a type declaration
            if (x1->name == ":")
                return Unify(x1->right, Type(val));

            if (Infix *x2 = val->AsInfix())
                return
                    x1->name == x2->name &&
                    TreePatternMatchesValue(x1->left, x2->left) &&
                    TreePatternMatchesValue(x1->right, x2->right);
        }

        return false;

    case PREFIX:
        if (Prefix *x1 = pat->AsPrefix())
            if (Prefix *x2 = val->AsPrefix())
                return
                    TreePatternsMatch(x1->left, x2->left) &&
                    TreePatternMatchesValue(x1->right, x2->right);

        return false;

    case POSTFIX:
        if (Postfix *x1 = pat->AsPostfix())
            if (Postfix *x2 = val->AsPostfix())
                return
                    TreePatternMatchesValue(x1->left, x2->left) &&
                    TreePatternsMatch(x1->right, x2->right);

        return false;

    case BLOCK:
        if (Block *x1 = pat->AsBlock())
            if (Block *x2 = val->AsBlock())
                return
                    x1->opening == x2->opening &&
                    x1->closing == x2->closing &&
                    TreePatternMatchesValue(x1->child, x2->child);

        return false;

    }

    return false;
}


bool Types::TreePatternDependsOn(Tree *pattern, Tree *type)
// ----------------------------------------------------------------------------
//   Check if we have something like [foo X:#A] that depends on [#A]
// ----------------------------------------------------------------------------
{
    if (pattern == type)
        return true;
    switch (pattern->Kind())
    {
    case INTEGER:
    case REAL:
    case TEXT:
    case NAME:
        return false;

    case BLOCK:
    {
        Block *block = (Block *) pattern;
        return TreePatternDependsOn(block->child, type);

    }
    case PREFIX:
    {
        Prefix *prefix = (Prefix *) pattern;
        return (TreePatternDependsOn(prefix->left, type) ||
                TreePatternDependsOn(prefix->right, type));

    }
    case POSTFIX:
    {
        Postfix *postfix = (Postfix *) pattern;
        return (TreePatternDependsOn(postfix->left, type) ||
                TreePatternDependsOn(postfix->right, type));

    }
    case INFIX:
    {
        Infix *infix = (Infix *) pattern;
        return (TreePatternDependsOn(infix->left, type) ||
                TreePatternDependsOn(infix->right, type));

    }

    }
    return false; // Get rid of bogus GCC 8.3.1 warning "Control reaches end"
}


Tree *Types::DeclaredTypeName(Tree *type)
// ----------------------------------------------------------------------------
//   If we have a type name, lookup its definition
// ----------------------------------------------------------------------------
{
    if (Name *name = type->AsName())
    {
        // Don't lookup type variables (generic names such as #A)
        if (IsGeneric(name->value))
            return name;

        // Check if we have a type definition. If so, use it
        Rewrite_p rewrite;
        Scope_p scope;
        Tree *definition = context->Bound(name, true, &rewrite, &scope);
        if (definition && definition != name)
        {
            type = Join(type, definition);
            type = Join(type, rewrite->left);
        }
    }

    // By default, return input type
    return type;
}


Types::Decl Types::RewriteCategory(RewriteCandidate *rc)
// ----------------------------------------------------------------------------
//   Check if the rewrite candidate is of a special kind
// ----------------------------------------------------------------------------
{
    return RewriteCategory(rc->rewrite, rc->defined, rc->defined_name);
}


Types::Decl Types::RewriteCategory(Rewrite *rw, Tree *defined, text &label)
// ----------------------------------------------------------------------------
//   Check if the declaration is of a special kind
// ----------------------------------------------------------------------------
{
    Types::Decl decl = Types::Decl::NORMAL;
    Tree *body = rw->right;

    // Case of [sin X is C]: Use the name 'sin'
    if (Name *bodyname = body->AsName())
    {
        if (bodyname->value == "C")
            if (Name *defname = defined->AsName())
                if (IsValidCName(defname, label))
                    decl = Types::Decl::C;
        if (bodyname->value == "self")
            decl = Types::Decl::DATA;
    }

    // Case of [alloc X is C "_malloc"]: Use "_malloc"
    if (Prefix *prefix = body->AsPrefix())
    {
        if (Name *name = prefix->left->AsName())
        {
            if (name->value == "C")
                if (IsValidCName(prefix->right, label))
                    decl = Types::Decl::C;
            if (name->value == "builtin")
                decl = Types::Decl::BUILTIN;
        }
    }

    return decl;
}


bool Types::IsValidCName(Tree *tree, text &label)
// ----------------------------------------------------------------------------
//   Check if the name is valid for C
// ----------------------------------------------------------------------------
{
    uint len = 0;

    if (Name *name = tree->AsName())
    {
        label = name->value;
        len = label.length();
    }
    else if (Text *text = tree->AsText())
    {
        label = text->value;
        len = label.length();
    }

    if (len == 0)
    {
        Ooops("No valid C name in $1", tree);
        return false;
    }

    // We will NOT call functions beginning with _ (internal functions)
    for (uint i = 0; i < len; i++)
    {
        char c = label[i];
        if (!isalpha(c) && c != '_' && !(i && isdigit(c)))
        {
            Ooops("C name $1 contains invalid characters", tree);
            return false;
        }
    }
    return true;
}


Tree *Types::TypeError(Tree *t1, Tree *t2)
// ----------------------------------------------------------------------------
//   Show type matching errors
// ----------------------------------------------------------------------------
{
    Tree *x1 = nullptr;
    Tree *x2 = nullptr;
    for (auto &t : types)
    {
        if (t.second == t1)
        {
            x1 = t.first;
            if (x2)
                break;
        }
        if (t.second == t2)
        {
            x2 = t.first;
            if (x1)
                break;
        }
    }

    if (x1 == x2)
    {
        if (x1)
            Ooops("Type of $1 cannot be both $2 and $3", x1, t1, t2);
        else
            Ooops("Cannot unify type $2 and $1", t1, t2);
    }
    else
    {
        if (x1)
            Ooops("Cannot unify type $2 of $1", x1, t1);
        else
            Ooops("Cannot unify type $1", t1);
        if (x2)
            Ooops("with type $2 of $1", x2, t2);
        else
            Ooops("with type $1", t2);
    }

    return nullptr;
}



// ============================================================================
//
//   Boxed type management
//
// ============================================================================

void Types::AddBoxedType(Tree *type, Type_p mtype)
// ----------------------------------------------------------------------------
//   Associate a tree type to a boxed machine type
// ----------------------------------------------------------------------------
///  The tree type could be a named type, e.g. [integer], or data, e.g. [X,Y]
//   The machine type could be integerTy or StructType({integerTy, realTy})
{
    Tree *base = BaseType(type);
    record(types_boxing, "Add %T boxing %t (%t)", mtype, type, base);
    assert(!boxed[base] || boxed[base] == mtype);
    boxed[base] = mtype;
}


Type_p Types::BoxedType(Tree *type)
// ----------------------------------------------------------------------------
//   Return the boxed type if there is one
// ----------------------------------------------------------------------------
{
    // Check if we already had this signature
    Tree *base = BaseType(type);
    auto it = boxed.find(base);
    Type_p mtype = (it != boxed.end()) ? it->second : nullptr;
    record(types_boxing, "Type %T is boxing %t (%t)", mtype, type, base);
    return mtype;
}



// ============================================================================
//
//   Debug utilities
//
// ============================================================================

void Types::DumpTypes()
// ----------------------------------------------------------------------------
//   Dump the list of types in this
// ----------------------------------------------------------------------------
{
    uint i = 0;

    std::cout << "TYPES:\n";
    for (auto &t : types)
    {
        Tree *value = t.first;
        Tree *type = t.second;
        Tree *base = BaseType(type);
        std::cout << "#" << ++i
                  << "\t" << ShortTreeForm(value)
                  << " (" << (void *) value << ")"
                  << "\t: " << type
                  << " (" << (void *) type << ")";
        if (base != type)
            std::cout << "\t= " << base << " (" << (void *) base << ")";
        std::cout << "\n";
    }
}


void Types::DumpMachineTypes()
// ----------------------------------------------------------------------------
//   Dump the list of machine types in this
// ----------------------------------------------------------------------------
{
    uint i = 0;
    Save<intptr_t> saveTrace(RECORDER_TRACE(types_boxing), 0);

    std::cout << "MACHINE TYPES:\n";
    for (auto &b : boxed)
    {
        Tree *type = b.first;
        Type_p mtype = b.second;
        std::cout << "#" << ++i
                  << "\t" << type;
        JIT::Print("\t= ", mtype);
        std::cout << "\n";
    }
}


void Types::DumpUnifications()
// ----------------------------------------------------------------------------
//   Dump the current unifications
// ----------------------------------------------------------------------------
{
    uint i = 0;
    std::cout << "UNIFICATIONS:\n";
    for (auto &t : unifications)
    {
        Tree *type = t.first;
        Tree *base = t.second;
        std::cout << "#" << ++i
                  << "\t" << type << " (" << (void *) type << ")"
                  << "\t= " << base << " (" << (void *) base << ")\n";
    }
}


void Types::DumpRewriteCalls()
// ----------------------------------------------------------------------------
//   Dump the list of rewrite calls
// ----------------------------------------------------------------------------
{
    uint i = 0;
    std::cout << "CALLS:\n";
    for (auto &t : rcalls)
    {
        Tree *expr = t.first;
        std::cout << "#" << ++i << "\t"
                  << ShortTreeForm(expr) << " (" << (void *) expr << ")\n";

        RewriteCalls *calls = t.second;
        calls->Dump();
    }
}

XL_END


XL::Types *xldebug(XL::Types *ti)
// ----------------------------------------------------------------------------
//   Dump a type inference
// ----------------------------------------------------------------------------
{
    if (!XL::Allocator<XL::Types>::IsAllocated(ti))
    {
        std::cout << "Cowardly refusing to show bad Types pointer "
                  << (void *) ti << "\n";
    }
    else
    {
        ti->DumpRewriteCalls();
        ti->DumpUnifications();
        ti->DumpTypes();
        ti->DumpMachineTypes();
    }
    return ti;
}
XL::Types *xldebug(XL::Types_p ti)      { return xldebug((XL::Types *) ti); }

XL::Tree *xldebug(XL::Tree *);
XL::Context *xldebug(XL::Context *);
XL::RewriteCalls *xldebug(XL::RewriteCalls *);
XL::RewriteCandidate *xldebug(XL::RewriteCandidate *);


void *xldebug(uintptr_t address)
// ----------------------------------------------------------------------------
//   Debugger entry point to debug a garbage-collected pointer
// ----------------------------------------------------------------------------
{
    void *ptr = (void *) address;

#define CHECK_ALLOC(T)                                  \
    if (XL::Allocator<XL::T>::IsAllocated(ptr))         \
    {                                                   \
        std::cout << "Pointer " << ptr                  \
                  << " appears to be a " #T "\n";       \
        return xldebug((XL::T *) ptr);                  \
    }

    CHECK_ALLOC(Integer);
    CHECK_ALLOC(Real);
    CHECK_ALLOC(Text);
    CHECK_ALLOC(Name);
    CHECK_ALLOC(Block);
    CHECK_ALLOC(Prefix);
    CHECK_ALLOC(Postfix);
    CHECK_ALLOC(Infix);
    CHECK_ALLOC(Types);
    CHECK_ALLOC(Context);
    CHECK_ALLOC(RewriteCalls);
    CHECK_ALLOC(RewriteCandidate);

    return XL::GarbageCollector::DebugPointer(ptr);
}


RECORDER(types,                 64, "Type analysis");
RECORDER(types_ids,             64, "Assigned type identifiers");
RECORDER(types_unifications,    64, "Type unifications");
RECORDER(types_calls,           64, "Type deductions in rewrites (calls)");
RECORDER(types_boxing,          64, "Machine type boxing and unboxing");
