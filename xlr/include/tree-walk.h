#ifndef TREE_WALK_H
#define TREE_WALK_H
// ****************************************************************************
//  tree-walk.h                                                     Tao project
// ****************************************************************************
//
//   File Description:
//
//    Walking around a tree
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
//  (C) 2010 Jérôme Forissier <jerome@taodyne.com>
//  (C) 2010 Catherine Burvelle <cathy@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "tree.h"


XL_BEGIN

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

    Tree *FindParent(Tree *ancestor, Tree *aChild, kstring subpath)
    {
        if (Tree *result = aChild->Do(this))
        {
            if (level <= 0 )
            {
                // The requested parent is already identified
                // and it is the received result, so return it.
                return result;
            }
            // The ancestor is on the path between the child and the
            // requested parent. So add the subpath, decrement the
            // level and return the ancestor.
            path.append(subpath);
            level--;
            return ancestor;
        }
        // Nothing found on this path, return NULL
        return NULL;
    }

    Tree *DoInteger(Integer *what)
    {
        if (child == what)
        {
            return what;
        }
        return NULL;
    }
    Tree *DoReal(Real *what)
    {
        if (child == what)
        {
            return what;
        }
        return NULL;
    }
    Tree *DoText(Text *what)
    {
        if (child == what)
        {
            return what;
        }
        return NULL;
    }
    Tree *DoName(Name *what)
    {
        if (child == what)
        {
            return what;
        }
        return NULL;
    }

    Tree *DoPrefix(Prefix *what)
    {
        if (child == what)
        {
            return what;
        }
        if (Tree *left = FindParent(what, what->left, "l"))
            return left;
        if (Tree *right = FindParent(what, what->right, "r"))
            return right;
        return NULL;
    }

    Tree *DoPostfix(Postfix *what)
    {
        if (child == what)
        {
            return what;
        }
        if (Tree *left = FindParent(what, what->left, "l"))
            return left;
        if (Tree *right = FindParent(what, what->right, "r"))
            return right;
        return NULL;
    }

    Tree *DoInfix(Infix *what)
    {
        if (child == what)
        {
            return what;
        }
        if (Tree *left = FindParent(what, what->left, "l"))
            return left;
        if (Tree *right = FindParent(what, what->right, "r"))
            return right;
        return NULL;
    }
    Tree *DoBlock(Block *what)
    {
        if (child == what)
        {
            return what;
        }
        if (Tree *aChild = FindParent(what, what->child, "c"))
            return aChild;
        return NULL;
    }

    Tree *Do(Tree *what)
    {
        if (child == what)
        {
            return what;
        }
        return NULL;
    }

    Tree_p child;
    int    level;
    text   path;
};

XL_END

#endif // TREE_WALK_H
