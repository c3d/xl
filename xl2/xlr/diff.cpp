// ****************************************************************************
//  diff.cpp                                                        XLR project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of the tree diffing and patching algorithms.
//     Based on paper: [CDHSI]
//         "Change Detection in Hierarchically Structured Information"
//         (Stanford University, 1996)
//     http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.48.9224
//     &rep=rep1&type=pdf
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
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "diff.h"
#include "bfs.h"
#include "inorder.h"
#include "postorder.h"
#include "lcs.h"
#include "options.h"
#include "hash.h"
#include "renderer.h"
#include <iostream>
#include <cassert>
#include <map>
#include <sstream>
#include <iterator>

// When this macro is set, we use plain string comparison to determine whether
// two text leaves should be paired (i.e., considered equal during the leaf
// matching phase). This is the fastest mode.
// When not set, leaves with strings that are just "similar enough" may be
// paired together.
//#define EXACT_STRING_MATCH 1

XL_BEGIN

#include "sha1_ostream.h" // Won't work outside of XL namespace

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

// FIXME: should be in Tree?
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

struct CommonLeavesInfo : Info
// ----------------------------------------------------------------------------
//   A pointer to map that stores a leaf count
// ----------------------------------------------------------------------------
{
    // For a given node in tree T1, the following map is indexed by nodes in T2
    // The value is the number of leaves that the two nodes have in common
    // with respect to the current leaf matching in the TreeDiff class
    typedef std::map<node_id, unsigned int> map;
    typedef map * data_t;
    CommonLeavesInfo(data_t p): p(p) {}
    operator data_t() { return p; }
    data_t p;
};

struct Words
// ----------------------------------------------------------------------------
//   Representation of a text string as a vector of words
// ----------------------------------------------------------------------------
{
    Words(std::string &s)
    {
        std::istringstream iss(s);
        std::copy(std::istream_iterator<std::string>(iss),
                  std::istream_iterator<std::string>(),
                  std::back_inserter< std::vector<std::string> >(words));
    }
    std::string& operator[] (size_t pos) { return words[pos]; }
    size_t size() { return words.size(); }

    std::vector<std::string> words;
};

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
    unsigned int LeafCount()
    {
        if (!t)
            return 0;
        return t->Get<LeafCountInfo>();
    }
    Tree * Parent()
    {
        if (!t || !t->Exists<ParentInfo>())
            return NULL;
        return (t->Get<ParentInfo>());
    }

protected:

    Tree *     t;
    Matching * m;
};

struct NodeTable : public std::map<node_id, Node>
//
// FIXME comment
//
{
    NodeTable() {}
    virtual ~NodeTable() {}
    node_id NewId() { node_id n = next_id; next_id += step; return n; }
    void SetNextId(node_id next_id) { this->next_id = next_id; }
    void SetStep(node_id step) { this->step = step; }

    node_id next_id;
    node_id step;
};

// FIXME typedef std::vector<Node> node_vector;
struct node_vector : std::vector<Node> {};

struct Matching
// ------------------------------------------------------------------------
//   Map some nodes back and forth between two trees
// ------------------------------------------------------------------------
{
    typedef std::map<node_id, node_id>::iterator  iterator;
    typedef std::map<node_id, node_id>::size_type size_type;

    void insert(node_id x, node_id y) { to[x] = y; fro[y] = x; }
    iterator begin() { return to.begin(); }
    iterator end()   { return to.end(); }
    size_type size() { return to.size(); }

    std::map<node_id, node_id> to;
    std::map<node_id, node_id> fro;
};

struct NodeForAlign : Node
//
// FIXME comment
//
{
    NodeForAlign(Matching &m): Node(), m(m) {}
    NodeForAlign(Matching &m, Tree *t): Node(t), m(m) {}
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
    Matching &m;
};

// FIXME typedef std::vector<NodeForAlign> node_vector_align;
struct node_vector_align : std::vector<NodeForAlign> {};

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
    }

protected:

    std::ostream &out;
    bool          showInfos;
};

std::ostream&
operator <<(std::ostream &out, node_vector &m)
// ----------------------------------------------------------------------------
//    Display a vector of nodes
// ----------------------------------------------------------------------------
{
    XL::PrintNode pn(out, true);
    for (unsigned i = 0; i < m.size(); i++)
        m[i].GetTree()->Do(pn);

    return out;
}

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

