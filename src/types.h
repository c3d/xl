#ifndef TYPES_H
#define TYPES_H
// ****************************************************************************
//  types.h                                                       XL project
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************
//
//  The type system in XL is somewhat similar to what is found in Haskell,
//  except that it's based on the shape of trees.
//
//  A type form in XL can be:
//   - A type name              integer
//   - A litteral value         0       1.5             "Hello"
//   - A range of values        0..4    1.3..8.9        "A".."Z"
//   - A union of types         0|3|5   integer|real
//   - A rewrite specifier      integer => real
//   - The type of a pattern    type (X:integer, Y:integer)

#include "tree.h"
#include "context.h"
#include <map>


XL_BEGIN

// ============================================================================
//
//   Forward classes
//
// ============================================================================

struct RewriteCalls;
typedef GCPtr<RewriteCalls>              RewriteCalls_p;
typedef std::map<Tree_p, RewriteCalls_p> rcall_map;

extern Name_p tree_type;


// ============================================================================
//
//   Class used to infer types in a program (hacked Damas-Hindley-Milner)
//
// ============================================================================

class Types
// ----------------------------------------------------------------------------
//   Record type information
// ----------------------------------------------------------------------------
{
    Context_p   context;        // Context in which we lookup things
    TreeMap     types;          // Map an expression to its type
    TreeMap     unifications;   // Map a type to its reference type
    rcall_map   rcalls;         // Rewrites to call for a given tree
    bool        declaration;    // Analyzing type of a declaration
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
    Tree *      DeclarationType(Tree *expr);
    Tree *      NewType(Tree *expr);
    Tree *      ValueType(Tree *expr);
    Tree *      BaseType(Tree *expr);
    rcall_map & TypesRewriteCalls();
    RewriteCalls *HasRewriteCalls(Tree *what);
    Scope *     TypesScope();
    Context *   TypesContext();
    bool        HasCaptures(Tree *form, TreeList &captured);

public:
    // Interface for Tree::Do() to annotate the tree
    Tree *      DoInteger(Integer *what);
    Tree *      DoReal(Real *what);
    Tree *      DoText(Text *what);
    Tree *      DoName(Name *what);
    Tree *      DoPrefix(Prefix *what);
    Tree *      DoPostfix(Postfix *what);
    Tree *      DoInfix(Infix *what);
    Tree *      DoBlock(Block *what);

public:
    // Common code for all constants (integer, real, text)
    Tree *      DoConstant(Tree *what, kind k);

    // Annotate expressions with type variables
    Tree *      AssignType(Tree *expr, Tree *type);
    Tree *      TypeOf(Tree *expr);
    Tree *      MakeTypesExplicit(Tree *expr);
    Tree *      TypeDeclaration(Infix *decl);
    Tree *      TypeOfRewrite(Infix *rw);
    Tree *      Statements(Tree *expr, Tree *left, Tree *right);

    // Attempt to evaluate an expression and perform required unifications
    Tree *      Evaluate(Tree *tree, bool mayFail = false);

    // Evaluate a type expression
    Tree *      EvaluateType(Tree *tree);

    // Indicates that two trees must have compatible types
    Tree *      Unify(Tree *t1, Tree *t2);
    Tree *      Join(Tree *old, Tree *replacement);
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
    bool        TreePatternsMatch(Tree *t1, Tree *t2);
    bool        TreePatternMatchesValue(Tree *pat, Tree *val);

    // Checking if we have specific kinds of types
    Tree *      IsTypeOf(Tree *type);
    Infix *     IsRewriteType(Tree *type);
    Infix *     IsRangeType(Tree *type);
    Infix *     IsUnionType(Tree *type);

    // Generation of type names
    Name *      NewTypeName(TreePosition pos);

    // Lookup a type name in the given context
    Tree *      DeclaredTypeName(Tree *input);

    // Error messages
    Tree *      TypeError(Tree *t1, Tree *t2);

    // Debug utilities
    void        DumpTypes();
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

extern "C" XL::Types *debugt(XL::Types *ti);
extern "C" XL::Types *debugu(XL::Types *ti);
extern "C" XL::Types *debugr(XL::Types *ti);

RECORDER_DECLARE(types);
RECORDER_DECLARE(types_ids);
RECORDER_DECLARE(types_unifications);
RECORDER_DECLARE(types_calls);

#endif // TYPES_H
