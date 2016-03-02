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
//  (C) 2010 Jérôme Forissier <jerome@taodyne.com>
//  (C) 2010 Catherine Burvelle <cathy@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "tree.h"
#include "action.h"

ELFE_BEGIN

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
    FindParentAction(Tree *self, ELFE::kind parentKind, bool):
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

    Tree_p      child;
    int         level;
    ELFE::kind parentKind;
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
            return NULL;
        depth += l;
        Tree * result = aChild->Do(this);
        depth -= l;
        if (result)
            path.append(subpath);
        return result;
    }

    Tree *DoInteger(Integer *)
    {
        return NULL;
    }
    Tree *DoReal(Real *)
    {
        return NULL;
    }
    Tree *DoText(Text *)
    {
        return NULL;
    }
    Tree *DoName(Name *)
    {
        return NULL;
    }

    Tree *DoPrefix(Prefix *what)
    {
        if (Name * n = what->left->AsName())
        {
            if (n->value == look)
                return what;
        }
        if (Tree *right = FindChild(what->right, "r", -1))
            return right;
        return NULL;
    }

    Tree *DoPostfix(Postfix *what)
    {
        if (Tree *left = FindChild(what->left, "l", -1))
            return left;
        return NULL;
    }

    Tree *DoInfix(Infix *what)
    {
        if (Tree *left = FindChild(what->left, "l", 0))
            return left;
        if (Tree *right = FindChild(what->right, "r", 0))
            return right;
        return NULL;
    }
    Tree *DoBlock(Block *what)
    {
        if (Tree *aChild = FindChild(what->child, "c", 0))
            return aChild;
        return NULL;
    }

    Tree *Do(Tree *)
    {
        return NULL;
    }

    text   look;
    int    depth;
    text   path;
};
ELFE_END

#endif // TREE_WALK_H
