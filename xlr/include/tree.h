#ifndef TREE_H
#define TREE_H
// ****************************************************************************
//  tree.h                          (C) 1992-2003 Christophe de Dinechin (ddd)
//                                                            XL2 project
// ****************************************************************************
//
//   File Description:
//
//     Basic representation of parse tree.
//
//     See the big comment at the top of parser.h for details about
//     the basics of XL tree representation
//
//
//
//
//
//
// ****************************************************************************
// This program is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "base.h"
#include "gc.h"
#include "info.h"
#include <map>
#include <vector>
#include <cassert>

#include <iostream>

XL_BEGIN

// ============================================================================
//
//    The types being defined or used to define XL trees
//
// ============================================================================

struct Tree;                                    // Base tree
struct Integer;                                 // Integer: 0, 3, 8
struct Real;                                    // Real: 3.2, 1.6e4
struct Text;                                    // Text: "ABC"
struct Name;                                    // Name / symbol: ABC, ++-
struct Block;                                   // Block: (A), {A}
struct Prefix;                                  // Prefix: sin X
struct Postfix;                                 // Postfix: 3!
struct Infix;                                   // Infix: A+B, newline
struct Info;                                    // Information in trees
struct Context;                                 // Execution context
struct Symbols;                                 // Symbol table
struct Sha1;                                    // Hash used for id-ing trees



// ============================================================================
//
//    Pointer and structure types
//
// ============================================================================

typedef GCPtr<Tree>                     Tree_p;
typedef GCPtr<Integer, longlong>        Integer_p;
typedef GCPtr<Real, double>             Real_p;
typedef GCPtr<Text, text>               Text_p;
typedef GCPtr<Name>                     Name_p;
typedef GCPtr<Block>                    Block_p;
typedef GCPtr<Prefix>                   Prefix_p;
typedef GCPtr<Postfix>                  Postfix_p;
typedef GCPtr<Infix>                    Infix_p;
typedef GCPtr<Symbols>                  Symbols_p;

typedef ulong TreePosition;                     // Position in source files
typedef std::vector<Tree_p> TreeList;           // A list of trees
typedef Tree *(*eval_fn) (Context *, Tree *);   // Compiled evaluation code



// ============================================================================
//
//    The Tree class
//
// ============================================================================

enum kind
// ----------------------------------------------------------------------------
//   The kinds of tree that compose an XL parse tree
// ----------------------------------------------------------------------------
{
    INTEGER, REAL, TEXT, NAME,                  // Leaf nodes
    BLOCK, PREFIX, POSTFIX, INFIX,              // Non-leaf nodes

    KIND_FIRST          = INTEGER,
    KIND_LAST           = INFIX,
    KIND_LEAF_FIRST     = INTEGER,
    KIND_LEAF_LAST      = NAME,
    KIND_NLEAF_FIRST    = BLOCK,
    KIND_NLEAF_LAST     = INFIX
};


struct Tree
// ----------------------------------------------------------------------------
//   The base class for all XL trees
// ----------------------------------------------------------------------------
{
    enum { KINDBITS = 3, KINDMASK=7 };
    enum { NOWHERE = ~0UL };

    // Constructor and destructor
    Tree (kind k, TreePosition pos = NOWHERE):
        tag((pos<<KINDBITS) | k), info(NULL), code(NULL), symbols(NULL) {}
    Tree(kind k, Tree *from):
        tag(from->tag), info(from->info ? from->info->Copy() : NULL),
        code(NULL), symbols(NULL)
    {
        assert(k == Kind()); (void) k;
    }
    ~Tree();

    // Perform recursive actions on a tree
    template<typename Action>
    typename Action::value_type Do(Action &a);
    template<typename Action>
    typename Action::value_type Do(Action *a) { return Do<Action>(*a); }

    // Attributes
    kind                Kind()                { return kind(tag & KINDMASK); }
    TreePosition        Position()            { return (long) tag>>KINDBITS; }
    bool                IsLeaf()              { return Kind() <= NAME; }
    bool                IsConstant()          { return Kind() <= TEXT; }

    // Safe cast to an appropriate subclass
    Integer *           AsInteger();
    Real *              AsReal();
    Text *              AsText();
    Name *              AsName();
    Block *             AsBlock();
    Infix *             AsInfix();
    Prefix *            AsPrefix();
    Postfix *           AsPostfix();

    // Symbols management (obsolete)
    XL::Symbols *       Symbols()                       { return symbols; }
    void                SetSymbols(XL::Symbols *s)      { symbols = s; }

    // Info management
    template<class I>
    typename I::data_t  Get() const;
    template<class I>
    void                Set(typename I::data_t data);
    template<class I>
    void                Set2(typename I::data_t data);
    template<class I>
    I*                  GetInfo() const;
    template<class I>
    void                SetInfo(I *);
    template<class I>
    bool                Exists() const;
    template <class I>
    bool                Purge();
    template <class I>
    I*                  Remove();
    template <class I>
    I*                  Remove(I *);


    // Conversion to text
                        operator text();

public:
    static int          Compare(Tree *t1, Tree *t2, bool recurse = true);
    static bool         Equal(Tree *t1, Tree *t2, bool recurse = true);

public:
    ulong       tag;                            // Position + kind
    Info *      info;                           // Information for tree

    // OBSOLETE FIELDS (should go away soon)
    eval_fn     code;                           // Compiled code
    Symbols_p   symbols;                        // Symbol table for evaluation

    GARBAGE_COLLECT(Tree);

private:
    Tree (const Tree &);
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
    Integer(longlong i = 0, TreePosition pos = NOWHERE):
        Tree(INTEGER, pos), value(i) {}
    Integer(Integer *i): Tree(INTEGER, i), value(i->value) {}
    longlong            value;
    operator longlong()         { return value; }

    GARBAGE_COLLECT(Integer);
};
template<>inline Integer_p::operator longlong() const { return pointer->value; }