struct CountLeaves : Action
// ------------------------------------------------------------------------
//   Recursively count and store the number of leaves under a node
// ------------------------------------------------------------------------
{
    Tree *DoInteger(Integer *what)
    {
        what->Set2<LeafCountInfo>(1);
        return NULL;
    }
    Tree *DoReal(Real *what)
    {
        what->Set2<LeafCountInfo>(1);
        return NULL;
    }
    Tree *DoText(Text *what)
    {
        what->Set2<LeafCountInfo>(1);
        return NULL;
    }
    Tree *DoName(Name *what)
    {
        what->Set2<LeafCountInfo>(1);
        return NULL;
    }
    Tree *DoBlock(Block *what)
    {
        unsigned cc = 0;
        if (what->child)
        {
            what->child->Do(this);
            cc = what->child->Get<LeafCountInfo>();
        }
        what->Set2<LeafCountInfo>(cc);
        return NULL;
    }
    Tree *DoInfix(Infix *what)
    {
        what->left->Do(this);
        what->right->Do(this);
        unsigned lc = what->left->Get<LeafCountInfo>();
        unsigned rc = what->left->Get<LeafCountInfo>();
        what->Set2<LeafCountInfo>(lc + rc);
        return NULL;
    }
    Tree *DoPrefix(Prefix *what)
    {
        what->left->Do(this);
        what->right->Do(this);
        unsigned lc = what->left->Get<LeafCountInfo>();
        unsigned rc = what->left->Get<LeafCountInfo>();
        what->Set2<LeafCountInfo>(lc + rc);
        return NULL;
    }
    Tree *DoPostfix(Postfix *what)
    {
        what->left->Do(this);
        what->right->Do(this);
        unsigned lc = what->left->Get<LeafCountInfo>();
        unsigned rc = what->left->Get<LeafCountInfo>();
        what->Set2<LeafCountInfo>(lc + rc);
        return NULL;
    }
    Tree *Do(Tree *what)
    {
        return NULL;
    }
};

struct AssignNodeIds : SimpleAction
// ------------------------------------------------------------------------
//   Set an integer node ID to each node. Append node to a table.
// ------------------------------------------------------------------------
{
    AssignNodeIds(NodeTable &tab, node_id from_id = 1, node_id step = 1)
      : tab(tab), id(from_id), step(step) {}
    virtual Tree *Do(Tree *what)
    {
        what->Set2<NodeIdInfo>(id);
        tab[id] = what;
        id += step;
        return NULL;
    }
    NodeTable &tab;
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
    void SetChains(node_vector *c) { chains = c; }
    node_vector *chains;
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
        what->Purge<CommonLeavesInfo>();
        return NULL;
    }
};

TreeDiff::TreeDiff(Tree *t1, Tree *t2) :
  t1(NULL), t2(t2), nodes1(*new NodeTable), nodes2(*new NodeTable),
  matching(*new Matching), escript(NULL)
{
    // Make a deep copy because the diff algorithm needs to modify the tree
    if (t1)
    {
        TreeClone clone;
        this->t1 = t1->Do(clone);
    }
}

TreeDiff::~TreeDiff()
{
    // t1 is a clone of the first tree passed to CTOR -> delete it
    delete t1;

    // t2 is the second tree passed to CTOR -> clean it
    PurgeDiffInfos purge;
    InOrderTraversal iot(purge);
    t2->Do(iot);

    delete &nodes1;
    delete &nodes2;
    delete &matching;
    if (escript)
        delete escript;
}

static void MatchOneKind(Matching &M, node_vector &S1, node_vector &S2)
// ----------------------------------------------------------------------------
//    Find a matching between two series of nodes of the same kind
// ----------------------------------------------------------------------------
{
    if (S1.empty())
        return;

    IFTRACE(diff)
        std::cout << "  Matching (kind = " << S1.front().GetTree()->Kind() << ")\n";

    // Compute the Longest Common Subsequence in the node chains of
    // both trees...
    IFTRACE(diff)
        std::cout << "   Running LCS..." << std::flush;
    node_vector lcs1, lcs2;
    LCS<node_vector> lcs_algo;
    lcs_algo.Compute(S1, S2);
    lcs_algo.Extract2(S1, lcs1, S2, lcs2);
    IFTRACE(diff)
        std::cout << " done, " << lcs1.size() << " node(s)\n";

    // ...add node pairs to matching...
    for (unsigned int i = 0; i < lcs1.size(); i++)
    {
        Node &x = lcs1[i], &y = lcs2[i];
        if (!x.IsMatched())
        {
            M.insert(x.Id(), y.Id());
            x.SetMatched();
            y.SetMatched();
        }
    }

    // Nodes still unmatched after the call to LCS are processed using
    // linear search
    IFTRACE(diff)
        std::cout << "   Running linear matching..." << std::flush;
    unsigned count = 0;
    node_vector::iterator x, y;
    for (x = S1.begin(); x != S1.end(); x++)
    {
        // ...for each unmatched node in S1...
        if ((*x).IsMatched())
            continue;
        for (y = S2.begin(); y != S2.end(); y++)
        {
            // ...if there is an unmatched node y in S2...
            if ((*y).IsMatched())
                continue;
            // ...such that equal(x,y)...
            if ((*x) == (*y))
            {
                // A. Add (x,y) to M
                M.insert((*x).Id(), (*y).Id());
                // B. Mark x and y "matched".
                (*x).SetMatched();
                (*y).SetMatched();
                count++;
                break;
            }
        }
    }
    IFTRACE(diff)
        std::cout << " done, " << count << " node(s)\n";
}

