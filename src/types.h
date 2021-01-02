#ifndef TYPES_H
#define TYPES_H
// ****************************************************************************
//  types.h                                                          XL project
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

#include "tree.h"
#include "context.h"
#include <map>

XL_BEGIN


// Forward classes
class Types;
typedef GCPtr<Types> Types_p;
struct RewriteCandidate;
struct RewriteCalls;
typedef GCPtr<RewriteCalls> RewriteCalls_p;
typedef std::map<Tree_p, RewriteCalls_p> rcall_map;
extern Name_p tree_type;


class Types
// ----------------------------------------------------------------------------
//   Record types information in a given context
// ----------------------------------------------------------------------------
{
public:
    Types(Scope *scope);
    virtual ~Types();
    typedef Tree *value_type;

    // Create a types structure for local processing
    virtual Types *     LocalTypes();

    // Create rewrite calls for this class
    virtual RewriteCalls *NewRewriteCalls();

public:
    // Main entry point: perform type analysis on a whole program
    virtual Tree *      TypeAnalysis(Tree *source);

    // Return the type for the current mode, for a value and for a declaration
    Tree *              Type(Tree *expr);
    Tree *              ValueType(Tree *expr);
    Tree *              DeclarationType(Tree *expr);
    Tree *              BaseType(Tree *type, bool recurse = true);
    Tree *              KnownType(Tree *expr, bool recurse = true);

    // Access to the scope
    Scope *             TypesScope()            { return context->Symbols(); }
    Context *           TypesContext()          { return context; }

public:
    // Interface for Tree::Do() to traverse and annotate the tree
    Tree *              Do(Integer *what);
    Tree *              Do(Real *what);
    Tree *              Do(Text *what);
    Tree *              Do(Name *what);
    Tree *              Do(Prefix *what);
    Tree *              Do(Postfix *what);
    Tree *              Do(Infix *what);
    Tree *              Do(Block *what);

protected:
    // Local types (use LocalTypes() to create)
    Types(Scope *scope, Types *parent);

    // Common code for all constants (integer, real, text)
    Tree *              DoConstant(Tree *what, Tree *type, kind k);
    Tree *              DoStatements(Tree *expr, Tree *left, Tree *right);
    Tree *              DoTypeAnnotation(Infix *decl);
    Tree *              DoRewrite(Infix *rewrite);

    // Type compatibility checks
    Tree *              TypeCoversType(Tree *type, Tree *ref);

public:
    // Evaluate expression and perform required unifications
    virtual Tree *      Evaluate(Tree *tree, bool mayFail = false);

    // Evaluate a type epxression
    virtual Tree *      EvaluateType(Tree *tree, bool mayfail = false);

    // Type associations and unifications
    Tree *              TypeError(Tree *t1, Tree *t2, Tree *x1=0, Tree *x2=0);
    Tree *              AssignType(Tree *expr, Tree *type, bool add = false);
    Tree *              Unify(Tree *t1, Tree *t2, Tree *x1=0, Tree *x2=0);
    Tree *              Join(Tree *old, Tree *replacement);
    Tree *              JoinedType(Tree *type, Tree *old, Tree *replacement);
    Tree *              UnionType(Tree *t1, Tree *t2);
    Tree *              PatternType(Tree *form);
    Tree *              UnknownType(TreePosition pos);
    Tree *              MakeTypesExplicit(Tree *expr);

    // Checking if we have specific kinds of types
    static Tree *       IsPatternType(Tree *type);
    static Tree *       IsRewriteType(Tree *type);
    static Infix *      IsUnionType(Tree *type);
    static bool         IsUnknownType(text name);
    static Name *       IsUnknownType(Tree *type);
    static Name *       IsTypeName(Tree *type);

    // Access to rewrite calls
    RewriteCalls *      TreeRewriteCalls(Tree *what, bool recurse = true);
    void                TreeRewriteCalls(Tree *what, RewriteCalls *rc);


protected:
    struct TypeEvaluator
    {
        TypeEvaluator(Types *types);

        // Interface for Tree::Do() to traverse and annotate the tree
        typedef Tree *value_type;
        Tree *  Do(Tree *what);
        Tree *  Do(Name *what);
        Tree *  Do(Infix *what);
        Tree *  Do(Prefix *what);
        Tree *  Evaluate(Tree *what);

        Types_p types;
    };

public:
    // Checking if a declaration is data or a C declaration
    enum class Decl { NORMAL, C, DATA, BUILTIN };
    static Decl RewriteCategory(RewriteCandidate *rc);
    static Decl RewriteCategory(Rewrite *rw, Tree *defined, text &label);
    static bool IsValidCName(Tree *tree, text &label);

public:
    // Debug utilities
    virtual void Dump();
    virtual void DumpAll();

public:
    GARBAGE_COLLECT(Types);

protected:
    Context_p   context;        // Context for lookups
    tree_map    types;          // Type associated with an expression if any
    tree_map    unifications;   // Map a type to its reference type
    tree_map    captured;       // Trees captured from enclosing context
    rcall_map   rcalls;         // Rewrites to call for a given tree
    Types_p     parent;         // Parent type information if any
    bool        declaration;    // Analyzing type of a declaration
    static uint id;             // Id of next type
};



// ============================================================================
//
//   Inline functions
//
// ============================================================================

inline bool Types::IsUnknownType(text name)
// ----------------------------------------------------------------------------
//   Check if a given type is a generated generic type name
// ----------------------------------------------------------------------------
{
    return name.size() && name[0] == '#';
}


inline Name *Types::IsUnknownType(Tree *type)
// ----------------------------------------------------------------------------
//   Check if a given type is a generated generic type name
// ----------------------------------------------------------------------------
{
    if (Name *name = type->AsName())
        if (IsUnknownType(name->value))
            return name;
    return nullptr;
}


inline Name *Types::IsTypeName(Tree *type)
// ----------------------------------------------------------------------------
//   Check if a given type is a 'true' type name, i.e. not generated
// ----------------------------------------------------------------------------
{
    if (Name *name = type->AsName())
        if (!IsUnknownType(name->value))
            return name;
    return nullptr;
}


inline bool IsTreeType(Tree *type)
// ----------------------------------------------------------------------------
//   Return true for the 'tree' type
// ----------------------------------------------------------------------------
{
    return type == tree_type;
}


inline bool IsErrorType(Tree *type)
// ----------------------------------------------------------------------------
//   Check if a type represents an error
// ----------------------------------------------------------------------------
{
    return type == nullptr;
}

XL_END

RECORDER_DECLARE(types);
RECORDER_DECLARE(types_ids);
RECORDER_DECLARE(types_unifications);
RECORDER_DECLARE(types_calls);
RECORDER_DECLARE(types_boxing);
RECORDER_DECLARE(types_joined);

#endif // TYPES_H
