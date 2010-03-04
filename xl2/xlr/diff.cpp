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
#include <iostream>
#include <cassert>

XL_BEGIN

#include "sha1_ostream.h" // Won't work outside of XL namespace

TreeDiff::TreeDiff(Tree *t1, Tree *t2) : t1(NULL), t2(t2), escript(NULL)
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

    matching::iterator it;
    for (it = m.begin(); it != m.end(); it++)
        delete (*it);

    if (escript)
        delete escript;
}

node_id TreeDiff::AssignNodeIds(Tree *t, node_table &m, node_id from_id,
                                node_id step)
// ----------------------------------------------------------------------------
//    Assign a node_id identifier to each node of a tree and build a node map
// ----------------------------------------------------------------------------
{
    // Nodes are numbered in breadth-first order
    // This is important for the edit script generation algorithm

    TreeDiff::SetNodeIdAction action(m, from_id, step);
    BreadthFirstSearch bfs(action);
    t->Do(bfs);
    m.SetNextId(action.id);
    m.SetStep(step);
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

void TreeDiff::CreateChildVectors(Tree *t)
// ----------------------------------------------------------------------------
//    Prepare tree so that EditOperations may be applied to it
// ----------------------------------------------------------------------------
{
    SetChildVectorInfo action;
    InOrderTraversal iot(action);
    t->Do(iot);
}

void TreeDiff::UpdateChildren(Tree *t)
// ----------------------------------------------------------------------------
//    Sync child pointers after an edit operation and check tree consistency
// ----------------------------------------------------------------------------
{
    SyncWithChildVectorInfo action;
    t->Do(action);
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
        std::cout << "  Matching (kind = " << S1.front().GetTree()->Kind() << ")\n";

    // Compute the Longest Common Subsequence in the node chains of
    // both trees...
    IFTRACE(diff)
        std::cout << "   Running LCS...";
    node_vector lcs1, lcs2;
    LCS<node_vector> lcs_algo;
    lcs_algo.Compute(S1, S2);
    lcs_algo.Extract2(S1, lcs1, S2, lcs2);
    IFTRACE(diff)
        std::cout << " done.\n";

    // ...add node pairs to matching...
    for (unsigned int i = 0; i < lcs1.size(); i++)
    {
        Node &x = lcs1[i], &y = lcs2[i];
        if (!x.IsMatched())
        {
            NodePair *p = new NodePair(x.Id(), y.Id());
            M.insert(p);
            x.SetMatched();
            y.SetMatched();
        }
    }

    // Nodes still unmatched after the call to LCS are processed using
    // linear search
    IFTRACE(diff)
        std::cout << "   Running linear matching...";
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
        std::cout << " done.\n";
}

void TreeDiff::DoFastMatch()
// ----------------------------------------------------------------------------
//    Compute a "good" matching between trees t1 and t2
// ----------------------------------------------------------------------------
{
    node_vector chains1[KIND_LAST + 1];  // All nodes of T1, per kind
    node_vector chains2[KIND_LAST + 1];  // All nodes of T2, per kind

    IFTRACE(diff)
       std::cout << "Entering DoFastMatch\n";

    // For each tree, sort the nodes into node chains (one for each node kind).
    BuildChains(t1, chains1);
    BuildChains(t2, chains2);

    IFTRACE(diff)
        std::cout << " Matching leaves\n";
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
        (*it).second.GetTree()->Set2<TreeDiffInfo>(this);

    IFTRACE(diff)
        std::cout << " Matching internal nodes\n";
    for (int k = KIND_NLEAF_FIRST; k <= KIND_NLEAF_LAST; k++)
    {
        node_vector &S1 = chains1[k], &S2 = chains2[k];
        MatchOneKind(m, S1, S2);
    }

    IFTRACE(diff)
        std::cout << "Matching done. " << m.size()
                  << "/" << nodes1.size() << " nodes ("
                  << (float)m.size()*100/nodes1.size() << "%) matched.\n";
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

void TreeDiff::DoEditScript()
// ----------------------------------------------------------------------------
//    Compute an Edit Script to change t1 into t2, given a matching m
// ----------------------------------------------------------------------------
{
    IFTRACE(diff)
       std::cout << "Entering DoEditScript\n";

    // Prepare T1 and T2
    CreateChildVectors(t1);
    CreateChildVectors(t2);

    // 1.
    escript = new EditScript;
    EditScript &E = *escript;
    matching &Mprime = m;

    // 2. Visit the nodes of T2 in breadth-first order
    XL::TreeDiff::node_table::reverse_iterator rit;
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
        node_id w = PartnerInFirstTree(x, Mprime);
        node_id z = PartnerInFirstTree(y, Mprime);
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
            NodePair *p = new NodePair(w, x);
            Mprime.insert(p);
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
                if (y != PartnerInSecondTree(v, Mprime))
                {
                    // iii. If (y, v) not in M'
                    
                    // A. Let z be the partner of y in M'
                    node_id z = PartnerInFirstTree(y, Mprime);
                    
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
        UpdateChildren(t1);
    else
        ((Block *)t1)->child = NULL;

    IFTRACE(diff)
       std::cout << "DoEditScript done\n";
}

// TODO: find a better type for 'matching' so that this can be implemented
// with a direct access
node_id TreeDiff::PartnerInFirstTree(node_id id, const matching &M)
{
    XL::TreeDiff::matching::iterator it;
    for (it = M.begin(); it != M.end(); it++)
        if ((*it)->y == id)
            return (*it)->x;

    return 0;
}

// TODO: find a better type for 'matching' so that this can be implemented
// with a direct access
node_id TreeDiff::PartnerInSecondTree(node_id id, const matching &M)
{
    XL::TreeDiff::matching::iterator it;
    for (it = M.begin(); it != M.end(); it++)
        if ((*it)->x == id)
            return (*it)->y;

    return 0;
}

unsigned TreeDiff::FindPos(node_id x)
{
    // 1. Let y = p(x) in T2 and let w be the partner of x
    Tree *yptr = nodes2[x].GetTree()->Get<ParentInfo>();
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
    node_id u = PartnerInFirstTree(v, m);

    // 5. Suppose u is the ith child of its parent that is marked "in order."
    //    Return i+1.
    Tree *pu_ptr = nodes1[u].GetTree()->Get<ParentInfo>();
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

void TreeDiff::AlignChildren(node_id w, node_id x)
// ----------------------------------------------------------------------------
//    Generate Move operations if children of w and x are mis-aligned
// ----------------------------------------------------------------------------
{
    IFTRACE(diff)
       std::cout << " Entering AlignChildren(" << w << ", " << x << ")\n";

    Tree * wptr = nodes1[w].GetTree();
    assert(wptr);
    if (wptr->IsLeaf())
        return;
    Tree * xptr = nodes2[x].GetTree();
    assert(xptr);

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
        node_id partner = PartnerInSecondTree(child, m);
        if (nodes2[partner].GetTree()->Get<ParentInfo>() == xptr)
            S1.push_back(NodeForAlign(m, nodes1[child].GetTree()));
    }

    // 2. ...and lest S2 be the sequence of children of x whose partners are
    // children of w.
    node_vector_align S2;
    cv = xptr->Get<ChildVectorInfo>();
    for (it = cv->begin(); it != cv->end() && (*it); it++)
    {
        node_id child = (*it)->Get<NodeIdInfo>();
        node_id partner = PartnerInFirstTree(child, m);
        if (nodes1[partner].GetTree()->Get<ParentInfo>() == wptr)
            S2.push_back(NodeForAlign(m, nodes2[child].GetTree()));
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
        std::cout << " done.\n";

    // 5. For each (a,b) in S, mark nodes a and b "in order"
    for (unsigned int i = 0; i < lcs1.size(); i++)
    {
        NodeForAlign &a = lcs1[i], &b = lcs2[i];
        a.SetInOrder();
        b.SetInOrder();
    }


    // 6. For each a in S1, b in S2 such that (a, b) in M but (a, b) not in S
    for (unsigned int i = 0; i < S1.size(); i++)
    {
        for (unsigned int j = 0; j < S2.size(); j++)
        {
            node_id a = S1[i].Id(), b = S2[j].Id();

            if (PartnerInSecondTree(a, m) == b)
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
                }
        }
    }
    IFTRACE(diff)
       std::cout << " AlignChildren done.\n";
}

bool TreeDiff::FindPair(node_id a, node_id b,
                        node_vector_align s1, node_vector_align s2)
{
   for (unsigned int i = 0; i < s1.size(); i++)
       if (s1[i].Id() == a && s2[i].Id() == b)
           return true;
   return false;
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
    m.insert(new NodePair(0, 0));
    t1->Set2<MatchedInfo>(true);
    t2->Set2<MatchedInfo>(true);

    HashInfo<>::data_t h1, h2;
    TreeHashAction<> h_action(TreeHashAction<>::Force);
    t2->Do(h_action);
    h2 = t2->Get< HashInfo<> >();

    // Assign some IDs to tree nodes and build node tables.
    // The first tree is numbered starting from 1, while the second one
    // is numbered with negative integers (-1, -2, etc.)
    // Start from 0 to account for the dummy root nodes
    AssignNodeIds(t1, nodes1, 0);
    AssignNodeIds(t2, nodes2, 0, -1);

    // Set 'parent' pointers in each node of each tree
    SetParentPointers(t1);
    SetParentPointers(t2);

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
    DoFastMatch();

    IFTRACE(diff)
        std::cout << "Matching:\n" << m << "\n";

    // Generate list of operations to transform t1 into t2
    DoEditScript();

    IFTRACE(diff)
    {
        std::cout << "T1 (after transformation):";
        debugp(t1);
    }

    h_action.reset();
    t1->Do(h_action);
    h1 = t1->Get< HashInfo<> >();
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

bool TreeDiff::NodeForAlign::operator ==(const TreeDiff::NodeForAlign &n)
{
    if (!IsMatched())
        return false;
    return (PartnerInSecondTree(Id(), m) == n.Id());
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
    t->Set2<LeafCountInfo>(act.nb_leaves);
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

void EditOperation::Insert::Apply(TreeDiff::node_table &table)
// ----------------------------------------------------------------------------
//    Apply an Insert operation on a node table and the underlying tree
// ----------------------------------------------------------------------------
{
    if (table.find(parent) == table.end())
        return;

    Tree *parent_ptr = table[parent].GetTree();
    if (!parent_ptr)
        return;

    node_id new_id = table.NewId();
    table[new_id] = leaf;
    leaf->Set2<ParentInfo>(parent_ptr);
    leaf->Set2<NodeIdInfo>(new_id);
    SetChildVectorInfo scvi;
    leaf->Do(scvi);
    std::vector<Tree *> *v = parent_ptr->Get<ChildVectorInfo>();
    v->insert(v->begin() + pos - 1, leaf);
}

void EditOperation::Delete::Apply(TreeDiff::node_table &table)
// ----------------------------------------------------------------------------
//    Apply a Delete operation on a node table and the underlying tree
// ----------------------------------------------------------------------------
{
    if (table.find(leaf) == table.end())
        return;

    Tree *parent_ptr = NULL, *leaf_ptr = table[leaf].GetTree();
    if (leaf_ptr)
        parent_ptr = leaf_ptr->Get<ParentInfo>();
    if (parent_ptr)
    {
        std::vector<Tree *> *v = parent_ptr->Get<ChildVectorInfo>();
        std::vector<Tree *>::iterator it;
        for (it = v->begin(); it != v->end(); it++)
            if ((*it) == leaf_ptr)
            {
                v->erase(it);
                break;
            }
    }
    delete leaf_ptr;
    table.erase(leaf);
}

void EditOperation::Update::Apply(TreeDiff::node_table &table)
// ----------------------------------------------------------------------------
//    Apply an Update operation on a node table and the underlying tree
// ----------------------------------------------------------------------------
{
    if (table.find(leaf) == table.end())
        return;

    Tree *leaf_ptr = table[leaf].GetTree();
    if (!leaf_ptr)
        return;

    TreeCopyTemplate<CM_NODE_ONLY> copy(leaf_ptr);
    this->value->Do(copy);
}

void EditOperation::Move::Apply(TreeDiff::node_table &table)
// ----------------------------------------------------------------------------
//    Apply a Move operation on a node table and the underlying tree
// ----------------------------------------------------------------------------
{
    if (table.find(subtree) == table.end() ||
        table.find(parent)  == table.end())
        return;

    Tree *subtree_ptr = table[subtree].GetTree();         // subtree
    Tree *psubtree_ptr = subtree_ptr->Get<ParentInfo>();  // parent of subtree
    Tree *parent_ptr = table[parent].GetTree();           // new parent

    if (!subtree_ptr || !parent_ptr)
        return;

    // Update parent pointer of subtree
    subtree_ptr->Set2<ParentInfo>(parent_ptr);
    // Insert subtree_ptr in child vector of new parent 
    std::vector<Tree *> *v;
    v = parent_ptr->Get<ChildVectorInfo>();
    v->insert(v->begin() + pos - 1, subtree_ptr);
    // Remove subtree_ptr from child vector of old parent
    v = psubtree_ptr->Get<ChildVectorInfo>();
    std::vector<Tree *>::iterator it;
    for (it = v->begin(); it != v->end(); it++)
        if ((*it) == subtree_ptr)
        {
            v->erase(it);
            break;
        }
}

void EditScript::Apply(TreeDiff::node_table &table)
// ----------------------------------------------------------------------------
//    Apply an Edit Script to a node table and the underlying tree
// ----------------------------------------------------------------------------
{
    EditScriptBase::iterator it;

    for (it = this->begin(); it != this->end() ; it++)
        (*it)->Apply(table);
}

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
            (*it).second.GetTree()->Do(pn);
            out << std::endl;
        }
        return out;
    }

    XL::TreeDiff::node_table::reverse_iterator rit;
    for (rit = m.rbegin(); rit != m.rend(); rit++)
    {
        (*rit).second.GetTree()->Do(pn);
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
        m[i].GetTree()->Do(pn);

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
    out << "INS((" << op.leaf->Get<NodeIdInfo>() << ", ";
    op.leaf->Do(pn);
    out << "), " << op.parent << ", " << op.pos << ")";
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
        else
            out << std::endl;
    }
    return out;
}

XL_END
