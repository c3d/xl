#ifndef BFS_H
#define BFS_H
// ****************************************************************************
//  bfs.h                                                          XLR project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of the breadth-first search algorithm on a tree.
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
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************


#include <queue>
#include "tree.h"

XL_BEGIN

struct BreadthFirstSearch : Action
// ----------------------------------------------------------------------------
//   Execute ActionClass on a tree (whole or part), in breadth-first order
// ----------------------------------------------------------------------------
{
    BreadthFirstSearch (Action &action, bool fullScan = true):
        action(action), fullScan(fullScan) {}
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
    Tree *Do(Tree *what)
    {
        nodes.push(what);
        do
        {
            Block * bl; Infix * in; Prefix * pr; Postfix  * po;
            Tree * curr, * res;

            curr = nodes.front();
            res = curr->Do(action);
            if (!fullScan && res)
                return res;
            nodes.pop();
            if ((bl = curr->AsBlock()) != NULL)
            {
              nodes.push(bl->child);
            }
            if ((in = curr->AsInfix()) != NULL)
            {
              nodes.push(in->left);
              nodes.push(in->right);
            }
            if ((pr = curr->AsPrefix()) != NULL)
            {
              nodes.push(pr->left);
              nodes.push(pr->right);
            }
            if ((po = curr->AsPostfix()) != NULL)
            {
              nodes.push(po->left);
              nodes.push(po->right);
            }
        }
        while (!nodes.empty());
        return NULL;
    }

    Action & action;
    bool fullScan;
    std::queue<Tree *> nodes;
};

XL_END

#endif BFS_H