void TreeDiff::FastMatch()
// ----------------------------------------------------------------------------
//    Compute a "good" matching between trees t1 and t2
// ----------------------------------------------------------------------------
{
    node_vector chains1[KIND_LAST + 1];  // All nodes of T1, per kind
    node_vector chains2[KIND_LAST + 1];  // All nodes of T2, per kind

    IFTRACE(diff)
       std::cout << "Entering FastMatch\n";

    // For each tree, sort the nodes into node chains (one for each node kind).
    StoreNodeIntoChainArray action(chains1);
    InOrderTraversal iot(action);
    t1->Do(iot);
    action.SetChains(chains2);
    t2->Do(iot);

    IFTRACE(diff)
        std::cout << " Matching leaves\n";
    for (int k = KIND_LEAF_FIRST; k <= KIND_LEAF_LAST; k++)
    {
        node_vector &S1 = chains1[k], &S2 = chains2[k];
        MatchOneKind(matching, S1, S2);
    }

    // In order to match internal nodes, we need to count common leaves (given
    // the leaf matching)
    IFTRACE(diff)
        std::cout << " Counting common leaves..." << std::flush;
    Matching::iterator mit;
    for (mit = matching.begin(); mit != matching.end(); mit++)
    {
        node_id a = (*mit).first, b = (*mit).second;
        for (Tree *x = nodes1[a].Parent(); x; x = x->Get<ParentInfo>())
        {
            std::map<node_id, unsigned int> * c;
            CommonLeavesInfo *i = x->GetInfo<CommonLeavesInfo>();
            if (!i)
            {
                c = new std::map<node_id, unsigned int>;
                x->Set2<CommonLeavesInfo>(c);
                (*c)[0] = 0;
            }
            else
              c = (CommonLeavesInfo::data_t) *i;

            for (Tree *y = nodes2[b].Parent(); y; y = y->Get<ParentInfo>())
                (*c)[y->Get<NodeIdInfo>()]++;
        }
    }
    IFTRACE(diff)
        std::cout << " done\n";

    // FIXME:
    // TreeDiff object (this) must be available to each Node of first tree
    // because it is used by Node::operator== for internal nodes.
    NodeTable::iterator it;
    for (it = nodes1.begin(); it != nodes1.end(); it++)
        (*it).second.GetTree()->Set2<TreeDiffInfo>(this);

    IFTRACE(diff)
        std::cout << " Matching internal nodes\n";
    for (int k = KIND_NLEAF_FIRST; k <= KIND_NLEAF_LAST; k++)
    {
        node_vector &S1 = chains1[k], &S2 = chains2[k];
        MatchOneKind(matching, S1, S2);
    }

    IFTRACE(diff)
        std::cout << "Matching done. " << matching.size()
                  << "/" << nodes1.size() << " nodes ("
                  << (float)matching.size()*100/nodes1.size() << "%) matched.\n";
}

static float Similarity(std::string s1, std::string s2)
// ----------------------------------------------------------------------------
//    Return a similarity score bewteen 0 and 1 (1 means strings are equal).
// ----------------------------------------------------------------------------
{
#ifdef EXACT_STRING_MATCH
    return ((float)!s1.compare(s2));
#else
    // The score is the length of the longest common subsequence (LCS) of the
    // two strings, divided by the length of the longest string.
    // This yields a score of 0 if the strings have nothing in common, and 1
    // if they are identical.
    // Longest strings are compared word by word, shortest ones char by char.

    float ret;
    size_t len1 = s1.length(), len2 = s2.length();
    if (len1 + len2 > 200)
    {
        Words S1(s1), S2(s2), *small = &S1, *big   = &S2;
        if (len1 > len2)
        {
            small = &S2;
            big = &S1;
        }

        if (big->size() == 0)
            return 1.0;

        LCS<Words> lcs;
        lcs.Compute(*small, *big);
        ret = (float)lcs.Length() / big->size();
    }
    else
    {
        std::string *small = &s1, *big = &s2;
        if (len1 > len2)
        {
            small = &s2;
            big = &s1;
        }

        if (big->size() == 0)
            return 1.0;

        LCS<std::string> lcs;
        lcs.Compute(*small, *big);
        ret = (float)lcs.Length() / big->size();
    }
    return ret;
#endif
}