struct Real : Tree
// ----------------------------------------------------------------------------
//   Real numbers
// ----------------------------------------------------------------------------
{
    Real(double d = 0.0, TreePosition pos = NOWHERE):
        Tree(REAL, pos), value(d) {}
    Real(Real *r): Tree(REAL, r), value(r->value) {}
    double              value;
    operator double()           { return value; }
    GARBAGE_COLLECT(Real);
};
template<> inline Real_p::operator double() const   { return pointer->value; }


struct Text : Tree
// ----------------------------------------------------------------------------
//   Text, e.g. "Hello World"
// ----------------------------------------------------------------------------
{
    Text(text t, text open="\"", text close="\"", TreePosition pos=NOWHERE):
        Tree(TEXT, pos), value(t), opening(open), closing(close) {}
    Text(Text *t):
        Tree(TEXT, t),
        value(t->value), opening(t->opening), closing(t->closing) {}
    text                value;
    text                opening, closing;
    static text         textQuote, charQuote;
    operator text()     { return value; }
    bool IsCharacter()  { return opening == charQuote && closing == charQuote; }
    bool IsText()       { return opening == textQuote && closing == textQuote; }

    GARBAGE_COLLECT(Text);
};
template<> inline Text_p::operator text() const  { return pointer->value; }


struct Name : Tree
// ----------------------------------------------------------------------------
//   A node representing a name or symbol
// ----------------------------------------------------------------------------
{
    Name(text n, TreePosition pos = NOWHERE):
        Tree(NAME, pos), value(n) {}
    Name(Name *n):
        Tree(NAME, n), value(n->value) {}
    text                value;
    operator bool();
    GARBAGE_COLLECT(Name);
};



// ============================================================================
//
//   Structured types: Block, Prefix, Infix
//
// ============================================================================

struct Block : Tree
// ----------------------------------------------------------------------------
//   A block, such as (X), {X}, [X] or indented block
// ----------------------------------------------------------------------------
{
    Block(Tree *c, text open, text close, TreePosition pos = NOWHERE):
        Tree(BLOCK, pos), child(c), opening(open), closing(close) {}
    Block(Block *b, Tree *ch):
        Tree(BLOCK, b),
        child(ch), opening(b->opening), closing(b->closing) {}
    bool IsIndent()     { return opening == indent && closing == unindent; }
    bool IsParentheses(){ return opening == "(" && closing == ")"; }
    bool IsBraces()     { return opening == "{" && closing == "}"; }
    bool IsSquare()     { return opening == "[" && closing == "]"; }
    bool IsGroup()      { return IsIndent() || IsParentheses() || IsBraces(); }
    Tree_p              child;
    text                opening, closing;
    static text         indent, unindent;
    GARBAGE_COLLECT(Block);
};


