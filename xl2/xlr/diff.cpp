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
#include <iostream>
#include <cassert>

XL_BEGIN

TreeDiff::~TreeDiff()
{
    node_table *tab[] = { &nodes1, &nodes2 };

    for (unsigned i = 0; i < (sizeof(tab)/sizeof(node_table *)); i++)
    {
        node_table::iterator it;
        for (it = tab[i]->begin(); it != tab[i]->end(); it++)
        {
            (*it).second.GetTree()->Purge<NodeIdInfo>();
            (*it).second.GetTree()->Purge<MatchedInfo>();
            (*it).second.GetTree()->Purge<TreeDiffInfo>();
            (*it).second.GetTree()->Purge<LeafCountInfo>();
            (*it).second.GetTree()->Purge<ParentInfo>();
        }
    }
    matching::iterator it;
    for (it = m.begin(); it != m.end(); it++)
        delete (*it);
}

node_id TreeDiff::AssignNodeIds(Tree *t, node_table &m, node_id from_id,
                                node_id step)
// ----------------------------------------------------------------------------
//    Assign a node_id identifier to each node of a tree and build a node map
// ----------------------------------------------------------------------------
{
    // Nodes are numbered in breadth-first order
    // Any other unique numbering would be OK

    TreeDiff::SetNodeIdAction action(m, from_id, step);
    BreadthFirstSearch bfs(action);
    t->Do(bfs);
    return action.id;
}

void TreeDiff::SetParentPointers(Tree *t)
// ----------------------------------------------------------------------------
//    Set ParentInfo (pointer to parent node) in each node of t 
// ----------------------------------------------------------------------------
{
    SetParentInfo action;
    BreadthFirstSearch bfs(action);
    t->Do(bfs);
}

void TreeDiff::BuildChains(Tree *t, node_vector *out)
// ----------------------------------------------------------------------------
//    Build per-kind node tables for a tree
// ----------------------------------------------------------------------------
{
    // This method performs an in-order traversal of the tree (siblings are
    // visited from left to right) and stores the nodes into per-label tables.
    // It is assumed that 'out' is a pre-allocated array of KIND_LAST + 1
    // node_vector elements.

    StoreNodeIntoChainArray action(out);
    InOrderTraversal iot(action);
    t->Do(iot);
}

void TreeDiff::MatchOneKind(matching &M, node_vector &S1, node_vector &S2)
// ----------------------------------------------------------------------------
//    Find a matching between two series of nodes of the same kind
// ----------------------------------------------------------------------------
{
    if (S1.empty())
        return;

    IFTRACE(diff)
        std::cout << "Matching nodes, kind = " << S1.front().Id() << std::endl;

    // Compute the Longest Common Subsequence in the node chains of
    // both trees...
    IFTRACE(diff)
        std::cout << "Running LCS...";
    node_vector lcs1, lcs2;
    LCS<node_vector> lcs_algo;
    lcs_algo.Compute(S1, S2);
    lcs_algo.Extract2(S1, lcs1, S2, lcs2);
    IFTRACE(diff)
        std::cout << " done." << std::endl;

    // ...add node pairs to matching...
    for (unsigned int i = 0; i < lcs1.size(); i++)
    {
        Node &x = lcs1[i], &y = lcs2[i];
        NodePair *p = new NodePair(x.Id(), y.Id());
        M.insert(p);
        x.SetMatched();
        y.SetMatched();
    }

    // Nodes still unmatched after the call to LCS are processed using
    // linear search
    IFTRACE(diff)
        std::cout << "Running linear matching...";
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
                NodePair *p = new NodePair((*x).Id(), (*y).Id());
                M.insert(p);
                // B. Mark x and y "matched".
                (*x).SetMatched();
                (*y).SetMatched();
                break;
            }
        }
    }
    IFTRACE(diff)
        std::cout << " done." << std::endl;
}

