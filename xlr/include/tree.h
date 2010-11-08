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
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "base.h"
#include "gc.h"
#include "info.h"
#include "action.h"
#include <map>
#include <vector>
#include <cassert>


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
struct Prefix;                                  // Prefix: sin X
struct Postfix;                                 // Postfix: 3!
struct Infix;                                   // Infix: A+B, newline
struct Block;                                   // Block: (A), {A}
struct Action;                                  // Action on trees
struct Info;                                    // Information in trees
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
typedef GCPtr<Prefix>                   Prefix_p;
typedef GCPtr<Postfix>                  Postfix_p;
typedef GCPtr<Infix>                    Infix_p;
typedef GCPtr<Block>                    Block_p;
typedef GCPtr<Symbols>                  Symbols_p;

typedef ulong TreePosition;                     // Position in context
typedef std::vector<Tree_p> TreeList;           // A list of trees
typedef Tree *(*eval_fn) (Tree *);              // Compiled evaluation code



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
        tag((pos<<KINDBITS) | k),
        code(NULL), symbols(NULL), info(NULL), source(NULL) {}
    Tree(kind k, Tree *from):
        tag(from->tag),
        code(from->code), symbols(from->symbols),
        info(from->info ? from->info->Copy() : NULL), source(from)
    {
        assert(k == Kind());
    }
    ~Tree();

    // Perform recursive actions on a tree
    Tree *              Do(Action *action);
    Tree *              Do(Action &action)    { return Do(&action); }

    // Attributes
    kind                Kind()                { return kind(tag & KINDMASK); }
    TreePosition        Position()            { return (long) tag>>KINDBITS; }
    bool                IsLeaf()              { return Kind() <= NAME; }
    bool                IsConstant()          { return Kind() <= TEXT; }
    XL::Symbols *       Symbols()             { return symbols; }
    void                SetSymbols(XL::Symbols *s) { symbols = s; }


    // Info
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


    // Safe cast to an appropriate subclass
    Integer *           AsInteger();
    Real *              AsReal();
    Text *              AsText();
    Name *              AsName();
    Block *             AsBlock();
    Infix *             AsInfix();
    Prefix *            AsPrefix();
    Postfix *           AsPostfix();

    // Conversion to text
                        operator text();

public:
    ulong       tag;                            // Position + kind
    eval_fn     code;                           // Compiled code
    Symbols_p   symbols;                        // Symbol table for evaluation
    Info *      info;                           // Information for tree
    Tree_p      source;                         // Source for the tree

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
    operator text()             { return value; }
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
            delete ic;
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



// ============================================================================
//
//    Tree cloning
//
// ============================================================================

struct DeepCopyCloneMode;       // Child nodes are cloned too (default)
struct ShallowCopyCloneMode;    // Child nodes are referenced
struct NodeOnlyCloneMode;       // Child nodes are left NULL


template <typename mode> struct TreeCloneTemplate : Action
// ----------------------------------------------------------------------------
//   Clone a tree
// ----------------------------------------------------------------------------
{
    TreeCloneTemplate() {}
    virtual ~TreeCloneTemplate() {}

    Tree *DoInteger(Integer *what)
    {
        return new Integer(what->value, what->Position());
    }
    Tree *DoReal(Real *what)
    {
        return new Real(what->value, what->Position());

    }
    Tree *DoText(Text *what)
    {
        return new Text(what->value, what->opening, what->closing,
                        what->Position());
    }
    Tree *DoName(Name *what)
    {
        return new Name(what->value, what->Position());
    }

    Tree *DoBlock(Block *what)
    {
        return new Block(Clone(what->child), what->opening, what->closing,
                         what->Position());
    }
    Tree *DoInfix(Infix *what)
    {
        return new Infix (what->name, Clone(what->left), Clone(what->right),
                          what->Position());
    }
    Tree *DoPrefix(Prefix *what)
    {
        return new Prefix(Clone(what->left), Clone(what->right),
                          what->Position());
    }
    Tree *DoPostfix(Postfix *what)
    {
        return new Postfix(Clone(what->left), Clone(what->right),
                           what->Position());
    }
    Tree *Do(Tree *what)
    {
        return what;            // ??? Should not happen
    }
protected:
    // Default is to do a deep copy
    Tree *  Clone(Tree *t) { return t->Do(this); }
};


template<> inline
Tree *TreeCloneTemplate<ShallowCopyCloneMode>::Clone(Tree *t)
// ----------------------------------------------------------------------------
//   Specialization for the shallow copy clone
// ----------------------------------------------------------------------------
{
    return t;
}


