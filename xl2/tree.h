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
// This document is confidential.
// Do not redistribute without written permission
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include <ostream>
#include "base.h"


enum XLKind
// ----------------------------------------------------------------------------
//   The fixed set of tree types
// ----------------------------------------------------------------------------
{
    xlUNKNOWN,
    xlINTEGER, xlREAL, xlSTRING, xlNAME,        // Atoms
    xlBLOCK,                                    // Unary parentheses and block
    xlPREFIX,                                   // Binary prefix-op
    xlINFIX,                                    // Ternary infix-op
    xlBUILTIN,                                  // Special
    xlLAST
};


struct XLTree
// ----------------------------------------------------------------------------
//   The base class for all XL trees
// ----------------------------------------------------------------------------
{
    typedef std::ostream ostream;
    virtual void        Output(ostream &out) { out << "tree"; }
    virtual XLKind      Kind() { return xlUNKNOWN; }
    
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
    virtual XLKind      Kind() { return xlINTEGER; }
    longlong            value;
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
    XLBlock(XLTree *c, char i, char o): child(c), opening(i), closing(o) {}
    virtual void        Output(ostream &o);
    virtual XLKind      Kind() { return xlBLOCK; }
    XLTree              *child;
    char                opening, closing;
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
    bool Integer(XLInteger *input)      { return false; }
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
            case xlINTEGER:
                done = action.Integer((XLInteger *) input);
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
