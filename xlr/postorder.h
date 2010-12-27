#ifndef POSTORDER_H
#define POSTORDER_H
// ****************************************************************************
//  postorder.h                                                    XLR project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of the post-order traversal algorithm on a tree.
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


#include "tree.h"

XL_BEGIN

template <typename Action>
struct PostOrderTraversal
// ----------------------------------------------------------------------------
//   Execute action on a tree (whole or part), following the post-order algo
// ----------------------------------------------------------------------------
{
    PostOrderTraversal (Action &action, bool fullScan = true):
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
        ret = what->right->Do(this);
        if (!fullScan && ret)
            return ret;
        return what->Do(action);
    }
    value_type DoPrefix(Prefix *what)
    {
        value_type ret = what->left->Do(this);
        if (!fullScan && ret)
            return ret;
        ret = what->right->Do(this);
        if (!fullScan && ret)
            return ret;
        return what->Do(action);
    }
    value_type DoPostfix(Postfix *what)
    {
        value_type   ret;
        ret = what->left->Do(this);
        if (!fullScan && ret)
            return ret;
        ret = what->right->Do(this);
        if (!fullScan && ret)
            return ret;
        return what->Do(action);
    }

    Action & action;
    bool fullScan;
};

XL_END

#endif // POSTORDER_H