template<> inline
Tree *TreeCloneTemplate<NodeOnlyCloneMode>::Clone(Tree *)
// ----------------------------------------------------------------------------
//   Specialization for the node-only clone
// ----------------------------------------------------------------------------
{
    return NULL;
}


typedef struct TreeCloneTemplate<DeepCopyCloneMode>     TreeClone;
typedef struct TreeCloneTemplate<ShallowCopyCloneMode>  ShallowCopyTreeClone;
typedef struct TreeCloneTemplate<NodeOnlyCloneMode>     NodeOnlyTreeClone;



// ============================================================================
//
//    Tree copying
//
// ============================================================================

enum CopyMode
// ----------------------------------------------------------------------------
//   Several ways of copying a tree.
// ----------------------------------------------------------------------------
{
    CM_RECURSIVE = 1,    // Copy child nodes (as long as their kind match)
    CM_NODE_ONLY         // Copy only one node
};


template <CopyMode mode> struct TreeCopyTemplate : Action
// ----------------------------------------------------------------------------
//   Copy a tree into another tree. Node values are copied, infos are not.
// ----------------------------------------------------------------------------
{
    TreeCopyTemplate(Tree *dest): dest(dest) {}
    virtual ~TreeCopyTemplate() {}

    Tree *DoInteger(Integer *what)
    {
        if (Integer *it = dest->AsInteger())
        {
            it->value = what->value;
            it->tag = ((what->Position()<<Tree::KINDBITS) | it->Kind());
            return what;
        }
        return NULL;
    }
    Tree *DoReal(Real *what)
    {
        if (Real *rt = dest->AsReal())
        {
            rt->value = what->value;
            rt->tag = ((what->Position()<<Tree::KINDBITS) | rt->Kind());
            return what;
        }
        return NULL;
    }
    Tree *DoText(Text *what)
    {
        if (Text *tt = dest->AsText())
        {
            tt->value = what->value;
            tt->tag = ((what->Position()<<Tree::KINDBITS) | tt->Kind());
            return what;
        }
        return NULL;
    }
    Tree *DoName(Name *what)
    {
        if (Name *nt = dest->AsName())
        {
            nt->value = what->value;
            nt->tag = ((what->Position()<<Tree::KINDBITS) | nt->Kind());
            return what;
        }
        return NULL;
    }

    Tree *DoBlock(Block *what)
    {
        if (Block *bt = dest->AsBlock())
        {
            bt->opening = what->opening;
            bt->closing = what->closing;
            bt->tag = ((what->Position()<<Tree::KINDBITS) | bt->Kind());
            if (mode == CM_RECURSIVE)
            {
                dest = bt->child;
                Tree *  br = what->child->Do(this);
                dest = bt;
                return br;
            }
            return what;
        }
        return NULL;
    }
    Tree *DoInfix(Infix *what)
    {
        if (Infix *it = dest->AsInfix())
        {
            it->name = what->name;
            it->tag = ((what->Position()<<Tree::KINDBITS) | it->Kind());
            if (mode == CM_RECURSIVE)
            {
                dest = it->left;
                Tree *lr = what->left->Do(this);
                dest = it;
                if (!lr)
                    return NULL;
                dest = it->right;
                Tree *rr = what->right->Do(this);
                dest = it;
                if (!rr)
                    return NULL;
            }
            return what;
        }
        return NULL;
    }
    Tree *DoPrefix(Prefix *what)
    {
        if (Prefix *pt = dest->AsPrefix())
        {
            pt->tag = ((what->Position()<<Tree::KINDBITS) | pt->Kind());
            if (mode == CM_RECURSIVE)
            {
                dest = pt->left;
                Tree *lr = what->left->Do(this);
                dest = pt;
                if (!lr)
                    return NULL;
                dest = pt->right;
                Tree *rr = what->right->Do(this);
                dest = pt;
                if (!rr)
                    return NULL;
            }
            return what;
        }
        return NULL;
    }
    Tree *DoPostfix(Postfix *what)
    {
        if (Postfix *pt = dest->AsPostfix())
        {
            pt->tag = ((what->Position()<<Tree::KINDBITS) | pt->Kind());
            if (mode == CM_RECURSIVE)
            {
                dest = pt->left;
                Tree *lr = what->left->Do(this);
                dest = pt;
                if (!lr)
                    return NULL;
                dest = pt->right;
                Tree *rr = what->right->Do(this);
                dest = pt;
                if (!rr)
                    return NULL;
            }
            return what;
        }
        return NULL;
    }
    Tree *Do(Tree *what)
    {
        return what;            // ??? Should not happen
    }
    Tree *dest;
};



// ============================================================================
//
//    Tree shape equality comparison
//
// ============================================================================

