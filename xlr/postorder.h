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
