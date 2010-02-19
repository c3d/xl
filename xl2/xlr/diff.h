#ifndef DIFF_H
#define DIFF_H
// ****************************************************************************
//  diff.h                                                          XLR project
// ****************************************************************************
//
//   File Description:
//
//     Definitions for tree comparison and transformation algorithms.
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
//  (C) 2010 Jerome Forissier <jerome@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "base.h"
#include "tree.h"
#include "renderer.h"
#include <set>
#include <map>
#include <iostream>
#include <math.h>

// Do not use LCS to compare text leaves and obtain a similarity score
// Use plain string comparison instead (much faster)
#define NO_LCS_STRCMP 1

XL_BEGIN

// ============================================================================
//
//    The types being defined or used to manipulate tree diffs
//
// ============================================================================

struct TreeDiff;                        // The main class to do diff operations
struct NodePair;                        // Two node identifiers
typedef longlong node_id;              // A node identifier
typedef std::set<NodePair *> matching;  // A correspondence between tree nodes


// ============================================================================
//
//    The TreeDiff class
//
// ============================================================================


struct NodePair
// ----------------------------------------------------------------------------
//   A pair of node identifiers
// ----------------------------------------------------------------------------
{
    NodePair(node_id x, node_id y): x(x), y(y) {}
    virtual ~NodePair() {}

    node_id x, y;
};

struct NodeIdInfo : Info
// ----------------------------------------------------------------------------
//   Node identifier information
// ----------------------------------------------------------------------------
{
    typedef node_id data_t;
    NodeIdInfo(node_id id): id(id) {}
    operator data_t() { return id; }
    data_t id;
};

struct MatchedInfo : Info
// ----------------------------------------------------------------------------
//   Has this node been matched with another node (during diff operation)
// ----------------------------------------------------------------------------
{
    typedef bool data_t;
    MatchedInfo(bool matched = false): matched(matched) {}
    operator data_t() { return matched; }
    data_t matched;
};

// FIXME
struct TreeDiffInfo : Info
{
    typedef TreeDiff * data_t;
    TreeDiffInfo(TreeDiff *td): td(td) {}
    operator data_t() { return td; }
    data_t td;
};

struct LeafCountInfo : Info
// ----------------------------------------------------------------------------
//   The number of leaves that can be reached under this node
// ----------------------------------------------------------------------------
{
    typedef unsigned int data_t;
    LeafCountInfo(data_t n): n(n) {}
    operator data_t() { return n; }
    data_t n;
};

// FIXME: should be in Tree
struct ParentInfo : Info
// ----------------------------------------------------------------------------
//   A pointer to the parent of a node
// ----------------------------------------------------------------------------
{
    typedef Tree * data_t;
    ParentInfo(data_t p): p(p) {}
    operator data_t() { return p; }
    data_t p;
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
    virtual Tree * Do(Tree *what) = 0;
};

struct PrintNode : Action
// ----------------------------------------------------------------------------
//   Display a node
// ----------------------------------------------------------------------------
{
    PrintNode(std::ostream &out): out(out) {}

    Tree *DoInteger(Integer *what)
    {
        DisplayInfos(what);
        out << "[Integer] " << what->value << std::endl;
        return NULL;
    }
    Tree *DoReal(Real *what)
    {
        DisplayInfos(what);
        out << "[Real] " << what->value << std::endl;
        return NULL;
    }
    Tree *DoText(Text *what)
    {
        DisplayInfos(what);
        out << "[Text] " << what->value << std::endl;
        return NULL;
    }
    Tree *DoName(Name *what)
    {
        DisplayInfos(what);
        out << "[Name] " << what->value << std::endl;
        return NULL;
    }
    Tree *DoBlock(Block *what)
    {
        DisplayInfos(what);
        out << "[Block] " << what->opening
                         << " " << what->closing << std::endl;
        return NULL;
    }
    Tree *DoInfix(Infix *what)
    {
        DisplayInfos(what);
        std::string name = (!what->name.compare("\n")) ?  "<CR>" : what->name;
        out << "[Infix] " << name << std::endl;
        return NULL;
    }
    Tree *DoPrefix(Prefix *what)
    {
        DisplayInfos(what);
        out << "[Prefix] " << std::endl;
        return NULL;
    }
    Tree *DoPostfix(Postfix *what)
    {
        DisplayInfos(what);
        out << "[Postfix] " << std::endl;
        return NULL;
    }
    Tree *Do(Tree *what)
    {
        return NULL;
    }

protected:

    void DisplayInfos(Tree *what)
    {
        if (what->Exists<NodeIdInfo>())
            out << "ID: " << what->Get<NodeIdInfo>() << " ";
        bool m = (what->Exists<MatchedInfo>() && what->Get<MatchedInfo>());
        out << (m ? "" : "un") << "matched" << " ";
    }

protected:

    std::ostream &out;
};