enum TreeMatchMode
// ----------------------------------------------------------------------------
//   The ways of comparing trees
// ----------------------------------------------------------------------------
{
    TM_RECURSIVE = 1,  // Compare whole tree
    TM_NODE_ONLY       // Compare one node only
};


template <TreeMatchMode mode> struct TreeMatchTemplate : Action
// ----------------------------------------------------------------------------
//   Check if two trees match in structure
// ----------------------------------------------------------------------------
{
    TreeMatchTemplate (Tree *t): test(t) {}
    Tree *DoInteger(Integer *what)
    {
        if (Integer *it = test->AsInteger())
            if (it->value == what->value)
                return what;
        return NULL;
    }
    Tree *DoReal(Real *what)
    {
        if (Real *rt = test->AsReal())
            if (rt->value == what->value)
                return what;
        return NULL;
    }
    Tree *DoText(Text *what)
    {
        if (Text *tt = test->AsText())
            if (tt->value == what->value)
                return what;
        return NULL;
    }
    Tree *DoName(Name *what)
    {
        if (Name *nt = test->AsName())
            if (nt->value == what->value)
                return what;
        return NULL;
    }

    Tree *DoBlock(Block *what)
    {
        // Test if we exactly match the block, i.e. the reference is a block
        if (Block *bt = test->AsBlock())
        {
            if (bt->opening == what->opening &&
                bt->closing == what->closing)
            {
                if (mode == TM_NODE_ONLY)
                    return what;
                test = bt->child;
                Tree *br = what->child->Do(this);
                test = bt;
                if (br)
                    return br;
            }
        }
        return NULL;
    }
    Tree *DoInfix(Infix *what)
    {
        if (Infix *it = test->AsInfix())
        {
            // Check if we match the tree, e.g. A+B vs 2+3
            if (it->name == what->name)
            {
                if (mode == TM_NODE_ONLY)
                    return what;
                test = it->left;
                Tree *lr = what->left->Do(this);
                test = it;
                if (!lr)
                    return NULL;
                test = it->right;
                Tree *rr = what->right->Do(this);
                test = it;
                if (!rr)
                    return NULL;
                return what;
            }
        }
        return NULL;
    }
    Tree *DoPrefix(Prefix *what)
    {
        if (Prefix *pt = test->AsPrefix())
        {
            if (mode == TM_NODE_ONLY)
                return what;
            // Check if we match the tree, e.g. f(A) vs. f(2)
            test = pt->left;
            Tree *lr = what->left->Do(this);
            test = pt;
            if (!lr)
                return NULL;
            test = pt->right;
            Tree *rr = what->right->Do(this);
            test = pt;
            if (!rr)
                return NULL;
            return what;
        }
        return NULL;
    }
    Tree *DoPostfix(Postfix *what)
    {
        if (Postfix *pt = test->AsPostfix())
        {
            if (mode == TM_NODE_ONLY)
                return what;
            // Check if we match the tree, e.g. A! vs 2!
            test = pt->right;
            Tree *rr = what->right->Do(this);
            test = pt;
            if (!rr)
                return NULL;
            test = pt->left;
            Tree *lr = what->left->Do(this);
            test = pt;
            if (!lr)
                return NULL;
            return what;
        }
        return NULL;
    }
    Tree *Do(Tree *)
    {
        return NULL;
    }

    Tree *       test;
};

typedef struct TreeMatchTemplate<TM_RECURSIVE> TreeMatch;



// ============================================================================
//
//    Hash key for tree rewrite
//
// ============================================================================
//  We use this hashing key to quickly determine if two trees "look the same"

struct RewriteKey : Action
// ----------------------------------------------------------------------------
//   Compute a hashing key for a rewrite
// ----------------------------------------------------------------------------
{
    RewriteKey (ulong base = 0): key(base) {}
    ulong Key()  { return key; }

    ulong Hash(ulong id, text t)
    {
        ulong result = 0xC0DED;
        text::iterator p;
        for (p = t.begin(); p != t.end(); p++)
            result = (result * 0x301) ^ *p;
        return id | (result << 3);
    }
    ulong Hash(ulong id, ulong value)
    {
        return id | (value << 3);
    }

    Tree *DoInteger(Integer *what)
    {
        key = (key << 3) ^ Hash(0, what->value);
        return what;
    }
    Tree *DoReal(Real *what)
    {
        ulong *value = (ulong*)&what->value;
        key = (key << 3) ^ Hash(1, *value);
        return what;
    }
    Tree *DoText(Text *what)
    {
        key = (key << 3) ^ Hash(2, what->value);
        return what;
    }
    Tree *DoName(Name *what)
    {
        key = (key << 3) ^ Hash(3, what->value);
        return what;
    }

    Tree *DoBlock(Block *what)
    {
        key = (key << 3) ^ Hash(4, what->opening + what->closing);
        return what;
    }
    Tree *DoInfix(Infix *what)
    {
        key = (key << 3) ^ Hash(5, what->name);
        return what;
    }
    Tree *DoPrefix(Prefix *what)
    {
        ulong old = key;
        key = 0; what->left->Do(this);
        key = (old << 3) ^ Hash(6, key);
        return what;
    }
    Tree *DoPostfix(Postfix *what)
    {
        ulong old = key;
        key = 0; what->right->Do(this);
        key = (old << 3) ^ Hash(7, key);
        return what;
    }
    Tree *Do(Tree *what)
    {
        key = (key << 3) ^ Hash(1, (ulong) (Tree *) what);
        return what;
    }

    ulong key;
};


