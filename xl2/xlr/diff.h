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
#include <list>
#include <iostream>
#include <math.h>

#include <typeinfo> // Debug

// When this macro is set, we use plain string comparison to determine whether
// two text leaves should be matched (i.e., considered equal during the matching
// phase).
// When unset, leaves with strings that are just "similar enough" may be
// matched.
// FIXME: Longest Common Subsequence comparison is SLOW!
//#define NO_LCS_STRCMP 1


XL_BEGIN

// ============================================================================
//
//    The types being defined or used to manipulate tree diffs
//
// ============================================================================

struct TreeDiff;                        // The main class to do diff operations
struct NodePair;                        // Two node identifiers
typedef std::set<NodePair *> matching;  // A correspondence between tree nodes
struct EditScript;                      // A list of edit operations

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

struct InOrderInfo : Info
// ----------------------------------------------------------------------------
//   In order/out of order marker for the FindPos and AlignChildren algorithms
// ----------------------------------------------------------------------------
{
    typedef bool data_t;
    InOrderInfo(bool inorder = false): inorder(inorder) {}
    operator data_t() { return inorder; }
    data_t inorder;
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

struct ChildVectorInfo : Info
// ----------------------------------------------------------------------------
//   A pointer to a child vector
// ----------------------------------------------------------------------------
{
    typedef std::vector<Tree *> * data_t;
    ChildVectorInfo(data_t p): p(p) {}
    operator data_t() { return p; }
    data_t p;
};

struct PrintNode : Action
// ----------------------------------------------------------------------------
//   Display a node
// ----------------------------------------------------------------------------
{
    PrintNode(std::ostream &out, bool showInfos = false):
        out(out), showInfos(showInfos) {}

    Tree *DoInteger(Integer *what)
    {
        if (showInfos)
            ShowInfos(what);
        out << "[Integer] " << what->value;
        return NULL;
    }
    Tree *DoReal(Real *what)
    {
        if (showInfos)
            ShowInfos(what);
        out << "[Real] " << what->value;
        return NULL;
    }
    Tree *DoText(Text *what)
    {
        if (showInfos)
            ShowInfos(what);
        out << "[Text] " << what->value;
        return NULL;
    }
    Tree *DoName(Name *what)
    {
        if (showInfos)
            ShowInfos(what);
        out << "[Name] " << what->value;
        return NULL;
    }
    Tree *DoBlock(Block *what)
    {
        if (showInfos)
            ShowInfos(what);
        out << "[Block] " << what->opening
                         << " " << what->closing;
        return NULL;
    }
    Tree *DoInfix(Infix *what)
    {
        if (showInfos)
            ShowInfos(what);
        std::string name = (!what->name.compare("\n")) ?  "<CR>" : what->name;
        out << "[Infix] " << name;
        return NULL;
    }
    Tree *DoPrefix(Prefix *what)
    {
        if (showInfos)
            ShowInfos(what);
        out << "[Prefix] ";
        return NULL;
    }
    Tree *DoPostfix(Postfix *what)
    {
        if (showInfos)
            ShowInfos(what);
        out << "[Postfix] ";
        return NULL;
    }
    Tree *Do(Tree *what)
    {
        return NULL;
    }

protected:

    void ShowInfos(Tree *what)
    {
        if (what->Exists<NodeIdInfo>())
            out << "ID: " << what->Get<NodeIdInfo>() << " ";
        bool m = (what->Exists<MatchedInfo>() && what->Get<MatchedInfo>());
        out << (m ? "" : "un") << "matched" << " ";
        bool io = (what->Exists<InOrderInfo>() && what->Get<InOrderInfo>());
        out << (io ? "in" : "out-of") << "-order" << " ";
if (what->Exists<ChildVectorInfo>())
{
    out << "cv=";
    std::vector<Tree *> *cv = what->Get<ChildVectorInfo>();
    for (unsigned i = 0; i < cv->size(); i++)
        out << (void*)(*cv)[i] << " ";
}
    }

protected:

    std::ostream &out;
    bool          showInfos;
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
        if (what->child)
            what->child->Set2<ParentInfo>(what);
        return NULL;
    }
    Tree *DoInfix(Infix *what)
    {
        what->left->Set2<ParentInfo>(what);
        what->right->Set2<ParentInfo>(what);
        return NULL;
    }
    Tree *DoPrefix(Prefix *what)
    {
        what->left->Set2<ParentInfo>(what);
        what->right->Set2<ParentInfo>(what);
        return NULL;
    }
    Tree *DoPostfix(Postfix *what)
    {
        what->left->Set2<ParentInfo>(what);
        what->right->Set2<ParentInfo>(what);
        return NULL;
    }
    Tree *Do(Tree *what)
    {
        return NULL;
    }
};

