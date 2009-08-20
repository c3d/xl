#ifndef TYPES_H
#define TYPES_H
// ****************************************************************************
//  types.h                         (C) 1992-2009 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Define the type system in interpreted XL
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
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************
//  The type system in interpreted XL is somewhat similar to what is found
//  in Haskell, except that it's based on the shape of trees.
//
//  The fundamental types are:
//  - integer, type of 0, 1, 2, ...
//  - real, type of 0.3, 4.2, 6.3e21
//  - text, type of "ABC" and "Hello World"
//  - character, type of 'A' and ' '
//  - boolean, type of true and false
//
//  The type constructors are:
//  - T1 <nl> T2: Values of either type T1 or T2 (<nl> is infix newline)
//  - T1 -> T2: A function taking T1 and returning T2
//  - expr: A tree with the given shape, e.g  (T1, T2), T1+T2
//
//  The type analysis phase consists in scanning the input tree,
//  recording type information and returning typed trees.


#include "tree.h"
#include "context.h"
#include <map>

XL_BEGIN

typedef std::map<Tree *, Tree *>        type_map;

struct InferTypes : Action
// ----------------------------------------------------------------------------
//    Infer the types in an expression
// ----------------------------------------------------------------------------
{
    InferTypes(Context *ctx): Action(), context(ctx) {}

    Tree *Do (Tree *what);
    Tree *DoInteger(Integer *what);
    Tree *DoReal(Real *what);
    Tree *DoText(Text *what);
    Tree *DoName(Name *what);
    Tree *DoPrefix(Prefix *what);
    Tree *DoPostfix(Postfix *what);
    Tree *DoInfix(Infix *what);
    Tree *DoBlock(Block *what);
    Tree *DoNative(Native *what);

    Context *   context;
    type_map    types;
};

extern Tree * integer_type;
extern Tree * real_type;
extern Tree * text_type;
extern Tree * character_type;
extern Tree * boolean_type;
extern Tree * nil_type;

XL_END

#endif // TYPES_H