extern text     sha1(Tree *t);
extern Name_p   xl_true;
extern Name_p   xl_false;


typedef long node_id;              // A node identifier


struct NodeIdInfo : Info
// ----------------------------------------------------------------------------
//   Node identifier information
// ----------------------------------------------------------------------------
{
    NodeIdInfo(node_id id): id(id) {}

    typedef node_id data_t;
    operator data_t() { return id; }
    node_id id;
};


struct SimpleAction : Action
// ----------------------------------------------------------------------------
//   Holds a method to be run on any kind of tree node
// ----------------------------------------------------------------------------
{
    SimpleAction() {}
    virtual ~SimpleAction() {}
    Tree *DoBlock(Block *what)
    {
        return Do(what);
    }
    Tree *DoInfix(Infix *what)
    {
        return Do(what);
    }
    Tree *DoPrefix(Prefix *what)
    {
        return Do(what);
    }
    Tree *DoPostfix(Postfix *what)
    {
        return Do(what);
    }
    virtual Tree *  Do(Tree *what) = 0;
};


struct SetNodeIdAction : SimpleAction
// ------------------------------------------------------------------------
//   Set an integer node ID to each node.
// ------------------------------------------------------------------------
{
    SetNodeIdAction(node_id from_id = 1, node_id step = 1)
        : id(from_id), step(step) {}
    virtual Tree *Do(Tree *what)
    {
        what->Set<NodeIdInfo>(id);
        id += step;
        return NULL;
    }
    node_id id;
    node_id step;
};


struct FindParentAction : Action
// ------------------------------------------------------------------------
//   Find an ancestor of a node
// ------------------------------------------------------------------------
// "level" gives the depth of the parent. 0 means the node itself, 1 is the
// parent, 2 the grand-parent, etc...
// "path" is a string containing the path between the node and its ancestor:
// l means left, r means right and c means child of block.
{
    FindParentAction(Tree *self, uint level = 1):
        child(self), level(level), path() {}

    Tree *DoInteger(Integer *what)
    {
        if (child == what)
            return what;
        return NULL;
    }
    Tree *DoReal(Real *what)
    {
        if (child == what)
            return what;
        return NULL;
    }
    Tree *DoText(Text *what)
    {
        if (child == what)
            return what;
        return NULL;
    }
    Tree *DoName(Name *what)
    {
        if (child == what)
            return what;
        return NULL;
    }
    Tree *FindParent(Tree *child, kstring subpath)
    {
        if (Tree *parent = Do(child))
        {
            if (level <= 0)
                return parent;
            path.append(subpath);
            level--;
            return child;
        }
        return NULL;
    }

    Tree *DoPrefix(Prefix *what)
    {
        if (child == what)
            return what;
        if (Tree *left = FindParent(what->left, "l"))
            return left;
        if (Tree *right = FindParent(what->right, "r"))
            return right;
        return NULL;
    }

    Tree *DoPostfix(Postfix *what)
    {
        if (child == what)
            return what;
        if (Tree *left = FindParent(what->left, "l"))
            return left;
        if (Tree *right = FindParent(what->right, "r"))
            return right;
        return NULL;
    }

    Tree *DoInfix(Infix *what)
    {
        if (child == what)
            return what;
        if (Tree *left = FindParent(what->left, "l"))
            return left;
        if (Tree *right = FindParent(what->right, "r"))
            return right;
        return NULL;
    }
    Tree *DoBlock(Block *what)
    {
        if (child == what)
            return what;
        if (Tree *child = FindParent(what->child, "c"))
            return child;
        return NULL;
    }

    Tree *Do(Tree *what)
    {
        if (child == what)
            return what;
        return NULL;
    }

    Tree_p child;
    int    level;
    text   path;
};

XL_END

#endif // TREE_H
