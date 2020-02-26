#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H
// *****************************************************************************
// compiler-types.h                                                 XL project
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
// (C) 2009-2011,2014-2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
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
//
//  The type system in XL is based on the shape of trees, for example:
//     type positive_add is matching X+Y when X > 0 and Y > 0
//
//  A value can belong to multiple types. For example, 3+5 belongs to
//  the infix type (since 3+5 is an infix form) as well as to positive_add

#include "tree.h"
#include "context.h"
#include "llvm-crap.h"
#include <map>


XL_BEGIN

// ============================================================================
//
//   Forward classes
//
// ============================================================================

struct RewriteCalls;
struct RewriteCandidate;
typedef GCPtr<RewriteCalls>              RewriteCalls_p;
typedef std::map<Tree_p, RewriteCalls_p> rcall_map;
typedef std::map<Tree_p, JIT::Type_p>    box_map;

extern Name_p tree_type;


// ============================================================================
//
//   Class used to record types in a program
//
// ============================================================================

class Types
// ----------------------------------------------------------------------------
//   Record type information
// ----------------------------------------------------------------------------
{
    Context_p   context;        // Context in which we lookup things
    tree_map    types;          // Map an expression to its type
    tree_map    unifications;   // Map a type to its reference type
    tree_map    captured;       // Trees captured from enclosing context
    rcall_map   rcalls;         // Rewrites to call for a given tree
    box_map     boxed;          // Tree type -> machine type
    bool        declaration;    // Analyzing type of a declaration
    bool        codegen;        // Code generation started
    static uint id;             // Id of next type

public:
    Types(Scope *scope);
    Types(Scope *scope, Types *parent);
    ~Types();
    typedef Tree *value_type;

public:
    // Main entry point
    Tree *      TypeAnalysis(Tree *source);
    Tree *      Type(Tree *expr);
    Tree *      KnownType(Tree *expr);
    Tree *      ValueType(Tree *expr);
    Tree *      DeclarationType(Tree *expr);
    Tree *      CodegenType(Tree *expr);
    Tree *      BaseType(Tree *expr);
    rcall_map & TypesRewriteCalls();
    RewriteCalls *HasRewriteCalls(Tree *what);
    Scope *     TypesScope();
    Context *   TypesContext();

    // Machine types management
    void        AddBoxedType(Tree *treeType, JIT::Type_p machineType);
    JIT::Type_p BoxedType(Tree *type);

public:
    // Interface for Tree::Do() to annotate the tree
    Tree *      Do(Integer *what);
    Tree *      Do(Real *what);
    Tree *      Do(Text *what);
    Tree *      Do(Name *what);
    Tree *      Do(Prefix *what);
    Tree *      Do(Postfix *what);
    Tree *      Do(Infix *what);
    Tree *      Do(Block *what);

public:
    // Common code for all constants (integer, real, text)
    Tree *      DoConstant(Tree *what, kind k);

    // Annotate expressions with type variables
    Tree *      AssignType(Tree *expr, Tree *type);
    Tree *      TypeOf(Tree *expr);
    Tree *      MakeTypesExplicit(Tree *expr);
    Tree *      TypeDeclaration(Rewrite *decl);
    Tree *      TypeOfRewrite(Rewrite *rw);
    Tree *      Statements(Tree *expr, Tree *left, Tree *right);

    // Attempt to evaluate an expression and perform required unifications
    Tree *      Evaluate(Tree *tree, bool mayFail = false);

    // Evaluate a type expression
    Tree *      EvaluateType(Tree *tree);

    // Indicates that two trees must have compatible types
    Tree *      Unify(Tree *t1, Tree *t2);
    Tree *      Join(Tree *old, Tree *replacement);
    Tree *      JoinedType(Tree *type, Tree *old, Tree *replacement);
    Tree *      UnionType(Tree *t1, Tree *t2);

    // Check attributes of the given name
    bool        IsGeneric(text name);
    Name *      IsGeneric(Tree *type);
    Name *      IsTypeName(Tree *type);

    // Operations on types
    Tree *      ValueMatchesType(Tree *type, Tree *value, bool conversions);
    bool        IsTreeType(Tree *type);
    bool        TypeCoversConstant(Tree *type, Tree *cst);
    bool        TypeCoversType(Tree *type, Tree *test);

    // Checking if we have specific kinds of types
    Tree *      IsTypeOf(Tree *type);
    Infix *     IsRewriteType(Tree *type);
    Infix *     IsRangeType(Tree *type);
    Infix *     IsUnionType(Tree *type);

    // Lookup a type name in the given context
    Tree *      DeclaredTypeName(Tree *input);

    // Checking if a declaration is data or a C declaration
    enum class Decl { NORMAL, C, DATA, BUILTIN };
    static Decl RewriteCategory(RewriteCandidate *rc);
    static Decl RewriteCategory(Rewrite *rw, Tree *defined, text &label);
    static bool IsValidCName(Tree *tree, text &label);

    // Error messages
    Tree *      TypeError(Tree *t1, Tree *t2);

    // Debug utilities
    void        DumpTypes();
    void        DumpMachineTypes();
    void        DumpUnifications();
    void        DumpRewriteCalls();

public:
    GARBAGE_COLLECT(Types);
};
typedef GCPtr<Types> Types_p;



// ============================================================================
//
//    Representation of types
//
// ============================================================================

struct TypeInfo : Info
// ----------------------------------------------------------------------------
//    Information recording the type of a given tree
// ----------------------------------------------------------------------------
{
    TypeInfo(Tree *type): type(type) {}
    typedef Tree_p       data_t;
    operator             data_t()  { return type; }
    Tree_p               type;
};



// ============================================================================
//
//   Inline functions
//
// ============================================================================

inline bool Types::IsGeneric(text name)
// ----------------------------------------------------------------------------
//   Check if a given type is a generated generic type name
// ----------------------------------------------------------------------------
{
    return name.size() && name[0] == '#';
}


inline Name *Types::IsGeneric(Tree *type)
// ----------------------------------------------------------------------------
//   Check if a given type is a generated generic type name
// ----------------------------------------------------------------------------
{
    if (Name *name = type->AsName())
        if (IsGeneric(name->value))
            return name;
    return nullptr;
}


inline Name *Types::IsTypeName(Tree *type)
// ----------------------------------------------------------------------------
//   Check if a given type is a 'true' type name, i.e. not generated
// ----------------------------------------------------------------------------
{
    if (Name *name = type->AsName())
        if (!IsGeneric(name->value))
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

XL_END

XL::Types *xldebug(XL::Types *ti);

RECORDER_DECLARE(types);
RECORDER_DECLARE(types_ids);
RECORDER_DECLARE(types_unifications);
RECORDER_DECLARE(types_calls);
RECORDER_DECLARE(types_boxing);
RECORDER_DECLARE(types_joined);

#endif // COMPILER_TYPES_H