struct Prefix : Tree
// ----------------------------------------------------------------------------
//   A prefix operator, e.g. sin X, +3
// ----------------------------------------------------------------------------
{
    Prefix(Tree *l, Tree *r, TreePosition pos = NOWHERE):
        Tree(PREFIX, pos), left(l), right(r) {}
    Prefix(Prefix *p, Tree *l, Tree *r):
        Tree(PREFIX, p), left(l), right(r) {}
    Tree_p               left;
    Tree_p               right;
    GARBAGE_COLLECT(Prefix);
};


struct Postfix : Tree
// ----------------------------------------------------------------------------
//   A postfix operator, e.g. 3!
// ----------------------------------------------------------------------------
{
    Postfix(Tree *l, Tree *r, TreePosition pos = NOWHERE):
        Tree(POSTFIX, pos), left(l), right(r) {}
    Postfix(Postfix *p, Tree *l, Tree *r):
        Tree(POSTFIX, p), left(l), right(r) {}
    Tree_p              left;
    Tree_p              right;
    GARBAGE_COLLECT(Postfix);
};


struct Infix : Tree
// ----------------------------------------------------------------------------
//   Infix operators, e.g. A+B, A and B, A,B,C,D,E
// ----------------------------------------------------------------------------
{
    Infix(text n, Tree *l, Tree *r, TreePosition pos = NOWHERE):
        Tree(INFIX, pos), left(l), right(r), name(n) {}
    Infix(Infix *i, Tree *l, Tree *r):
        Tree(INFIX, i), left(l), right(r), name(i->name) {}
    Infix *             LastStatement();
    Tree_p              left;
    Tree_p              right;
    text                name;
    GARBAGE_COLLECT(Infix);
};



// ============================================================================
//
//    Safe casts
//
// ============================================================================

inline Integer *Tree::AsInteger()
// ----------------------------------------------------------------------------
//    Return a pointer to an Integer or NULL
// ----------------------------------------------------------------------------
{
    if (this && Kind() == INTEGER)
        return (Integer *) this;
    return NULL;
}


inline Real *Tree::AsReal()
// ----------------------------------------------------------------------------
//    Return a pointer to an Real or NULL
// ----------------------------------------------------------------------------
{
    if (this && Kind() == REAL)
        return (Real *) this;
    return NULL;
}


inline Text *Tree::AsText()
// ----------------------------------------------------------------------------
//    Return a pointer to an Text or NULL
// ----------------------------------------------------------------------------
{
    if (this && Kind() == TEXT)
        return (Text *) this;
    return NULL;
}


inline Name *Tree::AsName()
// ----------------------------------------------------------------------------
//    Return a pointer to an Name or NULL
// ----------------------------------------------------------------------------
{
    if (this && Kind() == NAME)
        return (Name *) this;
    return NULL;
}


inline Block *Tree::AsBlock()
// ----------------------------------------------------------------------------
//    Return a pointer to an Block or NULL
// ----------------------------------------------------------------------------
{
    if (this && Kind() == BLOCK)
        return (Block *) this;
    return NULL;
}


inline Infix *Tree::AsInfix()
// ----------------------------------------------------------------------------
//    Return a pointer to an Infix or NULL
// ----------------------------------------------------------------------------
{
    if (this && Kind() == INFIX)
        return (Infix *) this;
    return NULL;
}


inline Prefix *Tree::AsPrefix()
// ----------------------------------------------------------------------------
//    Return a pointer to an Prefix or NULL
// ----------------------------------------------------------------------------
{
    if (this && Kind() == PREFIX)
        return (Prefix *) this;
    return NULL;
}


inline Postfix *Tree::AsPostfix()
// ----------------------------------------------------------------------------
//    Return a pointer to an Postfix or NULL
// ----------------------------------------------------------------------------
{
    if (this && Kind() == POSTFIX)
        return (Postfix *) this;
    return NULL;
}


inline Infix *Infix::LastStatement()
// ----------------------------------------------------------------------------
//   Return the last statement following a given infix
// ----------------------------------------------------------------------------
{
    Infix *last = this;
    Infix *next;
    while ((next = last->right->AsInfix()) &&
           (next->name == "\n" || next->name == ";"))
        last = next;
    return last;
}



// ============================================================================
// 
//   Template members for recursive operations on trees
// 
// ============================================================================

