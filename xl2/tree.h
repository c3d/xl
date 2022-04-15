#ifndef TREE_H
#define TREE_H
// *****************************************************************************
// tree.h                                                             XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2003-2005,2015,2019-2020, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
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

#include <iostream>
#include "base.h"


enum XLKind
// ----------------------------------------------------------------------------
//   The fixed set of tree types
// ----------------------------------------------------------------------------
{
    xlUNKNOWN,
    xlNATURAL, xlREAL, xlSTRING, xlNAME,        // Atoms
    xlBLOCK,                                    // Unary parentheses and block
    xlPREFIX,                                   // Binary prefix-op
    xlINFIX,                                    // Ternary infix-op
    xlBUILTIN,                                  // Special
    xlLAST
};


#define INDENT_MARKER     "I+"
#define UNINDENT_MARKER   "I-"


struct XLTree
// ----------------------------------------------------------------------------
//   The base class for all XL trees
// ----------------------------------------------------------------------------
{
    typedef std::ostream ostream;
    virtual ~XLTree() {}
    virtual void        Output(ostream &out) { out << "tree"; }
    virtual XLKind      Kind() { return xlUNKNOWN; }

    static uint         outputIndent;
    static bool         outputDebug;
};
std::ostream &operator<<(std::ostream &out, XLTree &tree);


struct XLNatural : XLTree
// ----------------------------------------------------------------------------
//   A node representing an integer
// ----------------------------------------------------------------------------
{
    XLNatural(ulonglong i = 0): value(i) {}
    virtual void        Output(ostream &out) { out << value; }
    virtual XLKind      Kind() { return xlNATURAL; }
    ulonglong           value;
};


struct XLReal : XLTree
// ----------------------------------------------------------------------------
//   A node representing a real number
// ----------------------------------------------------------------------------
{
    XLReal(double d = 0.0): value(d) {}
    virtual void        Output(ostream &out) { out << value; }
    virtual XLKind      Kind() { return xlREAL; }
    double              value;
};


struct XLString : XLTree
// ----------------------------------------------------------------------------
//   A node representing a string
// ----------------------------------------------------------------------------
{
    XLString(text t, char q): value(t), quote(q) {}
    virtual void        Output(ostream &out) { out << quote<<value<<quote; }
    virtual XLKind      Kind() { return xlSTRING; }
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
    virtual XLKind      Kind() { return xlNAME; }
    text                value;
};


struct XLBlock : XLTree
// ----------------------------------------------------------------------------
//   A parenthese operator
// ----------------------------------------------------------------------------
{
    XLBlock(XLTree *c, text i, text o): child(c), opening(i), closing(o) {}
    virtual void        Output(ostream &o);
    virtual XLKind      Kind() { return xlBLOCK; }
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
    virtual XLKind      Kind() { return xlPREFIX; }
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
    virtual XLKind      Kind() { return xlINFIX; }
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
    virtual XLKind      Kind() { return xlBUILTIN; }
    void        *cookie;
};



// ============================================================================
//
//   Tree traversal
//
// ============================================================================

struct XLAction
// ----------------------------------------------------------------------------
//   The default action indicates there is more to do...
// ----------------------------------------------------------------------------
{
    // Override these...
    bool Natural(XLNatural *input)      { return false; }
    bool Real(XLReal *input)            { return false; }
    bool String(XLString *input)        { return false; }
    bool Name(XLName *input)            { return false; }
    bool Block(XLBlock *input)          { return false; }
    bool Prefix(XLPrefix *input)        { return false; }
    bool Infix(XLInfix *input)          { return false; }
    bool Builtin(XLBuiltin *input)      { return false; }
};


template <class Action>
struct XLDo
// ----------------------------------------------------------------------------
//   Perform the actions on all trees in turn
// ----------------------------------------------------------------------------
{
    XLDo(Action &act): action(act) {}

    bool operator() (XLTree *input)
    {
        bool done = false;
        while (!done && input)
        {
            switch (input->Kind())
            {
            case xlNATURAL:
                done = action.Natural((XLNatural *) input);
                input = NULL;
                break;
            case xlREAL:
                done = action.Real((XLReal *) input);
                input = NULL;
                break;
            case xlSTRING:
                done = action.String((XLString *) input);
                input = NULL;
                break;
            case xlNAME:
                done = action.Name((XLName *) input);
                input = NULL;
                break;
            case xlBLOCK:
                done = action.Block((XLBlock *) input);
                if (!done)
                    input = ((XLBlock *) input)->child;
                break;
            case xlPREFIX:
                done = action.Prefix ((XLPrefix *) input);
                if (!done)
                    done = (*this) (((XLPrefix *) input)->left);
                if (done)
                    input = NULL;
                else
                    input = ((XLPrefix *) input)->right;
                break;
            case xlINFIX:
                done = action.Infix ((XLInfix *) input);
                if (!done)
                    done = (*this) (((XLInfix *) input)->left);
                if (done)
                    input = NULL;
                else
                    input = ((XLInfix *) input)->right;
                break;
            case xlBUILTIN:
                done = action.Builtin((XLBuiltin *) input);
                input = NULL;
                break;
            default:
                std::cerr << "Invalid tree\n";
                input = NULL;
            }
        }
        return done;
    }
    Action &action;
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
