#ifndef BFS_H
#define BFS_H
// *****************************************************************************
// include/bfs.h                                                      XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can r redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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
