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
//  (C) 2010 Jerome Forissier <jerome@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************


#include <queue>
#include "tree.h"

XL_BEGIN

template <typename Action>
struct BreadthFirstSearch
// ----------------------------------------------------------------------------
//   Execute ActionClass on a tree (whole or part), in breadth-first order
// ----------------------------------------------------------------------------
{
    BreadthFirstSearch (Action &action, bool fullScan = true):
        action(action), fullScan(fullScan) {}

    typedef typename Action::value_type value_type;

    value_type DoInteger(Integer *what)      { return Do(what); }
    value_type DoReal(Real *what)            { return Do(what); }
    value_type DoText(Text *what)            { return Do(what); }
    value_type DoName(Name *what)            { return Do(what); }
    value_type DoBlock(Block *what)          { return Do(what); }
    value_type DoInfix(Infix *what)          { return Do(what); }
    value_type DoPrefix(Prefix *what)        { return Do(what); }
    value_type DoPostfix(Postfix *what)      { return Do(what); }

    value_type Do(Tree *what)
    {
        nodes.push(what);
        do
        {
            Tree       *curr;
            value_type  res;

            curr = nodes.front();
            if (!curr)
            {
                nodes.pop();
                continue;
            }
            res = curr->Do(action);
            if (!fullScan && res)
                return res;
            nodes.pop();
            
            if (Block   *bl = curr->AsBlock())
            {
                nodes.push(bl->child);
            }
            else if (Infix *in = curr->AsInfix())
            {
                nodes.push(in->left);
                nodes.push(in->right);
            }
            else if (Prefix *pr = curr->AsPrefix())
            {
                nodes.push(pr->left);
                nodes.push(pr->right);
            }
            else if (Postfix *po = curr->AsPostfix())
            {
                nodes.push(po->left);
                nodes.push(po->right);
            }
        }
        while (!nodes.empty());

        return value_type();
    }

    Action & action;
    bool fullScan;
    std::queue<Tree_p> nodes;
};

XL_END

#endif // BFS_H
