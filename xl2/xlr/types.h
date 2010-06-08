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
//  - integer, type of 0, 1, 2, ...
//  - real, type of 0.3, 4.2, 6.3e21
//  - text, type of "ABC" and "Hello World"
//  - character, type of 'A' and ' '
//  - boolean, type of true and false
//
//  The type constructors are:
//  - T1 |  T2 : Values of either type T1 or T2
//  - T1 -> T2 : A function taking T1 and returning T2
//  - expr: A tree with the given shape, e.g  (T1, T2), T1+T2
//
//  The type analysis phase consists in scanning the input tree,
//  recording type information and returning typed trees.


#include "tree.h"
#include "context.h"
#include <map>

XL_BEGIN

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


struct InferTypes : Action
// ----------------------------------------------------------------------------
//    Infer the types in an expression
// ----------------------------------------------------------------------------
{
    InferTypes(Symbols *s): Action(), symbols(s) {}

    Tree *Do (Tree *what);
    Tree *DoInteger(Integer *what);
    Tree *DoReal(Real *what);
    Tree *DoText(Text *what);
    Tree *DoName(Name *what);
    Tree *DoPrefix(Prefix *what);
    Tree *DoPostfix(Postfix *what);
    Tree *DoInfix(Infix *what);
    Tree *DoBlock(Block *what);

    Symbols_p   symbols;
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

#define INFIX(name, rtype, t1, symbol, t2, code)
#define PARM(symbol, type)
#define PREFIX(name, rtype, symbol, parms, code)
#define POSTFIX(name, rtype, parms, symbol, code)
#define BLOCK(name, rtype, open, type, close, code)
#define NAME(symbol)
#define TYPE(symbol)    extern Name_p symbol##_type;
#include "basics.tbl"

XL_END

#endif // TYPES_H
