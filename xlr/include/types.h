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
//  The fundamental types are:
//  - Literals, which match only their exact value
//  - boolean, type of true and false
//  - integer, type of integer literals such as 0, 1, 2, ...
//  - real, type of real literals such as 0.3, 4.2, 6.3e21
//  - text, type of double-quoted text literals such as "ABC" and "Hello World"
//  - character, type of single-quoted text literals such as 'A' and ' '
//  - name, type of names (not symbols) such as A or Toto2
//  - symbol, type of symbols (not names) such as + or **
//  - infix, prefix, postfix, block, type of corresponding structured types
//
//  The type constructors are:
//  - (T): Values from T
//  - T1 |  T2 : Values of either type T1 or T2
//  - T1 -> T2 : A transformation taking T1 and returning T2
//  - type(pattern): A type matching the pattern, e.g. type(X+Y:integer)
//                   matches 2.3+5 or A+3
//
//  The type analysis phase consists in scanning the input tree,
//  recording type information and returning typed trees.

#include "tree.h"
#include "context.h"
#include <map>

XL_BEGIN

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
    TypeInference(Context *context) : context(context), types(), id(0) {}
    typedef bool value_type;

public:
    // Interface for Tree::Do() to annotate the tree
    bool DoInteger(Integer *what);
    bool DoReal(Real *what);
    bool DoText(Text *what);
    bool DoName(Name *what);
    bool DoPrefix(Prefix *what);
    bool DoPostfix(Postfix *what);
    bool DoInfix(Infix *what);
    bool DoBlock(Block *what);

    // Common code for all constants (integer, real, text)
    bool DoConstant(Tree *what);

public:
    // Annotate expressions with type variables
    bool        AssignType(Tree *expr, Tree *type = NULL);
    bool        Rewrite(Infix *rewrite);

    // Attempt to evaluate an expression and perform reqiored unifications
    bool        Evaluate(Tree *tree);    

    // Indicates that two trees must have compatible types
    bool        Unify(Tree *expr1, Tree *expr2);
    bool        UnifyType(Tree *t1, Tree *t2);
    bool        JoinTypes(Tree *base, Tree *other, bool knownGood = false);

    // Return the base type associated with a given tree
    Tree *      Type(Tree *expr);
    Tree *      Base(Tree *type);
    bool        IsGeneric(Tree *type);
    bool        IsTypeName(Tree *type);

    // Generation of type names
    Name *      NewTypeName(TreePosition pos);

public:
    Context_p   context;        // Context in which we lookup things
    tree_map    types;          // Map an expression to its type
    ulong       id;             // Id of next type
};


struct TypeClass : Info
// ----------------------------------------------------------------------------
//   Record which types have been unified
// ----------------------------------------------------------------------------
{
    TypeClass(Tree *base): base(base) {}
    typedef Tree_p       data_t;
    operator             data_t()  { return base; }
    Tree_p               base;
};



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
#define NAME(symbol)
#define TYPE(symbol)    extern Name_p symbol##_type;
#include "basics.tbl"

XL_END

extern "C" void debugt(XL::TypeInference *ti);

#endif // TYPES_H
