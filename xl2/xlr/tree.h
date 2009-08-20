#ifndef TREE_H
#define TREE_H
// ****************************************************************************
//  tree.h                          (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                            XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Basic representation of parse tree. See parser.h and parser.cpp
//     for details about the generation of these trees.
// 
// 
// 
// 
// 
// 
// 
// ****************************************************************************
// This program is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "base.h"


struct XLTree
// ----------------------------------------------------------------------------
//   The base class for all XL trees
// ----------------------------------------------------------------------------
{
    virtual ~XLTree() {}
};


struct XLInteger : XLTree
// ----------------------------------------------------------------------------
//   A node representing an integer
// ----------------------------------------------------------------------------
{
    XLInteger(longlong i = 0): value(i) {}
    longlong            value;
};


struct XLReal : XLTree
// ----------------------------------------------------------------------------
//   A node representing a real number
// ----------------------------------------------------------------------------
{
    XLReal(double d = 0.0): value(d) {}
    double              value;
};


struct XLString : XLTree
// ----------------------------------------------------------------------------
//   A node representing a string
// ----------------------------------------------------------------------------
{
    XLString(text t, char q): value(t), quote(q) {}
    text                value;
    char                quote;
};


struct XLName : XLTree
// ----------------------------------------------------------------------------
//   A node representing a name or symbol
// ----------------------------------------------------------------------------
{
    XLName(text n): value(n) {}
    text                value;
};


struct XLBlock : XLTree
// ----------------------------------------------------------------------------
//   A parenthese operator
// ----------------------------------------------------------------------------
{
    XLBlock(XLTree *c, text i, text o): child(c), opening(i), closing(o) {}
    XLTree              *child;
    text                opening, closing;
};


struct XLPrefix : XLTree
// ----------------------------------------------------------------------------
//   A prefix operator
// ----------------------------------------------------------------------------
{
    XLPrefix(XLTree *l, XLTree *r): left(l), right(r) {}
    XLTree              *left;
    XLTree              *right;
};


struct XLInfix : XLTree
// ----------------------------------------------------------------------------
//   An infix operator
// ----------------------------------------------------------------------------
{
    XLInfix(text n, XLTree *l, XLTree *r): name(n), left(l), right(r) {}
    text                name;
    XLTree              *left;
    XLTree              *right;
};


struct XLBuiltin : XLTree
// ----------------------------------------------------------------------------
//   A builtin directly performs some operation on a tree
// ----------------------------------------------------------------------------
{
    XLBuiltin(void *ptr): cookie(ptr) {}
    void        *cookie;
};
    
// For use in a debugger
extern "C" void debug(XLTree *);

#endif // TREE_H