// FIXME rewrite as an Action?
static bool LeafEqual(Tree *t1, Tree *t2)
// ----------------------------------------------------------------------------
//    Test if two leaves should be considered equal (matching phase)
// ----------------------------------------------------------------------------
{
    assert(t1 && t1->IsLeaf());
    assert(t2 && t2->IsLeaf());

    // [CDHSI]
    // Two leaves x and y are considered equal iff:
    //  * they have the same kind (label), i.e., l(x) = l(y)
    //  * their values are "similar", i.e., compare(v(x), v(y)) <= f
    //    where f is a parameter valued between 0 and 1
    //
    // This implementation requires strict equality for integer, reals, symbols
    // and a given similarity score for text

    if (t1->Kind() != t2->Kind())
        return false;

    switch (t1->Kind())
    {
    case INTEGER:
        if (t1->AsInteger()->value != t2->AsInteger()->value)
            return false;
        break;
    case REAL:
        if (t1->AsReal()->value != t2->AsReal()->value)
            return false;
        break;
    case NAME:
        if (t1->AsName()->value.compare(t2->AsName()->value) != 0)
            return false;
        break;
    case TEXT:
        if (Similarity(t1->AsText()->value, t2->AsText()->value) < 0.6)
            return false;
        break;
    default:
        assert(!"Unexpected leaf kind");
    }

    return true;
}

// FIXME rewrite as an Action?
static bool NonLeafEqual(Tree *t1, Tree *t2, TreeDiff &td)
// ----------------------------------------------------------------------------
//    Test if two internal nodes should be considered equal (matching phase)
// ----------------------------------------------------------------------------
{
    assert(t1 && !t1->IsLeaf());
    assert(t2 && !t2->IsLeaf());

    // [CDHSI]
    // For internal nodes, equal(x, y) is true iff:
    //  * they have the same kind (label), i.e., l(x) = l(y)
    //  * they have at least a given percentage of leaves in common, i.e.,
    //      |common(x, y)|/max(|x|,|y|) > t ; 0.5 <= t <= 1
    //    where:
    //      o |x| denotes the number of leaf nodes x contains
    //      o node x contains a node y if y is a leaf node descendant of x
    //      o common(x, y) = {(w, z) in M | x contains w and y contains z}
    //      o (M is a matching obtained after the "match leaves" pass)

    if (t1->Kind() != t2->Kind())
        return false;

    // CHECK THIS
    // We consider that two infix nodes cannot be considered equal if they do not
    // bear the same value
    if (t1->Kind() == INFIX)
        if (((Infix *)t1)->name.compare(((Infix *)t2)->name))
            return false;
    // Similarly, two blocks cannot be equal if they don't use the same delimiters
    if (t1->Kind() == BLOCK)
    {
        Block *b1 = (Block *)t1, *b2 = (Block *)t2;
        if (b1->opening.compare(b2->opening))
            return false;
        if (b1->closing.compare(b2->closing))
            return false;
    }

    unsigned c1, c2, max, common = 0;
    Node n1(t1), n2(t2);

    c1 = n1.LeafCount();
    c2 = n2.LeafCount();
    max = (c2 > c1) ? c2 : c1;
    CommonLeavesInfo::map * cmn = t1->Get<CommonLeavesInfo>();
    if (cmn)
    {
        CommonLeavesInfo::map::iterator it = cmn->find(n2.Id());
        if (it != cmn->end())
            common = (*it).second;
    }
    float percent = (float)common / max;

    return (percent > 0.5);
}

