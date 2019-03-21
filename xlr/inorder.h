#ifndef INORDER_H
#define INORDER_H
// *****************************************************************************
// inorder.h                                                          XL project
// *****************************************************************************
//
// File description:
//
//     Implementation of the in-order traversal algorithm on a tree.
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


#include "tree.h"

XL_BEGIN

template <typename Action>
struct InOrderTraversal
// ----------------------------------------------------------------------------
//   Execute action on a tree (whole or part), following the in-order algorithm
// ----------------------------------------------------------------------------
{
    InOrderTraversal (Action &action, bool fullScan = true):
        action(action), fullScan(fullScan) {}

    typedef typename Action::value_type value_type;

    value_type DoInteger(Integer *what)         { return what->Do(action); }
    value_type DoReal(Real *what)               { return what->Do(action); }
    value_type DoText(Text *what)               { return what->Do(action); }
    value_type DoName(Name *what)               { return what->Do(action); }

    value_type DoBlock(Block *what)
    {
        // REVISIT: Why do we need to test what->child? Test fail otherwise?!?
        value_type ret = value_type();
        if (what->child)
            ret = what->child->Do(this);
        if (!fullScan && ret)
            return ret;
        return what->Do(action);
    }
    value_type DoInfix(Infix *what)
    {    
        value_type ret = what->left->Do(this);
        if (!fullScan && ret)
            return ret;
        ret = what->Do(action);
        if (!fullScan && ret)
            return ret;
        return what->right->Do(this);
    }
    value_type DoPrefix(Prefix *what)
    {
        value_type ret = what->left->Do(this);
        if (!fullScan && ret)
            return ret;
        ret = what->Do(action);
        if (!fullScan && ret)
            return ret;
        return what->right->Do(this);
    }
    value_type DoPostfix(Postfix *what)
    {
        value_type ret = what->left->Do(this);
        if (!fullScan && ret)
            return ret;
        ret = what->Do(action);
        if (!fullScan && ret)
            return ret;
        return what->right->Do(this);
    }

public:
    Action & action;
    bool fullScan;
};

XL_END

#endif // INORDER_H
