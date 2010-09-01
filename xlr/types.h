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
//   High-level entry points for type management
// 
// ============================================================================

Tree *ValueMatchesType(Tree *type, Tree *value, bool conversions);
Tree *TypeCoversType(Tree *type, Tree *test, bool conversions);
Tree *TypeIntersectsType(Tree *type, Tree *test, bool conversions);
Tree *UnionType(Tree *t1, Tree *t2);
Tree *CanonicalType(Tree *value);
Tree *StructuredType(Tree *value);



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
    typedef Tree_p       data_t;
    TypeInfo(Tree *type): type(type) {}
    operator            data_t()  { return type; }
    Tree_p               type;
};


struct MatchType : Action
// ----------------------------------------------------------------------------
//   An action that checks if a value matches a type
// ----------------------------------------------------------------------------
{
    MatchType(Symbols *s, Tree *t): symbols(s), type(t) {}

    Tree *Do(Tree *what);
    Tree *DoInteger(Integer *what);
    Tree *DoReal(Real *what);
    Tree *DoText(Text *what);
    Tree *DoName(Name *what);
    Tree *DoPrefix(Prefix *what);
    Tree *DoPostfix(Postfix *what);
    Tree *DoInfix(Infix *what);
    Tree *DoBlock(Block *what);

    Tree *MatchStructuredType(Tree *what, Tree *kind = NULL);
    Tree *Rewrites(Tree *what);
    Tree *Normalize();
    Tree *NameMatch(Tree *what);

    Symbols_p symbols;
    Tree_p    type;
};


struct ArgumentTypeMatch : Action
// ----------------------------------------------------------------------------
//   Check if a tree matches the form of the left of a rewrite
// ----------------------------------------------------------------------------
{
    ArgumentTypeMatch (Tree *t,
                       Symbols *s, Symbols *l, Symbols *r) :
        symbols(s), locals(l), rewrite(r), test(t), defined(NULL) {}

    // Action callbacks
    virtual Tree *Do(Tree *what);
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);

public:
    Symbols_p      symbols;     // Context in which we evaluate values
    Symbols_p      locals;      // Symbols where we declare arguments
    Symbols_p      rewrite;     // Symbols in which the rewrite was declared
    Tree_p         test;        // Tree we test
    Tree_p         defined;     // Tree we define
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

#define INFIX(name, rtype, t1, symbol, t2, code, doc)
#define PARM(symbol, type)
#define PREFIX(name, rtype, symbol, parms, code, doc)
#define POSTFIX(name, rtype, parms, symbol, code, doc)
#define BLOCK(name, rtype, open, type, close, code, doc)
#define NAME(symbol)
#define TYPE(symbol)    extern Name_p symbol##_type;
#include "basics.tbl"

XL_END

#endif // TYPES_H
