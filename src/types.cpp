// ****************************************************************************
//  types.cpp                                                        XL project
// ****************************************************************************
//
//   File Description:
//
//    Common type analysis for XL programs
//
//    The Types class holds the implementation-independant portions of the type
//    analysis for XL, i.e. the part that is the same for the interpreter and
//    the compiler.
//
//
//
//
// ****************************************************************************
//   (C) 2020 Christophe de Dinechin <christophe@dinechin.org>
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

#include "types.h"
#include "errors.h"
#include "basics.h"
#include "cdecls.h"


XL_BEGIN

Types::Types(Scope *scope)
// ----------------------------------------------------------------------------
//   Constructor for top-level type inferences
// ----------------------------------------------------------------------------
    : context(new Context(scope)), parent(nullptr)
{
    record(types, "Created Types %p for scope %t", this, scope);
}


Types::Types(Scope *scope, Types *parent)
// ----------------------------------------------------------------------------
//   Constructor for "child" type inferences, i.e. done within a parent
// ----------------------------------------------------------------------------
    : context(new Context(scope)), types(), parent(parent)
{
    context->CreateScope();
    scope = context->Symbols();
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
    bool hasInstructions = context->ProcessDeclarations(program);
    Tree *result = hasInstructions ? Type(program) : declaration_type;
    record(types, "Type for %t in %p is %t", program, this, result);

    // Dump debug information if approriate
    if (RECORDER_TRACE(types))
        Dump();

    return result;
}


Tree *Types::Type(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the type associated with a given expression
// ----------------------------------------------------------------------------
{
    // First check if we already computed the type for this expression
    Tree *type = KnownType(expr);
    if (type)
    {
        record(types, "In %p cached type for %t is %t", this, expr, type);
        return type;
    }

    // Apply the rules corresponding to each tree
    record(types, "In %p searching type for %t", this, expr);
    type = expr->Do(this);
    record(types, "In %p type for %t is %t", this, expr, type);
    return type;
}



// ============================================================================
//
//    Tree::Do interface to
//
// ============================================================================

Tree *Types::Do(Integer *what)
// ----------------------------------------------------------------------------
//   Annotate an integer tree with its type - normally [integer]
// ----------------------------------------------------------------------------
{
    return DoConstant(what, integer_type, INTEGER);
}


Tree *Types::Do(Real *what)
// ----------------------------------------------------------------------------
//   Annotate a real tree with its type - normally [real]
// ----------------------------------------------------------------------------
{
    return DoConstant(what, real_type, REAL);
}


Tree *Types::Do(Text *what)
// ----------------------------------------------------------------------------
//   Annotate a text tree with its type - normally [text] or [character]
// ----------------------------------------------------------------------------
{
    Tree * type = what->IsCharacter() ? character_type : text_type;
    return DoConstant(what, type, TEXT);
}


Tree *Types::Do(Name *what)
// ----------------------------------------------------------------------------
//   Assign an unknown type to a name
// ----------------------------------------------------------------------------
{
    record(types_ids, "In %p type of name %t", this, what);

    // Lookup the name
    Tree *type = Evaluate(what);
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
                return xl_error;
            }
            Tree *type = DoRewrite(cdecl->rewrite);
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
    if (IsSequence(what))
        return DoStatements(what, what->left, what->right);

    // Case of [X : T] : Set type of [X] to [T]
    if (IsTypeAnnotation(what))
        return DoTypeAnnotation(what);

    // Case of [X is Y]: Analysis if any will be done during evaluation
    if (IsDefinition(what))
        return DoRewrite(what);

    // For all other cases, evaluate the infix
    return Evaluate(what);
}


Tree *Types::Do(Block *what)
// ----------------------------------------------------------------------------
//   A block evaluates as its child
// ----------------------------------------------------------------------------
{
    Tree *type = Type(what->child);
    type = AssignType(what, type);
    return type;
}


// ============================================================================
//
//   Expression evaluation during type check
//
// ============================================================================

