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
#include <map>
#include <vector>

XL_BEGIN


// ============================================================================
// 
//    Foundation types
// 
// ============================================================================

struct Context;                                 // Execution context
struct Tree;                                    // Base tree
struct Integer;                                 // Integer: 0, 3, 8
struct Real;                                    // Real: 3.2, 1.6e4
struct Text;                                    // Text: "ABC"
struct Name;                                    // Name / symbol: ABC, ++-
struct Prefix;                                  // Prefix: sin X
struct Postfix;                                 // Postfix: 3!
struct Infix;                                   // Infix: A+B, newline
struct Block;                                   // Block: (A), {A}
struct Native;                                  // Some native code
struct Action;                                  // Action on trees
typedef ulong tree_position;                    // Position in context
typedef std::map<text, Tree *> tree_data;       // Data associated to tree
typedef std::vector<Tree *> tree_list;          // A list of trees


struct Tree
// ----------------------------------------------------------------------------
//   The base class for all XL trees
// ----------------------------------------------------------------------------
{
    // Constructor and destructor
    Tree (tree_position pos = NOWHERE): position(pos) {}
    virtual ~Tree() {}

    // Evaluate a tree
    virtual Tree *      Run(Context *context);
    virtual Tree *      Call(Context *context, Tree *args);

    // Perform recursive actions on a tree
    virtual Tree *      Do(Action *action);
    virtual void        DoData(Action *action);
    Tree *              Do(Action &action) { return Do(&action); }

    // Return in normalized form (i.e. using only trees in this file)
    virtual Tree *      Normalize();

    // Tree properties
    void                Set(text name, Tree *prop) { data[name] = prop; }
    Tree *              Get(text name)             { return data[name]; }
    tree_position       Position()                 { return position; }

    // Conversion to text
                        operator text();

    // Operator new to record the tree in the garbage collector
    void *              operator new(size_t sz);

protected:
    tree_position       position;
    tree_data           data;
    static tree_position NOWHERE;
};


struct Action
// ----------------------------------------------------------------------------
//   An operation we do recursively on trees
// ----------------------------------------------------------------------------
{
    Action () {}
    virtual ~Action() {}
    virtual Tree *Do (Tree *what) = 0;

    // Specialization for the canonical nodes, default is to run them
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);
    virtual Tree *DoNative(Native *what);
};



// ============================================================================
// 
//   Leaf nodes (integer, real, name, text)
// 
// ============================================================================

struct Integer : Tree
// ----------------------------------------------------------------------------
//   Integer constants
// ----------------------------------------------------------------------------
{
    Integer(longlong i = 0, tree_position pos = NOWHERE):
        Tree(pos), value(i) {}
    virtual Tree *      Do(Action *action);
    virtual Tree *      Run(Context *context)                 { return this; }
    virtual Tree *      Call(Context *context, Tree *args)    { return this; }
    longlong            value;
};


struct Real : Tree
// ----------------------------------------------------------------------------
//   Real numbers
// ----------------------------------------------------------------------------
{
    Real(double d = 0.0, tree_position pos = NOWHERE):
        Tree(pos), value(d) {}
    virtual Tree *      Do(Action *action);
    virtual Tree *      Run(Context *context)                 { return this; }
    virtual Tree *      Call(Context *context, Tree *args)    { return this; }
    double              value;
};


struct Text : Tree
// ----------------------------------------------------------------------------
//   Text, e.g. "Hello World"
// ----------------------------------------------------------------------------
{
    Text(text t = "", tree_position pos = NOWHERE):
        Tree(pos), value(t) {}
    virtual Tree *      Do(Action *action);
    virtual Tree *      Run(Context *context)                 { return this; }
    virtual Tree *      Call(Context *context, Tree *args)    { return this; }
    virtual text        Opening() { return "\""; }
    virtual text        Closing() { return "\""; }
    text                value;
};


struct Quote : Text
// ----------------------------------------------------------------------------
//   Quoted text, e.g. 'Hello World'
// ----------------------------------------------------------------------------
{
    Quote(text t = "", tree_position pos = NOWHERE):
        Text(t, pos) {}
    virtual text        Opening() { return "'"; }
    virtual text        Closing() { return "'"; }
};


struct LongText : Text
// ----------------------------------------------------------------------------
//   Long text with delimiters, e.g html This is some text end_html
// ----------------------------------------------------------------------------
{
    LongText(text t, text open, text close, tree_position pos = NOWHERE):
        Text(t, pos), opening(open), closing(close) {}
    virtual text        Opening() { return opening; }
    virtual text        Closing() { return closing; }
    text                opening, closing;
};


struct Comment : LongText
// ----------------------------------------------------------------------------
//   Comments, e.g. /* Hello World */
// ----------------------------------------------------------------------------
{
    Comment(text t, text open, text close, tree_position pos = NOWHERE):
        LongText(t, open, close, pos) {}
};