struct SetChildVectorInfo : Action
// ----------------------------------------------------------------------------
//   Create (and fill) a child vector in the Info list of internal nodes
// ----------------------------------------------------------------------------
{
    SetChildVectorInfo() {}
    virtual ~SetChildVectorInfo() {}

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
        std::vector<Tree *> *v = new std::vector<Tree *>;
        if (what->child)
            v->push_back(what->child);
        what->Set2<ChildVectorInfo>(v);
        return NULL;
    }
    Tree *DoInfix(Infix *what)
    {
        std::vector<Tree *> *v = new std::vector<Tree *>;
        if (what->left)
            v->push_back(what->left);
        if (what->right)
            v->push_back(what->right);
        what->Set2<ChildVectorInfo>(v);
        return NULL;
    }
    Tree *DoPrefix(Prefix *what)
    {
        std::vector<Tree *> *v = new std::vector<Tree *>;
        if (what->left)
            v->push_back(what->left);
        if (what->right)
            v->push_back(what->right);
        what->Set2<ChildVectorInfo>(v);
        return NULL;
    }
    Tree *DoPostfix(Postfix *what)
    {
        std::vector<Tree *> *v = new std::vector<Tree *>;
        if (what->left)
            v->push_back(what->left);
        if (what->right)
            v->push_back(what->right);
        what->Set2<ChildVectorInfo>(v);
        return NULL;
    }
    Tree *Do(Tree *what)
    {
        return NULL;
    }
};

struct SyncWithChildVectorInfo : Action
// ----------------------------------------------------------------------------
//   Update the child pointers with values from ChildVectorInfo, and recurse
// ----------------------------------------------------------------------------
{
    SyncWithChildVectorInfo() {}
    virtual ~SyncWithChildVectorInfo() {}

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
        std::vector<Tree *> *v = what->Get<ChildVectorInfo>();
        what->child = (*v)[0];
        assert(v->size() == 1);
        what->child->Do(this);
        return NULL;
    }
    Tree *DoInfix(Infix *what)
    {
        std::vector<Tree *> *v = what->Get<ChildVectorInfo>();
        what->left = (*v)[0];
        what->right = (*v)[1];
        assert(v->size() == 2);
        what->left->Do(this);
        what->right->Do(this);
        return NULL;
    }
    Tree *DoPrefix(Prefix *what)
    {
        std::vector<Tree *> *v = what->Get<ChildVectorInfo>();
        what->left = (*v)[0];
        what->right = (*v)[1];
        assert(v->size() == 2);
        what->left->Do(this);
        what->right->Do(this);
        return NULL;
    }
    Tree *DoPostfix(Postfix *what)
    {
        std::vector<Tree *> *v = what->Get<ChildVectorInfo>();
        what->left = (*v)[0];
        what->right = (*v)[1];
        assert(v->size() == 2);
        what->left->Do(this);
        what->right->Do(this);
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
    TreeDiff (Tree *t1, Tree *t2);
    virtual ~TreeDiff();

    bool Diff(std::ostream& os);

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
        // Tree * operator ->() { return t; }
        const Node & operator = (Tree *t) { this->t = t; return *this; }
        Node & operator = (const Node &n) { t = n.t; return *this; }
        bool operator == (const Node &n);
        node_id Id() const
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
            t->Set2<MatchedInfo>(true);
        }
        unsigned int CountLeaves();
        bool ContainsLeaf(Tree *leaf) const;

    protected:

        Tree *t;

    public:
        matching *m;
    };