Tree *Types::DoConstant(Tree *what, Tree *base, kind k)
// ----------------------------------------------------------------------------
//   All constants have themselves as type, and evaluate normally
// ----------------------------------------------------------------------------
{
    Tree *type = nullptr;

    // Check if we need to evaluate rewrites for this kind in the context
    // This would happen for something like [0 is 1; 0]
    bool evaluate = context->HasRewritesFor(k);
    if (evaluate)
        type = Evaluate(what, true);

    // For real constants or if we failed to evaluate, assign basic type
    if (!type)
        type = base;

    // Preserve types that were expensive to compute
    if (evaluate)
        type = AssignType(what, type);

    return type;
}


Tree *Types::DoStatements(Tree *expr, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//   Process sequences of statements
// ----------------------------------------------------------------------------
{
    // We always process left and right to find as many errors as possible
    Tree *lt = Type(left);
    Tree *rt = Type(right);

    // If there was any error, report it
    if (!lt || !rt)
        return xl_error;

    // Return type of last non-declaration statement
    if (rt == declaration_type)
        return lt;
    return rt;
}


Tree *Types::DoTypeAnnotation(Infix *rewrite)
// ----------------------------------------------------------------------------
//   All declarations evaluate as themselves
// ----------------------------------------------------------------------------
{
    record(types, "Type annotation %t", rewrite);
    Tree *value = rewrite->left;
    Tree *type = rewrite->right;
    TypeEvaluator typeEvaluator(this);
    type = type->Do(typeEvaluator);
    AssignType(value, type);
    return declaration_type;
}


Tree *Types::DoRewrite(Infix *rewrite)
// ----------------------------------------------------------------------------
//   All declarations evaluate as themselves
// ----------------------------------------------------------------------------
{
    record(types, "Rewrite %t evaluates as declaration", rewrite);
    return declaration_type;
}


static Tree *lookupRewriteCalls(Scope *evalScope, Scope *sc,
                                Tree *what, Infix *entry, void *i)
// ----------------------------------------------------------------------------
//   Used to check if RewriteCalls pass
// ----------------------------------------------------------------------------
{
#if 0
    RewriteCalls *rc = (RewriteCalls *) i;
    return rc->Check(sc, what, entry);
#else
    return nullptr;
#endif
}


Tree *Types::Evaluate(Tree *what, bool mayFail)
// ----------------------------------------------------------------------------
//   Find candidates for the given expression and infer types from that
// ----------------------------------------------------------------------------
{
    record(types_calls, "In %p evaluating %t", this, what);

#if 0
    // Test if we are already trying to evaluate this particular pattern
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
#else
    Errors errors;
    errors.Log (Error("Unable to evaluate $1:", what), true);
    context->Lookup(what, lookupRewriteCalls, nullptr);
    return nullptr;
#endif
}



// ============================================================================
//
//   Type properties
//
// ============================================================================

Tree *Types::AssignType(Tree *expr, Tree *type, bool add)
// ----------------------------------------------------------------------------
//   Assign the given type to the expression
// ----------------------------------------------------------------------------
{
    // Check if what we want to assign is compatible with what is known
    Tree *known = KnownType(expr);
    if (known)
    {
        Tree *match = TypeMatchesType(type, known);
        if (!match)
        {
            if (!add)
            {
                Ooops("Type conflict for $1", expr);
                Ooops("  already known to have type $1", known);
                Ooops("  incompatible with type $1", type);
                return xl_error;
            }

            // Create a union type and insert that
            type = UnionType(known, type);
            types[expr] = type;
            return type;
        }
        return match;
    }

    types[expr] = type;
    return type;
}


Tree *Types::KnownType(Tree *expr, bool recurse)
// ----------------------------------------------------------------------------
//   Check if there is a type already recorded for this expression
// ----------------------------------------------------------------------------
{
    for (Types *ts = this; ts; ts = recurse ? ts->parent : nullptr)
    {
        auto it = ts->types.find(expr);
        if (it != ts->types.end())
            return it->second;
    }
    return nullptr;
}


Tree *Types::ExprMatchesType(Tree *expr, Tree *type)
// ----------------------------------------------------------------------------
//   Check if the expression matches some existing type condition
// ----------------------------------------------------------------------------
{
    Tree *known = KnownType(expr);
    if (known)
        return TypeMatchesType(type, known);
    return nullptr;
}


Tree *Types::TypeMatchesType(Tree *type, Tree *known)
// ----------------------------------------------------------------------------
//   Return the innermost type matching the expr
// ----------------------------------------------------------------------------
//   REVISIT: We hard-code the name and processing of some type operators here
{
    // Direct match: stop here
    if (known == type)
        return known;

    // Check if we need to follow aliases
    auto typeAlias = aliases.find(type);
    if (typeAlias != aliases.end())
        type = typeAlias->second;
    auto knownAlias = aliases.find(known);
    if (knownAlias != aliases.end())
        known = knownAlias->second;

    // Check if following aliases gave us a match
    if (known == type)
        return known;

    // Lookup names and check if they match after lookup
    if (Name *name = known->AsName())
    {
        Tree *base = context->Bound(name);
        Tree *bound = context->Bound(type);
        if (bound == base)
            return base;
    }

    // Check some type infix operators - REVISIT HARDCODED
    if (Infix *infix = known->AsInfix())
    {
        if (infix->name == "or")
        {
            Tree *left = TypeMatchesType(type, infix->left);
            if (left)
                return left;
            Tree *right = TypeMatchesType(type, infix->right);
            if (right)
                return right;
        }
        else if (infix->name == "and")
        {
            Tree *left = TypeMatchesType(type, infix->left);
            if (left)
            {
                Tree *right = TypeMatchesType(type, infix->right);
                if (right)
                    return known;
            }
        }
        else if (infix->name == "xor")
        {
            Tree *left = TypeMatchesType(type, infix->left);
            Tree *right = TypeMatchesType(type, infix->right);
            if ((left == nullptr) != (right == nullptr))
                return known;
        }
    }

    // Check some type prefix operators - REVISIT HARDCODED
    if (Prefix *prefix = known->AsPrefix())
    {
        if (Name *opcode = prefix->left->AsName())
        {
            if (opcode->value == "not")
            {
                Tree *right = TypeMatchesType(type, prefix->right);
                if (!right)
                    return known;
            }
        }
    }


    // Check infix operators for the type - REVISIT HARDCODED
    if (Infix *infix = type->AsInfix())
    {
        if (infix->name == "or")
        {
            Tree *left = TypeMatchesType(infix->left, known);
            if (left)
                return left;
            Tree *right = TypeMatchesType(infix->right, known);
            if (right)
                return right;
        }
        else if (infix->name == "and")
        {
            Tree *left = TypeMatchesType(infix->left, known);
            if (left)
            {
                Tree *right = TypeMatchesType(infix->right, known);
                if (right)
                    return known;
            }
        }
        else if (infix->name == "xor")
        {
            Tree *left = TypeMatchesType(infix->left, known);
            Tree *right = TypeMatchesType(infix->right, known);
            if ((left == nullptr) != (right == nullptr))
                return known;
        }
    }

    // Check some type prefix operators - REVISIT HARDCODED
    if (Prefix *prefix = type->AsPrefix())
    {
        if (Name *opcode = prefix->left->AsName())
        {
            if (opcode->value == "not")
            {
                Tree *right = TypeMatchesType(prefix->right, known);
                if (!right)
                    return known;
            }
        }
    }

    return nullptr;
}


Tree *Types::AliasType(Tree *alias, Tree *base)
// ----------------------------------------------------------------------------
//   Record an alias as being equivalent for the base type
// ----------------------------------------------------------------------------
{
    alias = BaseType(alias);
    base = BaseType(base);
    if (alias != base)
        aliases[alias] = base;
    return base;
}


Tree *Types::UnionType(Tree *t1, Tree *t2)
// ----------------------------------------------------------------------------
//  Create a union type between t1 and t2
// ----------------------------------------------------------------------------
{
    return new Infix("or", t1, t2);
}


Tree *Types::BaseType(Tree *alias)
// ----------------------------------------------------------------------------
//   Follow the chain of aliases and find base type
// ----------------------------------------------------------------------------
{
    for(;;)
    {
        auto found = aliases.find(alias);
        if (found != aliases.end())
            break;
        alias = found->second;
    }
    return alias;
}



// ============================================================================
//
//   Debug utilities
//
// ============================================================================

void Types::Dump()
// ----------------------------------------------------------------------------
//   Dump the list of types in this
// ----------------------------------------------------------------------------
{
    for (Types *types = this; types; types = types->parent)
    {
        std::cout << "TYPES " << (void *) types << ":\n";
        for (auto &t : types->types)
        {
            Tree *value = t.first;
            Tree *type = t.second;
            std::cout << "\t" << ShortTreeForm(value)
                      << "\t: " << type
                      << "\n";
        }
    }
}



// ============================================================================
//
//   TypeEvaluator class
//
// ============================================================================

Types::TypeEvaluator::TypeEvaluator(Types *types)
// ----------------------------------------------------------------------------
//   Constructor simply preserves the types
// ----------------------------------------------------------------------------
    : types(types)
{}


static Tree *lookupType(Scope *evalScope, Scope *sc,
                        Tree *what, Infix *entry, void *i)
// ----------------------------------------------------------------------------
//   Used to lookup type expressions
// ----------------------------------------------------------------------------
{
    if (Name *test = what->AsName())
        if (Name *decl = entry->left->AsName())
            if (test->value == decl->value)
                return decl;

    Ooops("Internal: Looking up type $1, found candidate $2 - Unimplemented",
          what, entry->left);
    return entry->left;
}


Tree *Types::TypeEvaluator::Do(Tree *what)
// ----------------------------------------------------------------------------
//   Evaluate the tree in the given types and context
// ----------------------------------------------------------------------------
{
    record(types, "In %p evaluating type %t", this, what);
    Tree *candidate = types->context->Lookup(what, lookupType, this);
    record(types, "In %p evaluated type %t to %t", this, what, candidate);
    return candidate;
}


Tree *Types::TypeEvaluator::Do(Name *what)
// ----------------------------------------------------------------------------
//   Evaluate the tree in the given types and context
// ----------------------------------------------------------------------------
{
    record(types, "In %p evaluating type name %t", this, what);
    Tree *candidate = types->context->Bound(what);
    record(types, "In %p evaluated type name %t to %t", this, what, candidate);
    return candidate;
}


Tree *Types::TypeEvaluator::Do(Infix *what)
// ----------------------------------------------------------------------------
//   Evaluate the tree in the given types and context
// ----------------------------------------------------------------------------
{
    // The type of the sequence is the type of the last statement
    if (IsSequence(what) || IsTypeAnnotation(what) || IsDefinition(what))
    {
        Ooops("Malformed type expression $1", what);
        return xl_error;
    }

    // Fallback to normal evaluation
    return Do((Tree *) what);
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
        ti->Dump();
    }
    return ti;
}


XL::Types *xldebug(XL::Types_p ti)
// ----------------------------------------------------------------------------
//   Dump a pointer to compiler types
// ----------------------------------------------------------------------------
{
    return xldebug((XL::Types *) ti);
}


RECORDER(types,                 64, "Type analysis");
RECORDER(types_ids,             64, "Assigned type identifiers");
RECORDER(types_unifications,    64, "Type unifications");
RECORDER(types_calls,           64, "Type deductions in rewrites (calls)");
RECORDER(types_boxing,          64, "Machine type boxing and unboxing");
RECORDER(types_joined,          64, "Joined types");
