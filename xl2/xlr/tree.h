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

#include <iostream>
#include "base.h"


struct XLTree
// ----------------------------------------------------------------------------
//   The base class for all XL trees
// ----------------------------------------------------------------------------
{
    typedef std::ostream ostream;
    virtual ~XLTree() {}
    virtual void        Output(ostream &out) { out << "tree"; }
    
    static uint         outputIndent;
    static bool         outputDebug;
};
std::ostream &operator<<(std::ostream &out, XLTree &tree);


struct XLInteger : XLTree
// ----------------------------------------------------------------------------
//   A node representing an integer
// ----------------------------------------------------------------------------
{
    XLInteger(longlong i = 0): value(i) {}
    virtual void        Output(ostream &out) { out << value; }
    longlong            value;
};


struct XLReal : XLTree
// ----------------------------------------------------------------------------
//   A node representing a real number
// ----------------------------------------------------------------------------
{
    XLReal(double d = 0.0): value(d) {}
    virtual void        Output(ostream &out) { out << value; }
    double              value;
};


struct XLString : XLTree
// ----------------------------------------------------------------------------
//   A node representing a string
// ----------------------------------------------------------------------------
{
    XLString(text t, char q): value(t), quote(q) {}
    virtual void        Output(ostream &out) { out << quote<<value<<quote; }
    text                value;
    char                quote;
};


struct XLName : XLTree
// ----------------------------------------------------------------------------
//   A node representing a name or symbol
// ----------------------------------------------------------------------------
{
    XLName(text n): value(n) {}
    virtual void        Output(ostream &out);
    text                value;
};


struct XLBlock : XLTree
// ----------------------------------------------------------------------------
//   A parenthese operator
// ----------------------------------------------------------------------------
{
    XLBlock(XLTree *c, text i, text o): child(c), opening(i), closing(o) {}
    virtual void        Output(ostream &o);
    XLTree              *child;
    text                opening, closing;
};


struct XLPrefix : XLTree
// ----------------------------------------------------------------------------
//   A prefix operator
// ----------------------------------------------------------------------------
{
    XLPrefix(XLTree *l, XLTree *r): left(l), right(r) {}
    virtual void        Output(ostream &o);
    XLTree              *left;
    XLTree              *right;
};


struct XLInfix : XLTree
// ----------------------------------------------------------------------------
//   An infix operator
// ----------------------------------------------------------------------------
{
    XLInfix(text n, XLTree *l, XLTree *r): name(n), left(l), right(r) {}
    virtual void        Output(ostream &o);
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



// ============================================================================
// 
//   Stream operations
// 
// ============================================================================

class nl_indent_t {};
extern class nl_indent_t nl_indent;
inline std::ostream &operator<<(std::ostream &out, nl_indent_t &unused)
// ----------------------------------------------------------------------------
//   Emitting a tree
// ----------------------------------------------------------------------------
{
    out << '\n';
    for (uint i = 0; i < XLTree::outputIndent; i++)
        out << ' ';
    return out;
}


class indent_t {};
extern class indent_t indent;
inline std::ostream &operator<<(std::ostream &out, indent_t &unused)
// ----------------------------------------------------------------------------
//   Emitting a tree
// ----------------------------------------------------------------------------
{
    XLTree::outputIndent += 2;
    return out;
}


class unindent_t {};
extern class unindent_t unindent;
inline std::ostream &operator<<(std::ostream &out, unindent_t &unused)
// ----------------------------------------------------------------------------
//   Emitting a tree
// ----------------------------------------------------------------------------
{
    XLTree::outputIndent -= 2;
    return out;
}


inline std::ostream &operator<<(std::ostream &out, XLTree &tree)
// ----------------------------------------------------------------------------
//   Emitting a tree
// ----------------------------------------------------------------------------
{
    tree.Output(out);
    return out;
}

    
extern "C" void debug(XLTree *);

#endif // TREE_H