struct SetParentInfo : Action
// ----------------------------------------------------------------------------
//   Update the ParentInfo value of the child(s) of a node
// ----------------------------------------------------------------------------
{
    SetParentInfo() {}
    virtual ~SetParentInfo() {}

    Tree *DoInteger(Integer *what)
    {
        return NULL;
    }
    Tree *DoReal(Real *what)
    {
        return NULL;
    }
    Tree *DoText(Text *what)
    {
        return NULL;
    }
    Tree *DoName(Name *what)
    {
        return NULL;
    }
    Tree *DoBlock(Block *what)
    {
        what->child->Set<ParentInfo>(what);
        return NULL;
    }
    Tree *DoInfix(Infix *what)
    {
        what->left->Set<ParentInfo>(what);
        what->right->Set<ParentInfo>(what);
        return NULL;
    }
    Tree *DoPrefix(Prefix *what)
    {
        what->left->Set<ParentInfo>(what);
        what->right->Set<ParentInfo>(what);
        return NULL;
    }
    Tree *DoPostfix(Postfix *what)
    {
        what->left->Set<ParentInfo>(what);
        what->right->Set<ParentInfo>(what);
        return NULL;
    }
    Tree *Do(Tree *what)
    {
        return NULL;
    }
};

struct TreeDiff
// ----------------------------------------------------------------------------
//   All you need to compare and patch parse trees
// ----------------------------------------------------------------------------
{
    TreeDiff (Tree *t1, Tree *t2): t1(t1), t2(t2) {}
    virtual ~TreeDiff();

    void Diff();

protected:

    struct Node
    // ------------------------------------------------------------------------
    //   Holder class for Tree *, that defines a custom operator ==.
    // ------------------------------------------------------------------------
    {
        Node(): t(NULL), m(NULL) {}
        Node(Tree *t): t(t), m(NULL) {}
        virtual ~Node() {}

        Tree * GetTree() const { return t; }
        Tree * operator ->() { return t; }
        const Node & operator = (Tree *t) { this->t = t; return *this; }
        bool operator == (const Node &n);
        node_id Id()
        {
            if (t && t->Exists<NodeIdInfo>())
                return t->Get<NodeIdInfo>();
            return 0;
        }
        bool IsMatched()
        {
            if (!t || !t->Exists<MatchedInfo>())
                return false;
            return (t->Get<MatchedInfo>());
        }
        void SetMatched(bool matched = true)
        {
            if (!t)
                return;
            t->Set<MatchedInfo>(true);
        }
        unsigned int CountLeaves();
        bool ContainsLeaf(Tree *leaf) const;

    protected:

        Tree *t;

    public:
        matching *m;
    };

public: // FIXME public for operator <<

    typedef std::map<node_id, TreeDiff::Node> node_table;
    typedef std::vector<TreeDiff::Node> node_vector;
    typedef std::set<NodePair *> matching;

protected:

    struct SetNodeIdAction : SimpleAction
    // ------------------------------------------------------------------------
    //   Set an integer node ID to each node. Append node to a table.
    // ------------------------------------------------------------------------
    {
        SetNodeIdAction(node_table &tab, node_id from_id = 1, node_id step = 1)
          : tab(tab), id(from_id), step(step) {}
        virtual Tree *Do(Tree *what)
        {
            what->Set<NodeIdInfo>(id);
            tab[id] = what;
            id += step;
            return NULL;
        }
        node_table &tab;
        node_id id;
        node_id step;
    };

    struct StoreNodeIntoChainArray : SimpleAction
    // ------------------------------------------------------------------------
    //   Append node to a node array based on node kind.
    // ------------------------------------------------------------------------
    {
        StoreNodeIntoChainArray(node_vector *chains): chains(chains) {}
        virtual Tree *Do(Tree *what)
        {
            chains[what->Kind()].push_back(what);
            return NULL;
        }
        node_vector *chains;
    };

    struct CountLeaves : SimpleAction
    // ------------------------------------------------------------------------
    //   Increment a counter if node is a leaf
    // ------------------------------------------------------------------------
    {
        CountLeaves(): nb_leaves(0) {}
        virtual Tree *Do(Tree *what)
        {
            if (what->IsLeaf())
                nb_leaves++;
            return NULL;
        }
        unsigned int nb_leaves;
    };

protected:

    void DoFastMatch();

    node_id AssignNodeIds(Tree *t, node_table &m, node_id from_id = 1,
                          node_id step = 1);
    void SetParentPointers(Tree *t);
    void BuildChains(Tree *allnodes, node_vector *out);
    void MatchOneKind(matching &M, node_vector &S1, node_vector &S2);

    static bool LeafEqual(Tree *t1, Tree *t2);
    static bool NonLeafEqual(Tree *t1, Tree *t2, TreeDiff &m);
    static float Similarity(std::string s1, std::string s2);

protected:

    Tree *t1, *t2;
    node_table nodes1, nodes2;  // To access nodes by ID
    matching m;
};

XL_END

std::ostream&
operator <<(std::ostream &out, XL::TreeDiff::node_table &m);

std::ostream&
operator <<(std::ostream &out, XL::TreeDiff::node_vector &m);

std::ostream&
operator <<(std::ostream &out, XL::TreeDiff::matching &m);

#endif // DIFF_H