void TreeDiff::DoEditScript()
// ----------------------------------------------------------------------------
//    Compute an Edit Script to change t1 into t2, given a matching m
// ----------------------------------------------------------------------------
{
    IFTRACE(diff)
       std::cout << "Entering DoEditScript\n";

    // Prepare T1 and T2: create a child vector in each node so that edit
    // operations may be applied
    SetChildVectorInfo action;
    InOrderTraversal iot(action);
    t1->Do(iot);
    t2->Do(iot);

    // 1.
    escript = new EditScript;
    EditScript & E = *escript;
    Matching &   Mprime = matching;

    // 2. Visit the nodes of T2 in breadth-first order
    NodeTable::reverse_iterator rit;
    for (rit = nodes2.rbegin(); rit != nodes2.rend(); rit++)
    {
        // (a) Let x be the current node in the breadth-first search of T2
        node_id x = (*rit).second.Id();
        Tree * xptr = nodes2[x].GetTree();
        assert(xptr);
        Tree * px_ptr = xptr->Get<ParentInfo>();
        node_id y = 0;
        if (px_ptr)
            y = px_ptr->Get<NodeIdInfo>();
        node_id w = Mprime.fro[x];
        node_id z = Mprime.fro[y];
        if (!nodes2[x].IsMatched())
        {
            // (b) x has no partner in M'
            //  i. k <- FindPos(x);
            unsigned k = FindPos(x);

            TreeCloneTemplate<NODE_ONLY> clone;
            Tree *t = xptr->Do(clone);
            EditOperation::Insert *ins;
            ins = new EditOperation::Insert(t, z, k);

            // ii. Append INS to E
            E.push_back(ins);

            // iii. Apply INS to T1
            ins->Apply(nodes1);

            // iii. Add (w, x) to M'
            w = t->Get<NodeIdInfo>();
            Mprime.insert(w, x);
            nodes1[w].SetMatched();
            nodes2[x].SetMatched();

            // FIXME - Check this (not in [CDHSI])
            t->Set<InOrderInfo>(true);
            xptr->Set<InOrderInfo>(true);
        }
        else
        {
            // x has a partner in M'
            if (px_ptr)
            {
                // (c) x is not the root
				//  i. Let w be the partner of x in M' and let v = p(w)
				//     in T1 [p(x) is the parent of x]
                Tree * wptr = nodes1[w].GetTree();
                assert(wptr);
                Tree * vptr = wptr->Get<ParentInfo>();
                assert(vptr);
                node_id v = vptr->Get<NodeIdInfo>();

                TreeMatchTemplate<TM_NODE_ONLY> compareNodes(wptr);
                if (!xptr->Do(compareNodes))
                {
                    // ii. If v(w) != v(x)
                    
                    TreeCloneTemplate<NODE_ONLY> clone;
                    Tree *t = xptr->Do(clone);
                    EditOperation::Update *upd;
                    upd = new EditOperation::Update(w, t);

                    // A. Append UPD(w, v(x)) to E
                    E.push_back(upd);
                    
                    // B. Apply UPD(w, v(x)) to T1
                    upd->Apply(nodes1);
                }
                if (y != Mprime.to[v])
                {
                    // iii. If (y, v) not in M'
                    
                    // A. Let z be the partner of y in M'
                    node_id z = Mprime.fro[y];
                    
                    // B. k <- FindPos(x)
                    unsigned k = FindPos(x);
                    
                    EditOperation::Move *mov;
                    mov = new EditOperation::Move(w, z, k);

                    // C. Append MOV(w, z, k) to E
                    E.push_back(mov);

                    // D. Apply MOV(w, z, k) to T1
                    mov->Apply(nodes1);

                    // FIXME - Check this (not in [CDHSI])
                    xptr->Set<InOrderInfo>(true);
                    wptr->Set<InOrderInfo>(true);
                }
            }
        }
        
        // (d) AlignChildren(w, x)
        AlignChildren(w, x);
    }
    
    // 3. Do a post-order traversal of T1
    std::list<Tree *> list;
    AddPtr< std::list<Tree *> > act(list);
    PostOrderTraversal pot(act);
    t1->Do(pot);
    std::list<Tree *>::iterator it;
    for (it = list.begin(); it != list.end(); it++)
    {
        // (a) Let w be the current node in the post-order traversal of T1.
        Node wn(*it);
        node_id w = wn.Id();
        if (!wn.IsMatched())
        {
            // (b) If w has no partner in M' then append DEL(w) to E
            //     and apply DEL(w) to T1
            EditOperation::Delete *del;
            del = new EditOperation::Delete(w);
            E.push_back(del);
            del->Apply(nodes1);
        }
    }

    if (t1->Get<ChildVectorInfo>()->size())
    {
        // Sync child pointers and check tree consistency
        SyncWithChildVectorInfo action;
        t1->Do(action);
    }
    else
        ((Block *)t1)->child = NULL;

    IFTRACE(diff)
       std::cout << "DoEditScript done\n";
}

unsigned TreeDiff::FindPos(node_id x)
{
    // 1. Let y = p(x) in T2 and let w be the partner of x
    Tree *yptr = nodes2[x].Parent();
    assert(yptr);

    // 2. If x is the leftmost child of y that is marked "in order", return 1
    std::vector<Tree *> *cv = yptr->Get<ChildVectorInfo>();
    assert(cv);
    bool in_order_found = false;
    for (unsigned i = 0; i < cv->size(); i++)
    {
        Tree *cur = (*cv)[i];
        if (cur->Get<InOrderInfo>() == true)
        {
            if (cur->Get<NodeIdInfo>() == x)
                return 1;
            else
            {
                in_order_found = true;
                break;
            }
        }
    }
    if (!in_order_found)    // FIXME - CHECK
        return 1;

    // 3. Find v in T2 where v is the rightmost sibling of x that is to the left
    //    of x and is marked "in order"
    Tree *vptr = NULL;
    bool found_x = false;
    for (unsigned i = cv->size() - 1; ; i--)
    {
        Tree *cur = (*cv)[i];
        if (found_x && cur->Get<InOrderInfo>() == true)
        {
            vptr = cur;
            break;
        }
        if (cur->Get<NodeIdInfo>() == x)
            found_x = true;
        if (i == 0)
            break;
    }
    if (!vptr)  // FIXME - CHECK
        return 1;
    node_id v = vptr->Get<NodeIdInfo>();

    // 4. Let u be the partner of v in T1
    node_id u = matching.fro[v];

    // 5. Suppose u is the ith child of its parent that is marked "in order."
    //    Return i+1.
    Tree *pu_ptr = nodes1[u].Parent();
    assert (pu_ptr);
    cv = pu_ptr->Get<ChildVectorInfo>();
    unsigned count = 0;
    for (unsigned i = 0; i < cv->size(); i++)
    {
        Tree *cur = (*cv)[i];
        if (cur->Get<InOrderInfo>())
            count++;
        if (cur->Get<NodeIdInfo>() == u)
            break;
    }
    return count + 1;
}

