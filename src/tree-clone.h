#ifndef TREE_CLONE_H
#define TREE_CLONE_H
// ****************************************************************************
//  tree-clone.h                                                  ELFE project
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
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "tree.h"

ELFE_BEGIN

// ============================================================================
//
//    Tree cloning
//
// ============================================================================

template <typename CloneMode> struct TreeCloneTemplate : CloneMode
// ----------------------------------------------------------------------------
//   Clone a tree
// ----------------------------------------------------------------------------
{
    TreeCloneTemplate() {}
    ~TreeCloneTemplate() {}

    typedef Tree *value_type;

public:
    // Clone configuration code (and main entry point)
    Tree *  Clone(Tree *t)
    {
        return CloneMode::Clone(t, this);
    }
    Tree *  Adjust(Tree *from, Tree *to)
    {
        return CloneMode::Adjust(from, to, this);
    }

public:
    // Do interface
    Tree *DoInteger(Integer *what)
    {
        return Adjust(what, new Integer(what->value, what->Position()));
    }
    Tree *DoReal(Real *what)
    {
        return Adjust(what, new Real(what->value, what->Position()));

    }
    Tree *DoText(Text *what)
    {
        return Adjust(what, new Text(what->value,
                                     what->opening, what->closing, 
                                     what->Position()));
    }
    Tree *DoName(Name *what)
    {
        return Adjust(what, new Name(what->value, what->Position()));
    }

    Tree *DoBlock(Block *what)
    {
        return Adjust(what, new Block(Clone(what->child),
                                      what->opening, what->closing,
                                      what->Position()));
    }
    Tree *DoInfix(Infix *what)
    {
        return Adjust(what, new Infix (what->name,
                                       Clone(what->left), Clone(what->right),
                                       what->Position()));
    }
    Tree *DoPrefix(Prefix *what)
    {
        return Adjust(what, new Prefix(Clone(what->left), Clone(what->right),
                                       what->Position()));
    }
    Tree *DoPostfix(Postfix *what)
    {
        return Adjust(what, new Postfix(Clone(what->left), Clone(what->right),
                                        what->Position()));
    }

};


struct DeepCloneMode
// ----------------------------------------------------------------------------
//   Clone mode where all the children nodes are copied (default)
// ----------------------------------------------------------------------------
{
    template<typename CloneClass>
    Tree *Clone(Tree *t, CloneClass *clone)
    {
        return t->Do(clone);
    }

    template<typename CloneClass>
    Tree *Adjust(Tree * /* from */, Tree *to, CloneClass */* clone */)
    {
        return to;
    }
};


struct ShallowCloneMode : DeepCloneMode
// ----------------------------------------------------------------------------
//   Shallow copy only create a new value for the top-level item
// ----------------------------------------------------------------------------
{
    template<typename CloneClass>
    Tree *Clone(Tree *t, CloneClass *) { return t; }
};


struct NullCloneMode : DeepCloneMode
// ----------------------------------------------------------------------------
//   Fill all children with NULL
// ----------------------------------------------------------------------------
{
    template<typename CloneClass>
    Tree *Clone(Tree *, CloneClass *) { return NULL; }
};


typedef struct TreeCloneTemplate<DeepCloneMode>         TreeClone;
typedef struct TreeCloneTemplate<ShallowCloneMode>      ShallowClone;
typedef struct TreeCloneTemplate<NullCloneMode>         NullClone;

inline Tree *elfe_deep_clone(Tree *input)
// ----------------------------------------------------------------------------
//   Perform a deep clone of a tree
// ----------------------------------------------------------------------------
{
    TreeClone clone;
    input = clone.Clone(input);
    return input;
}



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

ELFE_END

#endif // TREE_CLONE_H