struct Name : Tree
// ----------------------------------------------------------------------------
//   A node representing a name or symbol
// ----------------------------------------------------------------------------
{
    Name(text n, tree_position pos = NOWHERE):
        Tree(pos), value(n) {}
    virtual Tree *      Do(Action *action);
    virtual Tree *      Run(Context *context);
    virtual Tree *      Call(Context *context, Tree *args);
    text                value;
};



// ============================================================================
// 
//   Structured types: Block, Prefix, Infix
// 
// ============================================================================

struct Block : Tree
// ----------------------------------------------------------------------------
//   A block, such as (X), {X}, [X] or indented block (defaults to indented)
// ----------------------------------------------------------------------------
{
    Block(Tree *c, tree_position pos = NOWHERE): Tree(pos), child(c) {}
    virtual text        Opening() { return "\t+"; }
    virtual text        Closing() { return "\t-"; }
    virtual Tree *      Do(Action *action);
    virtual Tree *      Run(Context *context);
    virtual Tree *      Call(Context *context, Tree *args);
    static Block *      MakeBlock(Tree *child,
                                  text open, text close,
                                  tree_position pos = NOWHERE);
    Tree *              child;
};


struct Parentheses : Block
// ----------------------------------------------------------------------------
//   Parentheses around a tree, e.g. (X)
// ----------------------------------------------------------------------------
{
    Parentheses(Tree *c, tree_position pos = NOWHERE): Block(c, pos) {}
    virtual text        Opening() { return "("; }
    virtual text        Closing() { return ")"; }
};


struct Brackets : Block
// ----------------------------------------------------------------------------
//   Brackets around a tree, e.g. [X]
// ----------------------------------------------------------------------------
{
    Brackets(Tree *c, tree_position pos = NOWHERE): Block(c, pos) {}
    virtual text        Opening() { return "["; }
    virtual text        Closing() { return "]"; }
};


struct Curly : Block
// ----------------------------------------------------------------------------
//   Brackets around a tree, e.g. [X]
// ----------------------------------------------------------------------------
{
    Curly(Tree *c, tree_position pos = NOWHERE): Block(c, pos) {}
    virtual text        Opening() { return "{"; }
    virtual text        Closing() { return "}"; }
};


struct DelimitedBlock : Block
// ----------------------------------------------------------------------------
//   A block delimited by arbitrary opening/closing delimiters
// ----------------------------------------------------------------------------
{
    DelimitedBlock(Tree *c, text open, text close, tree_position pos=NOWHERE):
        Block(c, pos), opening(open), closing(close) {}
    virtual text        Opening() { return opening; }
    virtual text        Closing() { return closing; }
    text                opening, closing;
};


struct Prefix : Tree
// ----------------------------------------------------------------------------
//   A prefix operator, e.g. sin X, +3
// ----------------------------------------------------------------------------
{
    Prefix(Tree *l, Tree *r, tree_position pos = NOWHERE):
        Tree(pos), left(l), right(r) {}
    virtual Tree *      Do(Action *action);
    virtual Tree *      Run(Context *context);
    virtual Tree *      Call(Context *context, Tree *args);
    Tree *              left;
    Tree *              right;
};


struct Postfix : Prefix
// ----------------------------------------------------------------------------
//   A postfix operator, e.g. 3!
// ----------------------------------------------------------------------------
{
    Postfix(Tree *l, Tree *r, tree_position pos = NOWHERE):
        Prefix(l, r, pos) {}
    virtual Tree *      Do(Action *action);
    virtual Tree *      Run(Context *context);
    virtual Tree *      Call(Context *context, Tree *args);
};


struct Infix : Prefix
// ----------------------------------------------------------------------------
//   Infix operators, e.g. A+B, A and B, A,B,C,D,E
// ----------------------------------------------------------------------------
//   Since it's very frequent to have sequences with the same separator
//   e.g. blocks of code, where the separator is newline, or arguments
//   where the separator is a comma, the parser groups such identical
//   infix operators in a single Infix tree. This is the reason it holds
//   a tree list and not a just left and right
{
    Infix(text n, Tree *left, Tree *right, tree_position pos = NOWHERE):
        Prefix(left, right, pos), name(n) {}
    virtual Tree *      Do(Action *action);
    virtual Tree *      Run(Context *context);
    virtual Tree *      Call(Context *context, Tree *args);
    text                name;
};


struct Native : Tree
// ----------------------------------------------------------------------------
//   A native tree is intended to represent directly executable code
// ----------------------------------------------------------------------------
{
    Native(tree_position pos = NOWHERE): Tree(pos) {}
    virtual Tree *      Do(Action *action);
    virtual Tree *      Run(Context *context);
    virtual text        TypeName();
};
    
XL_END

#endif // TREE_H