template<typename Action>
typename Action::value_type Tree::Do(Action &action)
// ----------------------------------------------------------------------------
//   Perform an action on the tree 
// ----------------------------------------------------------------------------
{
    switch(Kind())
    {
    case INTEGER:       return action.DoInteger((Integer *) this);
    case REAL:          return action.DoReal((Real *) this);
    case TEXT:          return action.DoText((Text *) this);
    case NAME:          return action.DoName((Name *) this);
    case BLOCK:         return action.DoBlock((Block *) this);
    case PREFIX:        return action.DoPrefix((Prefix *) this);
    case POSTFIX:       return action.DoPostfix((Postfix *) this);
    case INFIX:         return action.DoInfix((Infix *) this);
    default:            assert(!"Unexpected tree kind");
    }
    return typename Action::value_type();
}


inline bool Tree::Equal(Tree *t1, Tree *t2, bool recurse)
// ----------------------------------------------------------------------------
//   Compare for equality
// ----------------------------------------------------------------------------
{
    return Compare(t1, t2, recurse) == 0;
}



// ============================================================================
//
//    Template members for info management
//
// ============================================================================

template <class I> inline typename I::data_t Tree::Get() const
// ----------------------------------------------------------------------------
//   Find if we have an information of the right type in 'info'
// ----------------------------------------------------------------------------
{
    for (Info *i = info; i; i = i->next)
        if (I *ic = dynamic_cast<I *> (i))
            return (typename I::data_t) *ic;
    return typename I::data_t();
}


template <class I> inline void Tree::Set(typename I::data_t data)
// ----------------------------------------------------------------------------
//   Set the information given as an argument
// ----------------------------------------------------------------------------
{
    Info *i = new I(data);
    i->next = info;
    info = i;
}


template <class I> inline I* Tree::GetInfo() const
// ----------------------------------------------------------------------------
//   Find if we have an information of the right type in 'info'
// ----------------------------------------------------------------------------
{
    for (Info *i = info; i; i = i->next)
        if (I *ic = dynamic_cast<I *> (i))
            return ic;
    return NULL;
}


template <class I> inline void Tree::Set2(typename I::data_t data)
// ----------------------------------------------------------------------------
//   Set the information given as an argument. Do not add new info if exists.
// ----------------------------------------------------------------------------
{
    I *i = GetInfo<I>();
    if (i)
        (*i) = data;
    else
        Set<I>(data);
}


template <class I> inline void Tree::SetInfo(I *i)
// ----------------------------------------------------------------------------
//   Set the information given as an argument
// ----------------------------------------------------------------------------
{
    Info *last = i;
    while(last->next)
        last = last->next;
    last->next = info;
    info = i;
}


template <class I> inline bool Tree::Exists() const
// ----------------------------------------------------------------------------
//   Verifies if the tree already has information of the given type
// ----------------------------------------------------------------------------
{
    for (Info *i = info; i; i = i->next)
        if (dynamic_cast<I *> (i))
            return true;
    return false;
}


template <class I> inline bool Tree::Purge()
// ----------------------------------------------------------------------------
//   Find and purge information of the given type
// ----------------------------------------------------------------------------
{
    Info *last = NULL;
    Info *next = NULL;
    bool purged = false;
    for (Info *i = info; i; i = next)
    {
        next = i->next;
        if (I *ic = dynamic_cast<I *> (i))
        {
            if (last)
                last->next = next;
            else
                info = next;
            ic->Delete();
            purged = true;
        }
        else
        {
            last = i;
        }
    }
    return purged;
}


template <class I> inline I* Tree::Remove()
// ----------------------------------------------------------------------------
//   Find information and unlinks it if it exists
// ----------------------------------------------------------------------------
{
    Info *prev = NULL;
    for (Info *i = info; i; i = i->next)
    {
        if (I *ic = dynamic_cast<I *> (i))
        {
            if (prev)
                prev->next = i->next;
            else
                info = i->next;
            i->next = NULL;
            return ic;
        }
        prev = i;
    }
    return NULL;
}


template <class I> inline I* Tree::Remove(I *toFind)
// ----------------------------------------------------------------------------
//   Find information matching input and remove it if it exists
// ----------------------------------------------------------------------------
{
    Info *prev = NULL;
    for (Info *i = info; i; i = i->next)
    {
        I *ic = dynamic_cast<I *> (i);
        if (ic == toFind)
        {
            if (prev)
                prev->next = i->next;
            else
                info = i->next;
            i->next = NULL;
            return ic;
        }
        prev = i;
    }
    return NULL;
}

extern Name_p   xl_true;
extern Name_p   xl_false;
extern Name_p   xl_nil;
extern Name_p   xl_empty;

XL_END

#endif // TREE_H