void TreeDiff::DoFastMatch()
// ----------------------------------------------------------------------------
//    Compute a "good" matching between trees t1 and t2
// ----------------------------------------------------------------------------
{
    node_vector chains1[KIND_LAST + 1];  // All nodes of T1, per kind
    node_vector chains2[KIND_LAST + 1];  // All nodes of T2, per kind

    IFTRACE(diff)
       std::cout << "Entering FastMatch" << std::endl;

    // For each tree, sort the nodes into node chains (one for each node kind).
    BuildChains(t1, chains1);
    BuildChains(t2, chains2);

    IFTRACE(diff)
        std::cout << "Matching leaves" << std::endl;
    for (int k = KIND_LEAF_FIRST; k <= KIND_LEAF_LAST; k++)
    {
        node_vector &S1 = chains1[k], &S2 = chains2[k];
        MatchOneKind(m, S1, S2);
    }

    // FIXME:
    // TreeDiff object (this) must be available to each Node of first tree
    // because it is used by Node::operator== for internal nodes.
    node_table::iterator it;
    for (it = nodes1.begin(); it != nodes1.end(); it++)
        (*it).second.GetTree()->Set<TreeDiffInfo>(this);

    IFTRACE(diff)
        std::cout << "Matching internal nodes" << std::endl;
    for (int k = KIND_NLEAF_FIRST; k <= KIND_NLEAF_LAST; k++)
    {
        node_vector &S1 = chains1[k], &S2 = chains2[k];
        MatchOneKind(m, S1, S2);
    }

    IFTRACE(diff)
        std::cout << "Matching done. " << m.size()
                  << "/" << nodes1.size() << " nodes ("
                  << (float)m.size()*100/nodes1.size() << "%) matched."
                  << std::endl;
}

float TreeDiff::Similarity(std::string s1, std::string s2)
// ----------------------------------------------------------------------------
//    Return a similarity score bewteen 0 and 1 (1 means strings are equal).
// ----------------------------------------------------------------------------
{
#ifdef NO_LCS_STRCMP
    // FIXME: computing the LCS seems quite time-consuming.
    // We may have to choose a different algorithm.
    // CHECK THIS: diff lorem1.xl lorem2.xl is much much faster than
    //             xlr -diff lorem1.xl lorem2.xl
    // Use simple string comparison
    return ((float)!s1.compare(s2));
#else
    // The score is the length of the longest common subsequence (LCS) of the
    // two strings, divided by the length of the longest string.
    // This yields a score of 0 if the strings have nothing in common, and 1
    // if they are identical.

    std::string *small = &s1;
    std::string *big = &s2;

    if (s1.length() > s2.length())
    {
        small = &s2;
        big = &s1;
    }

    if (big->length() == 0)
        return 1.0;


    LCS<std::string> lcs;
    lcs.Compute(*small, *big);
    float ret = (float)lcs.Length() / big->length();

    return ret;
#endif
}

bool TreeDiff::LeafEqual(Tree *t1, Tree *t2)
// ----------------------------------------------------------------------------
//    Test if two leaves should be considered equal.
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

bool TreeDiff::NonLeafEqual(Tree *t1, Tree *t2, TreeDiff &td)
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

    c1 = n1.CountLeaves();
    c2 = n2.CountLeaves();
    max = (c2 > c1) ? c2 : c1;

   // FIXME: this loop is quite time-consuming
    matching &M = td.m;
    matching::iterator it;
    for (it = M.begin(); it != M.end(); it++)
    {
        node_id wid = (*it)->x, zid = (*it)->y;
        const Node &x = Node(t1), &y = Node(t2);
        Tree *w = td.nodes1[wid].GetTree(), *z = td.nodes2[zid].GetTree();

        if (x.ContainsLeaf(w) && y.ContainsLeaf(z))
            common++;
    }

    float percent = (float)common / max;

    return (percent > 0.5);
}

void TreeDiff::Diff()
// ----------------------------------------------------------------------------
//    Compute the difference between the two trees. Returns an Edit Script.
// ----------------------------------------------------------------------------
{
    // First, assign some IDs to tree nodes and build node tables.
    // The first tree is numbered starting from 1, while the second one
    // is numbered with negative integers (-1, -2, etc.)
    AssignNodeIds(t1, nodes1);
    AssignNodeIds(t2, nodes2, -1, -1);

    if (!nodes1.size())
        return;

    // Set 'parent' pointers in each node of each tree
    SetParentPointers(t1);
    SetParentPointers(t2);

    // Find a "good" matching between t1 and t2
    DoFastMatch();

    IFTRACE(diff)
    {
      std::cout << "T1:\n" << nodes1 << "\n"
                << "T2:\n" << nodes2 << "\n"
                << "Matching:\n" << m << "\n";
    }

// TEST
    EditOperation::Insert ins(t1, 1, 4);
    std::cout << ins << std::endl;
    EditOperation::Move mov(5, 1, 4);
    std::cout << mov << std::endl;
    EditOperation::Delete del(2);
    std::cout << del << std::endl;
    EditOperation::Update upd(9, t2);
    std::cout << upd << std::endl;

    EditScript s;
    s.push_back(&ins);
    s.push_back(&del);
    s.push_back(&upd);
    s.push_back(&mov);
    std::cout << s << std::endl;
}

bool TreeDiff::Node::operator ==(const TreeDiff::Node &n)
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

