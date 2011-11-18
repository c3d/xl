#ifndef TYPES_H
#define TYPES_H
// ****************************************************************************
//  types.h                                                         XLR project
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
// This document is released under the GNU General Public License.
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
//   - A union of types         0,3,5   integer|real
//   - A block for precedence   (real)
//   - A rewrite specifier      integer => real
//   - The type of a pattern    type (X:integer, Y:integer)
//
//  REVISIT: The form A => B is to distinguish from a rewrite itself.
//  Not sure if this is desirable.

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
typedef GCPtr<RewriteCalls> RewriteCalls_p;
typedef std::map<Tree_p, RewriteCalls_p> rcall_map;



// ============================================================================
//
//   Class used to infer types in a program (hacked Damas-Hindley-Milner)
//
// ============================================================================

struct TypeInference
// ----------------------------------------------------------------------------
//   Scan a tree, record required types and perform type analysis
// ----------------------------------------------------------------------------
{
    TypeInference(Context *context);
    TypeInference(Context *context, TypeInference *parent);
    ~TypeInference();
    typedef bool value_type;
    enum unify_mode { STANDARD, DECLARATION };

public:
    // Main entry point
    bool TypeCheck(Tree *what);
    Tree *Type(Tree *expr);

public:
    // Interface for Tree::Do() to annotate the tree
    bool        DoInteger(Integer *what);
    bool        DoReal(Real *what);
    bool        DoText(Text *what);
    bool        DoName(Name *what);
    bool        DoPrefix(Prefix *what);
    bool        DoPostfix(Postfix *what);
    bool        DoInfix(Infix *what);
    bool        DoBlock(Block *what);

    // Common code for all constants (integer, real, text)
    bool        DoConstant(Tree *what);

public:
    // Annotate expressions with type variables
    bool        AssignType(Tree *expr, Tree *type = NULL);
    bool        Rewrite(Infix *rewrite);
    bool        Data(Tree *form);
    bool        Extern(Tree *form);

    // Attempt to evaluate an expression and perform required unifications
    bool        Evaluate(Tree *tree);

    // Indicates that two trees must have compatible types
    bool        UnifyTypesOf(Tree *expr1, Tree *expr2);
    bool        Unify(Tree *t1, Tree *t2,
                      Tree *x1, Tree *x2, unify_mode mode = STANDARD);
    bool        Unify(Tree *t1, Tree *t2, unify_mode mode = STANDARD);
    bool        Join(Tree *base, Tree *other, bool knownGood = false);
    bool        JoinConstant(Tree *cst, Name *tname);
    bool        UnifyPatterns(Tree *t1, Tree *t2);
    bool        Commit(TypeInference *child);

    // Return the base type associated with a given tree
    Tree *      Base(Tree *type);
    bool        IsGeneric(text name);
    bool        IsGeneric(Tree *type);
    bool        IsTypeName(Tree *type);

    // Generation of type names
    Name *      NewTypeName(TreePosition pos);

    // Lookup a type name in the given context
    Tree *      LookupTypeName(Tree *input);

    // Error messages
    bool        TypeError(Tree *t1, Tree *t2);

public:
    Context_p   context;        // Context in which we lookup things
    tree_map    types;          // Map an expression to its type
    tree_map    unifications;   // Map a type to its reference type
    rcall_map   rcalls;         // Rewrites to call for a given tree
    Tree_p      left, right;    // Current left and right of unification
    bool        prototyping;    // Prototyping a function declaration
    bool        matching;       // Matching a pattern
    static ulong id;            // Id of next type

    GARBAGE_COLLECT(TypeInference);
};
typedef GCPtr<TypeInference> TypeInference_p;



// ============================================================================
//
//   High-level entry points for type management
//
// ============================================================================

Tree *ValueMatchesType(Context *, Tree *type, Tree *value, bool conversions);
Tree *TypeCoversType(Context *, Tree *type, Tree *test, bool conversions);
Tree *TypeIntersectsType(Context *, Tree *type, Tree *test, bool conversions);
Tree *UnionType(Context *, Tree *t1, Tree *t2);
Tree *CanonicalType(Context *, Tree *value);
Tree *StructuredType(Context *, Tree *value);
bool  IsTreeType(Tree *type);



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
//   Declare the basic types
//
// ============================================================================

#undef INFIX
#undef PREFIX
#undef POSTFIX
#undef BLOCK
#undef FORM
#undef NAME
#undef TYPE
#undef PARM
#undef DS
#undef RS
#undef RETURNS
#undef GROUP
#undef SYNOPSIS
#undef DESCRIPTION
#undef SEE

#define SEE(see)
#define RETURNS(type, rdoc)
#define GROUP(grp)
#define SYNOPSIS(syno)
#define DESCRIPTION(desc)
#define INFIX(name, rtype, t1, symbol, t2, code, docinfo)
#define PARM(symbol, type, pdoc)
#define PREFIX(name, rtype, symbol, parms, code, docinfo)
#define POSTFIX(name, rtype, parms, symbol, code, docinfo)
#define BLOCK(name, rtype, open, type, close, code, docinfo)
#define FORM(name, rtype, form, parms, _code, docinfo)
#define NAME(symbol)
#define TYPE(symbol)    extern Name_p symbol##_type;
#include "basics.tbl"



// ============================================================================
//
//   Inline functions
//
// ============================================================================

inline bool TypeInference::IsGeneric(text name)
// ----------------------------------------------------------------------------
//   Check if a given type is a generated generic type name
// ----------------------------------------------------------------------------
{
    return name.size() && name[0] == '#';
}


inline bool TypeInference::IsGeneric(Tree *type)
// ----------------------------------------------------------------------------
//   Check if a given type is a generated generic type name
// ----------------------------------------------------------------------------
{
    if (Name *name = type->AsName())
        return IsGeneric(name->value);
    return false;
}


inline bool TypeInference::IsTypeName(Tree *type)
// ----------------------------------------------------------------------------
//   Check if a given type is a 'true' type name, i.e. not generated
// ----------------------------------------------------------------------------
{
    if (Name *name = type->AsName())
        return !IsGeneric(name->value);
    return false;
}


inline bool IsTreeType(Tree *type)
// ----------------------------------------------------------------------------
//   Return true for any 'tree' type
// ----------------------------------------------------------------------------
{
    return (type == tree_type       ||
            type == source_type     ||
            type == code_type       ||
            type == lazy_type       ||
            type == value_type);
}

XL_END

extern "C" void debugt(XL::TypeInference *ti);
extern "C" void debugu(XL::TypeInference *ti);
extern "C" void debugr(XL::TypeInference *ti);

#endif // TYPES_H