static bool FindPair(node_id a, node_id b,
                     node_vector_align &s1, node_vector_align &s2)
// ----------------------------------------------------------------------------
//    Return true if (a, b) is found in {(xi, yi) / xi in s1 and yi in s2}
// ----------------------------------------------------------------------------
{
   for (unsigned int i = 0; i < s1.size(); i++)
       if (s1[i].Id() == a && s2[i].Id() == b)
           return true;
   return false;
}

void TreeDiff::AlignChildren(node_id w, node_id x)
// ----------------------------------------------------------------------------
//    Generate Move operations if children of w and x are mis-aligned
// ----------------------------------------------------------------------------
{
    Tree * wptr = nodes1[w].GetTree();
    assert(wptr);
    if (wptr->IsLeaf())
        return;
    Tree * xptr = nodes2[x].GetTree();
    assert(xptr);

    IFTRACE(diff)
       std::cout << " Entering AlignChildren(" << w << ", " << x << ")\n";

    std::vector<Tree *> *cv;
    std::vector<Tree *>::iterator it;

    // 1. Mark all children of w and all children of x "out of order"
    // (Nothing to do, it's the default state)

    // 2. Let S1 be the sequence of children of w whose partners are
    // children of x...
    node_vector_align S1;
    cv = wptr->Get<ChildVectorInfo>();
    assert(cv);
    for (it = cv->begin(); it != cv->end() && (*it); it++)
    {
        node_id child = (*it)->Get<NodeIdInfo>();
        node_id partner = matching.to[child];
        if (nodes2[partner].Parent() == xptr)
            S1.push_back(NodeForAlign(matching, nodes1[child].GetTree()));
    }

    // 2. ...and lest S2 be the sequence of children of x whose partners are
    // children of w.
    node_vector_align S2;
    cv = xptr->Get<ChildVectorInfo>();
    for (it = cv->begin(); it != cv->end() && (*it); it++)
    {
        node_id child = (*it)->Get<NodeIdInfo>();
        node_id partner = matching.fro[child];
        if (nodes1[partner].Parent() == wptr)
            S2.push_back(NodeForAlign(matching, nodes2[child].GetTree()));
    }

    // 3. Define the function equal(a, b) to be true iff (a, b) in M'
    // (See NodeForAlign::operator ==)

    // 4. Let S <- LCS(S1, S2, equal)
    IFTRACE(diff)
        std::cout << "  Running LCS...";
    node_vector_align lcs1, lcs2;
    LCS<node_vector_align> lcs_algo;
    lcs_algo.Compute(S1, S2);
    lcs_algo.Extract2(S1, lcs1, S2, lcs2);
    IFTRACE(diff)
        std::cout << " done, " << lcs1.size() << " node(s)\n";

    // 5. For each (a,b) in S, mark nodes a and b "in order"
    for (unsigned int i = 0; i < lcs1.size(); i++)
    {
        NodeForAlign &a = lcs1[i], &b = lcs2[i];
        a.SetInOrder();
        b.SetInOrder();
    }


    IFTRACE(diff)
        std::cout << "  Moving nodes..." << std::flush;
    unsigned count = 0;
    // 6. For each a in S1, b in S2 such that (a, b) in M but (a, b) not in S
    for (unsigned int i = 0; i < S1.size(); i++)
    {
        for (unsigned int j = 0; j < S2.size(); j++)
        {
            node_id a = S1[i].Id(), b = S2[j].Id();

            if (matching.to[a] == b)
                if (!FindPair(a, b, lcs1, lcs2))
                {
                    // (a) k <- FindPos(b)
                    unsigned k = FindPos(b);

                    EditOperation::Move *mov;
                    mov = new EditOperation::Move(a, w, k);

                    // (b) Append MOV(a, w, k) to E...
                    escript->push_back(mov);

                    // (b) Apply MOV(a, w, k) to T1
                    mov->Apply(nodes1);

                    // (c) Mark a and b "in order"
                    S1[i].SetInOrder();
                    S2[j].SetInOrder();

                    count++;
                }
        }
    }
    IFTRACE(diff)
        std::cout << " done, " << count << " node(s)\n";
    IFTRACE(diff)
       std::cout << " AlignChildren done.\n";
}