unsigned int TreeDiff::Node::CountLeaves()
// ----------------------------------------------------------------------------
//    Return the number of leaves under a node; cache it for subsequent calls.
// ----------------------------------------------------------------------------
{
    Tree *t = GetTree();
    if (t->Exists<LeafCountInfo>())
        return t->Get<LeafCountInfo>();

    TreeDiff::CountLeaves act;
    PostOrderTraversal pot(act);
    t->Do(pot);
    t->Set<LeafCountInfo>(act.nb_leaves);
    return act.nb_leaves;
}

bool TreeDiff::Node::ContainsLeaf(Tree *leaf) const
// ----------------------------------------------------------------------------
//    Return true if node_id is a leaf and a child of current node
// ----------------------------------------------------------------------------
{
   // FIXME: optimize? A std::map<Tree *, bool> in each leaf (so that
   // tab[parent_ptr] is true if parent_ptr is a parent node of leaf)
   // does not improve performance (36.95s to FastMatch 8lorem vs. 36.14s
   // without optimization...)

    if (!leaf->IsLeaf())
        return false;

    Tree *cur = leaf;
    while (cur)
    {
        if (cur == t)
            return true;
        if (!cur->Exists<ParentInfo>())
            return false;
        cur = cur->Get<ParentInfo>();
    }
    return false;
}

XL_END

std::ostream&
operator <<(std::ostream &out, XL::TreeDiff::node_table &m)
// ----------------------------------------------------------------------------
//    Display a collection of nodes indexed by node_id
// ----------------------------------------------------------------------------
{
    if (m.empty())
        return out;

    XL::PrintNode pn(out, true);
    XL::TreeDiff::node_table::iterator it;
    if ((*m.begin()).first >= 0)
    {
        for (it = m.begin(); it != m.end(); it++)
        {
            (*it).second->Do(pn);
            out << std::endl;
        }
        return out;
    }

    XL::TreeDiff::node_table::reverse_iterator rit;
    for (rit = m.rbegin(); rit != m.rend(); rit++)
    {
        (*rit).second->Do(pn);
        out << std::endl;
    }
    return out;
}

std::ostream&
operator <<(std::ostream &out, XL::TreeDiff::node_vector &m)
// ----------------------------------------------------------------------------
//    Display a vector of nodes
// ----------------------------------------------------------------------------
{
    XL::PrintNode pn(out, true);
    for (unsigned i = 0; i < m.size(); i++)
        m[i]->Do(pn);

    return out;
}

std::ostream&
operator <<(std::ostream &out, XL::TreeDiff::matching &m)
// ----------------------------------------------------------------------------
//    Display a matching (correspondence between nodes of two trees)
// ----------------------------------------------------------------------------
{
    XL::TreeDiff::matching::iterator it;
    for (it = m.begin(); it != m.end(); it++)
        out << (*it)->x << " -> " << (*it)->y << std::endl;

    return out;
}

std::ostream&
// ----------------------------------------------------------------------------
//    Display an insert operation
// ----------------------------------------------------------------------------
operator <<(std::ostream &out, XL::EditOperation::Insert &op)
{
    XL::PrintNode pn(out);
    out << "INS(" ;
    op.leaf->Do(pn);
    out << ", " << op.parent << ", " << op.pos << ")";
    return out;
}

std::ostream&
// ----------------------------------------------------------------------------
//    Display a delete operation
// ----------------------------------------------------------------------------
operator <<(std::ostream &out, XL::EditOperation::Delete &op)
{
    out << "DEL(" << op.leaf << ")";
    return out;
}

std::ostream&
// ----------------------------------------------------------------------------
//    Display an update operation
// ----------------------------------------------------------------------------
operator <<(std::ostream &out, XL::EditOperation::Update &op)
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
operator <<(std::ostream &out, XL::EditOperation::Move &op)
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
operator <<(std::ostream &out, XL::EditScript &s)
{
    std::list<XL::EditOperation::Base *>::iterator it;

    for (it = s.begin(); it != s.end();)
    {
        XL::EditOperation::Insert *ins;
        XL::EditOperation::Delete *del;
        XL::EditOperation::Update *upd;
        XL::EditOperation::Move   *mov;
        if ((ins = dynamic_cast<XL::EditOperation::Insert *>(*it)))
        {
            out << *ins;
        }
        else if ((del = dynamic_cast<XL::EditOperation::Delete *>(*it)))
        {
            out << *del;
        }
        else if ((upd = dynamic_cast<XL::EditOperation::Update *>(*it)))
        {
            out << *upd;
        }
        else if ((mov = dynamic_cast<XL::EditOperation::Move *>(*it)))
        {
            out << *mov;
        }
        it++;
        if (it != s.end())
            out << ", ";
    }
    return out;
}