public: // FIXME public for operator <<

    struct node_table : public std::map<node_id, TreeDiff::Node>
    {
        node_table() {}
        virtual ~node_table() {}
        node_id NewId() { node_id n = next_id; next_id += step; return n; }
        void SetNextId(node_id next_id) { this->next_id = next_id; }
        void SetStep(node_id step) { this->step = step; }

        node_id next_id;
        node_id step;
    };

    typedef std::vector<TreeDiff::Node> node_vector;
    typedef std::set<NodePair *> matching;

protected:

    struct NodeForAlign : Node
    {
        NodeForAlign(matching &m): Node(), m(m) {}
        NodeForAlign(matching &m, Tree *t): Node(t), m(m) {}
        bool operator == (const NodeForAlign &n);
        void SetInOrder(bool b = true)
        {
            t->Set2<InOrderInfo>(b);
        }
        bool InOrder()
        {
            if (!t->Exists<InOrderInfo>())
                return false;
            return t->Get<InOrderInfo>();
        }

        // FIXME: call Node::operator= (?)
        NodeForAlign &operator =(const NodeForAlign &n) { m=n.m; t=n.t; return *this; }
        matching &m;
    };
    typedef std::vector<TreeDiff::NodeForAlign> node_vector_align;


    struct SetNodeIdAction : SimpleAction
    // ------------------------------------------------------------------------
    //   Set an integer node ID to each node. Append node to a table.
    // ------------------------------------------------------------------------
    {
        SetNodeIdAction(node_table &tab, node_id from_id = 1, node_id step = 1)
          : tab(tab), id(from_id), step(step) {}
        virtual Tree *Do(Tree *what)
        {
            what->Set2<NodeIdInfo>(id);
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

    template <typename I>
    struct AddPtr : SimpleAction
    // ------------------------------------------------------------------------
    //   Append node pointer to a container using push_back.
    // ------------------------------------------------------------------------
    {
        AddPtr(I &container): container(container) {}
        virtual Tree *Do(Tree *what)
        {
            container.push_back(what);
            return NULL;
        }
        I &container;
    };

    struct PurgeDiffInfos : SimpleAction
    // ------------------------------------------------------------------------
    //   Purge all Info pointers that may have been added to T2 by TreeDiff
    // ------------------------------------------------------------------------
    {
        PurgeDiffInfos() {}
        virtual Tree *Do(Tree *what)
        {
            what->Purge<NodeIdInfo>();
            what->Purge<MatchedInfo>();
            what->Purge<TreeDiffInfo>();
            what->Purge<LeafCountInfo>();
            what->Purge<ParentInfo>();
            what->Purge<ChildVectorInfo>();
            return NULL;
        }
    };

protected:

    bool     Diff();
    void     DoFastMatch();
    void     DoEditScript();
    unsigned FindPos(node_id x);
    void     AlignChildren(node_id w, node_id x);

    node_id AssignNodeIds(Tree *t, node_table &m, node_id from_id = 1,
                          node_id step = 1);
    void SetParentPointers(Tree *t);
    void BuildChains(Tree *allnodes, node_vector *out);
    void MatchOneKind(matching &M, node_vector &S1, node_vector &S2);
    void CreateChildVectors(Tree *t);
    void UpdateChildren(Tree *t);
    bool FindPair(node_id a, node_id b,
                  node_vector_align s1, node_vector_align s2);

    static node_id PartnerInFirstTree(node_id id, const matching &m);  // FIXME
    static node_id PartnerInSecondTree(node_id id, const matching &m); // FIXME
    static bool LeafEqual(Tree *t1, Tree *t2);
    static bool NonLeafEqual(Tree *t1, Tree *t2, TreeDiff &m);
    static float Similarity(std::string s1, std::string s2);

protected:

    Tree *t1, *t2;
    node_table nodes1, nodes2;  // To access nodes by ID
    node_id t1_lastnodeid, t2_lastnodeid;
    matching m;
    EditScript *escript;
};

std::ostream&
operator <<(std::ostream &out, XL::TreeDiff::node_table &m);

std::ostream&
operator <<(std::ostream &out, XL::TreeDiff::node_vector &m);

std::ostream&
operator <<(std::ostream &out, XL::TreeDiff::matching &m);


// ============================================================================
//
//    The EditOperation and EditScript classes
//
// ============================================================================


struct EditOperation
// ----------------------------------------------------------------------------
//   An operation on a tree. Edit scripts generated by TreeDiff are made of EO.
// ----------------------------------------------------------------------------
{
    EditOperation() {}
    virtual ~EditOperation() {}

    struct Base;
    struct Insert;
    struct Delete;
    struct Update;
    struct Move;
};

struct EditOperation::Base
// ----------------------------------------------------------------------------
//   Unspecified edit operation.
// ----------------------------------------------------------------------------
{
    Base() {}
    virtual ~Base() {}

    virtual void Apply(TreeDiff::node_table &table) = 0;
};

struct EditOperation::Insert : EditOperation::Base
// ----------------------------------------------------------------------------
//   The operation of inserting a new leaf node into a tree.
// ----------------------------------------------------------------------------
{
    Insert(Tree *leaf, node_id parent, unsigned pos):
        leaf(leaf), parent(parent), pos(pos) { assert(leaf); }
    virtual ~Insert() {}

    virtual void Apply(TreeDiff::node_table &table);

    Tree *   leaf;
    node_id  parent;
    unsigned pos;
};

struct EditOperation::Delete : EditOperation::Base
// ----------------------------------------------------------------------------
//   The operation of deleting a leaf node of a tree.
// ----------------------------------------------------------------------------
{
    Delete(node_id leaf): leaf(leaf) {}
    virtual ~Delete() {}

    virtual void Apply(TreeDiff::node_table &table);

    node_id leaf;
};

struct EditOperation::Update : EditOperation::Base
// ----------------------------------------------------------------------------
//   The operation of inserting a new leaf node into a tree.
// ----------------------------------------------------------------------------
{
    Update(node_id leaf, Tree *value): leaf(leaf), value(value)
        { assert(value); }
    virtual ~Update() {}

    virtual void Apply(TreeDiff::node_table &table);

    node_id leaf;
    Tree *  value;
};

struct EditOperation::Move : EditOperation::Base
// ----------------------------------------------------------------------------
//   The operation of moving a subtree from one parent to another.
// ----------------------------------------------------------------------------
{
    Move(node_id subtree, node_id parent, unsigned pos):
        subtree(subtree), parent(parent), pos(pos) {}
    virtual ~Move() {}

    virtual void Apply(TreeDiff::node_table &table);

    node_id subtree;
    node_id parent;
    unsigned pos;
};

typedef std::list<EditOperation::Base *> EditScriptBase;

struct EditScript : EditScriptBase
{
    EditScript() {}
    virtual ~EditScript() {}

    void Apply(TreeDiff::node_table &table);
};


std::ostream&
operator <<(std::ostream &out, XL::EditOperation::Insert &op);

std::ostream&
operator <<(std::ostream &out, XL::EditOperation::Delete &op);

std::ostream&
operator <<(std::ostream &out, XL::EditOperation::Update &op);

std::ostream&
operator <<(std::ostream &out, XL::EditOperation::Move &op);

std::ostream&
operator <<(std::ostream &out, XL::EditScript &s);

XL_END

#endif // DIFF_H