bool TreeDiff::Diff()
// ----------------------------------------------------------------------------
//    Compute the difference between the two trees (Edit Script)
// ----------------------------------------------------------------------------
{
    // We first add a dummy root node (a block) to both trees so that the
    // following statement always holds true:
    // [CDHSI] We assume, without loss of generality, that the roots of T1 and
    //   T2 are matched in M.
    // The dummy root nodes have ID 0
    t1 = new Block(t1, "<", ">");
    t2 = new Block(t2, "<", ">");
    matching.insert(0, 0);
    t1->Set2<MatchedInfo>(true);
    t2->Set2<MatchedInfo>(true);

    // Assign some IDs to tree nodes and build node tables.
    // The first tree is numbered starting from 1, while the second one
    // is numbered with negative integers (-1, -2, etc.)
    // Start from 0 to account for the dummy root nodes
    AssignNodeIds sni1(nodes1, 0, 1);
    BreadthFirstSearch bfs_sni1(sni1);
    t1->Do(bfs_sni1);
    nodes1.SetNextId(sni1.id);
    nodes1.SetStep(1);
    AssignNodeIds sni2(nodes2, 0, -1);
    BreadthFirstSearch bfs_sni2(sni2);
    t2->Do(bfs_sni2);
    nodes2.SetNextId(sni2.id);
    nodes2.SetStep(-1);

    // Set 'parent' pointers in each node of each tree
    SetParentInfo spi;
    BreadthFirstSearch bfs_spi(spi);
    t1->Do(bfs_spi);
    t2->Do(bfs_spi);

    // Count the number of leaves under each node
    CountLeaves cnt;
    t1->Do(cnt);
    t2->Do(cnt);

    IFTRACE(diff)
    {
        std::cout << "T1:";
        debugp(t1);
        std::cout << "T1 nodes:\n" << nodes1 << "\n";
        std::cout << "T2:";
        debugp(t2);
        std::cout << "T2 nodes:\n" << nodes2 << "\n";
    }

    // Find a "good" matching between t1 and t2
    FastMatch();

    IFTRACE(diff)
        std::cout << "Matching:\n" << matching << "\n";

    // Generate list of operations to transform t1 into t2
    DoEditScript();

    IFTRACE(diff)
    {
        std::cout << "T1 (after transformation):";
        debugp(t1);
    }

    // Use hash to check that transformed tree is identical to target
    HashInfo<>::data_t h1, h2;
    TreeHashAction<> h_action(TreeHashAction<>::Force);
    t1->Do(h_action);
    h1 = t1->Get< HashInfo<> >();
    h_action.reset();
    t2->Do(h_action);
    h2 = t2->Get< HashInfo<> >();
    if (h1 != h2)
    {
        std::cerr << "Diff error: T2 and T1 (after transformation) "
                  << "are not identical!\n"
                  << "Diff error: [" << h2 << " / " << h1 << "]\n";
        return true;
    }

    return false;
}

bool TreeDiff::Diff(std::ostream &os)
// ----------------------------------------------------------------------------
//    Compute tree diff and display it to os. Return true if error.
// ----------------------------------------------------------------------------
{
    bool hadError = Diff();
    os << *escript;
    return hadError;
}

bool Node::operator ==(const Node &n)
{
    Tree *t1 = t;
    Tree *t2 = n.GetTree();

    if (t1->Kind() != t2->Kind())
        return false;

    if (t1->IsLeaf())
        return LeafEqual(t1, t2);
    else
        return NonLeafEqual(t1, t2, *(t1->Get<TreeDiffInfo>()));
}

bool NodeForAlign::operator ==(const NodeForAlign &n)
{
    if (!IsMatched())
        return false;
    return (m.to[Id()] == n.Id());
}

void EditOperation::Insert::Apply(NodeTable &table)
// ----------------------------------------------------------------------------
//    Apply an Insert operation on a node table and the underlying tree
// ----------------------------------------------------------------------------
{
    Tree *pp = table[parent].GetTree();
    node_id new_id = table.NewId();

    table[new_id] = leaf;
    leaf->Set2<ParentInfo>(pp);
    leaf->Set2<NodeIdInfo>(new_id);
    SetChildVectorInfo scvi;
    leaf->Do(scvi);
    std::vector<Tree *> *v = pp->Get<ChildVectorInfo>();
    v->insert(v->begin() + pos - 1, leaf);
}

void EditOperation::Delete::Apply(NodeTable &table)
// ----------------------------------------------------------------------------
//    Apply a Delete operation on a node table and the underlying tree
// ----------------------------------------------------------------------------
{
    Tree *lp = table[leaf].GetTree(), *pp = lp->Get<ParentInfo>();
    std::vector<Tree *> *v = pp->Get<ChildVectorInfo>();
    std::vector<Tree *>::iterator it;

    for (it = v->begin(); it != v->end(); it++)
        if ((*it) == lp)
        {
            break;
        }
    std::vector<Tree *> *vl = lp->Get<ChildVectorInfo>();
    if (vl && vl->size() != 0)
    {
        // If deleted node had ONE child, replace deleted node by child
        assert(vl->size() == 1);
        v->erase(it);
        v->insert(it, (*vl)[0]);
        (*vl)[0]->Set2<ParentInfo>(pp);
    }
    else
    {
      v->erase(it);
    }
    delete lp;
    table.erase(leaf);
}

