#ifndef TREE_WALK_H
#define TREE_WALK_H
// *****************************************************************************
// tree-walk.h                                                        XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2011, Catherine Burvelle <catherine@taodyne.com>
// (C) 2010,2014-2017,2019-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

#include "tree.h"
#include "action.h"

XL_BEGIN

struct FindParentAction : Action
// ------------------------------------------------------------------------
//   Find an ancestor of a node
// ------------------------------------------------------------------------
// "level" gives the depth of the parent. 0 means the node itself, 1 is the
// parent, 2 the grand-parent, etc...
// "path" is a string containing the path between the node and its ancestor:
// l means left, r means right and c means child of block.
// "fullpath" is the full path from the top to the node
{
    FindParentAction(Tree *self, uint level = 1):
        child(self), level(level), path(), fullpath(), useKind(false)
    // ------------------------------------------------------------------------
    //   Find the ancestor of a node with the specified depth
    // ------------------------------------------------------------------------
    {}
    FindParentAction(Tree *self, XL::kind parentKind, bool):
        child(self), level(1), parentKind(parentKind), path(), fullpath(),
        useKind(true)
    // ------------------------------------------------------------------------
    //   Find the first ancestor of a node with the specified kind
    // ------------------------------------------------------------------------
    {}

    Tree *FindParent(Tree *ancestor, Tree *aChild, kstring subpath)
    {
        if (Tree *result = aChild->Do(this))
        {
            fullpath.append(subpath);
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
            if (useKind)
            {
                if (parentKind == ancestor->Kind())
                    level = 0;
            }
            else
                level--;
            return ancestor;
        }
        // Nothing found on this path, return nullptr
        return nullptr;
    }

    Tree *Do(Integer *what)
    {
        if (child == what)
        {
            return what;
        }
        return nullptr;
    }
    Tree *Do(Real *what)
    {
        if (child == what)
        {
            return what;
        }
        return nullptr;
    }
    Tree *Do(Text *what)
    {
        if (child == what)
        {
            return what;
        }
        return nullptr;
    }
    Tree *Do(Name *what)
    {
        if (child == what)
        {
            return what;
        }
        return nullptr;
    }

    Tree *Do(Prefix *what)
    {
        if (child == what)
        {
            return what;
        }
        if (Tree *left = FindParent(what, what->left, "l"))
            return left;
        if (Tree *right = FindParent(what, what->right, "r"))
            return right;
        return nullptr;
    }

    Tree *Do(Postfix *what)
    {
        if (child == what)
        {
            return what;
        }
        if (Tree *left = FindParent(what, what->left, "l"))
            return left;
        if (Tree *right = FindParent(what, what->right, "r"))
            return right;
        return nullptr;
    }

    Tree *Do(Infix *what)
    {
        if (child == what)
        {
            return what;
        }
        if (Tree *left = FindParent(what, what->left, "l"))
            return left;
        if (Tree *right = FindParent(what, what->right, "r"))
            return right;
        return nullptr;
    }
    Tree *Do(Block *what)
    {
        if (child == what)
        {
            return what;
        }
        if (Tree *aChild = FindParent(what, what->child, "c"))
            return aChild;
        return nullptr;
    }

    Tree *Do(Tree *what)
    {
        if (child == what)
        {
            return what;
        }
        return nullptr;
    }

    Tree_p      child;
    int         level;
    XL::kind parentKind;
    text        path;
    text        fullpath;
    bool        useKind;
};


struct FindChildAction : Action
// ------------------------------------------------------------------------
//   Find a prefix child with given name -- Feature #553
// ------------------------------------------------------------------------
{
    FindChildAction(text what, uint depth = 1):
        look(what), depth(depth), path()
    {
    }

    Tree *FindChild(Tree *aChild, kstring subpath, int l)
    {

        if (depth +l <= 0)
            return nullptr;
        depth += l;
        Tree * result = aChild->Do(this);
        depth -= l;
        if (result)
            path.append(subpath);
        return result;
    }

    Tree *Do(Integer *)
    {
        return nullptr;
    }
    Tree *Do(Real *)
    {
        return nullptr;
    }
    Tree *Do(Text *)
    {
        return nullptr;
    }
    Tree *Do(Name *)
    {
        return nullptr;
    }

    Tree *Do(Prefix *what)
    {
        if (Name * n = what->left->AsName())
        {
            if (n->value == look)
                return what;
        }
        if (Tree *right = FindChild(what->right, "r", -1))
            return right;
        return nullptr;
    }

    Tree *Do(Postfix *what)
    {
        if (Tree *left = FindChild(what->left, "l", -1))
            return left;
        return nullptr;
    }

    Tree *Do(Infix *what)
    {
        if (Tree *left = FindChild(what->left, "l", 0))
            return left;
        if (Tree *right = FindChild(what->right, "r", 0))
            return right;
        return nullptr;
    }
    Tree *Do(Block *what)
    {
        if (Tree *aChild = FindChild(what->child, "c", 0))
            return aChild;
        return nullptr;
    }

    Tree *Do(Tree *)
    {
        return nullptr;
    }

    text   look;
    int    depth;
    text   path;
};
XL_END

#endif // TREE_WALK_H
