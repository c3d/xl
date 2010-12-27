#ifndef TREE_CLONE_H
#define TREE_CLONE_H
// ****************************************************************************
//  tree-clone.h                                                    XLR project
// ****************************************************************************
// 
//   File Description:
// 
//    Tree clone and copy operations
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
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "tree.h"

XL_BEGIN

// ============================================================================
//
//    Tree cloning
//
// ============================================================================

struct DeepCopyCloneMode;       // Child nodes are cloned too (default)
struct ShallowCopyCloneMode;    // Child nodes are referenced
struct NodeOnlyCloneMode;       // Child nodes are left NULL


template <typename mode> struct TreeCloneTemplate
// ----------------------------------------------------------------------------
//   Clone a tree
// ----------------------------------------------------------------------------
{
    TreeCloneTemplate() {}
    ~TreeCloneTemplate() {}

    typedef Tree *value_type;

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


template <CopyMode mode> struct TreeCopyTemplate
// ----------------------------------------------------------------------------
//   Copy a tree into another tree. Node values are copied, infos are not.
// ----------------------------------------------------------------------------
{
    TreeCopyTemplate(Tree *dest): dest(dest) {}
    ~TreeCopyTemplate() {}

    typedef Tree *value_type;

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

XL_END

#endif // TREE_CLONE_H