void EditOperation::Update::Apply(NodeTable &table)
// ----------------------------------------------------------------------------
//    Apply an Update operation on a node table and the underlying tree
// ----------------------------------------------------------------------------
{
    TreeCopyTemplate<CM_NODE_ONLY> copy(table[leaf].GetTree());
    value->Do(copy);
}

void EditOperation::Move::Apply(NodeTable &table)
// ----------------------------------------------------------------------------
//    Apply a Move operation on a node table and the underlying tree
// ----------------------------------------------------------------------------
{
    Tree *sp  = table[subtree].GetTree();
    Tree *pp  = table[parent].GetTree();
    Tree *psp = sp->Get<ParentInfo>();

    sp->Set2<ParentInfo>(pp);

    std::vector<Tree *> *v;
    v = pp->Get<ChildVectorInfo>();
    v->insert(v->begin() + pos - 1, sp);

    v = psp->Get<ChildVectorInfo>();
    std::vector<Tree *>::iterator it;
    for (it = v->begin(); it != v->end(); it++)
        if ((*it) == sp)
        {
            v->erase(it);
            break;
        }
}

Tree * EditScript::Apply(Tree *tree)
// ----------------------------------------------------------------------------
//    Apply an Edit Script to a tree
// ----------------------------------------------------------------------------
{
    // TODO
    return NULL;
}

std::ostream&
operator <<(std::ostream &out, XL::NodeTable &m)
// ----------------------------------------------------------------------------
//    Display a collection of nodes indexed by node_id
// ----------------------------------------------------------------------------
{
    if (m.empty())
        return out;

    XL::PrintNode pn(out, true);
    XL::NodeTable::iterator it;
    if ((*m.begin()).first >= 0)
    {
        for (it = m.begin(); it != m.end(); it++)
        {
            (*it).second.GetTree()->Do(pn);
            out << std::endl;
        }
        return out;
    }

    XL::NodeTable::reverse_iterator rit;
    for (rit = m.rbegin(); rit != m.rend(); rit++)
    {
        (*rit).second.GetTree()->Do(pn);
        out << std::endl;
    }
    return out;
}

std::ostream&
operator <<(std::ostream &out, Matching &m)
// ----------------------------------------------------------------------------
//    Display a matching (correspondence between nodes of two trees)
// ----------------------------------------------------------------------------
{
    XL::Matching::iterator it;
    for (it = m.begin(); it != m.end(); it++)
        out << (*it).first << " -> " << (*it).second << std::endl;

    return out;
}

std::ostream&
// ----------------------------------------------------------------------------
//    Display an insert operation
// ----------------------------------------------------------------------------
operator <<(std::ostream &out, EditOperation::Insert &op)
{
    XL::PrintNode pn(out);
    out << "INS((" << op.leaf->Get<NodeIdInfo>() << ", ";
    op.leaf->Do(pn);
    out << "), " << op.parent << ", " << op.pos << ")";
    return out;
}

std::ostream&
// ----------------------------------------------------------------------------
//    Display a delete operation
// ----------------------------------------------------------------------------
operator <<(std::ostream &out, EditOperation::Delete &op)
{
    out << "DEL(" << op.leaf << ")";
    return out;
}

std::ostream&
// ----------------------------------------------------------------------------
//    Display an update operation
// ----------------------------------------------------------------------------
operator <<(std::ostream &out, EditOperation::Update &op)
{
    XL::PrintNode pn(out);
    out << "UPD(" << op.leaf << ", ";
    op.value->Do(pn);
    out << ")";
    return out;
}

std::ostream&
// ----------------------------------------------------------------------------
//    Display a move operation
// ----------------------------------------------------------------------------
operator <<(std::ostream &out, EditOperation::Move &op)
{
    out << "MOV(" << op.subtree
        << ", "   << op.parent
        << ", "   << op.pos     << ")";
    return out;
}

std::ostream&
// ----------------------------------------------------------------------------
//    Display an edit script
// ----------------------------------------------------------------------------
operator <<(std::ostream &out, EditScript &s)
{
    std::list<XL::EditOperation::Base *>::iterator it;

    for (it = s.begin(); it != s.end();)
    {
        XL::EditOperation::Insert *ins;
        XL::EditOperation::Delete *del;
        XL::EditOperation::Update *upd;
        XL::EditOperation::Move   *mov;
        if ((ins = dynamic_cast<XL::EditOperation::Insert *>(*it)))
            out << *ins;
        else if ((del = dynamic_cast<XL::EditOperation::Delete *>(*it)))
            out << *del;
        else if ((upd = dynamic_cast<XL::EditOperation::Update *>(*it)))
            out << *upd;
        else if ((mov = dynamic_cast<XL::EditOperation::Move *>(*it)))
            out << *mov;
        it++;
        if (it != s.end())
            out << ", ";
        else
            out << std::endl;
    }
    return out;
}

XL_END
