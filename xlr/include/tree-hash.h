#ifndef TREE_HASH_H
#define TREE_HASH_H
// ****************************************************************************
//  tree-hash.h                                                     Tao project
// ****************************************************************************
// 
//   File Description:
// 
// 
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
//
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "tree.h"

XL_BEGIN

// ============================================================================
//
//    Hash key for tree rewrite
//
// ============================================================================
//  We use this hashing key to quickly determine if two trees "look the same"

struct RewriteKey
// ----------------------------------------------------------------------------
//   Compute a hashing key for a rewrite
// ----------------------------------------------------------------------------
{
    RewriteKey () {}
    typedef ulong value_type;

    ulong Hash(ulong id, text t)
    {
        ulong result = 0xC0DED;
        text::iterator p;
        for (p = t.begin(); p != t.end(); p++)
            result = (result * 0x301) ^ *p;
        return id | (result << 3);
    }
    ulong Hash(ulong id, ulong value)
    {
        return id | (value << 3);
    }

    ulong DoInteger(Integer *what)
    {
        return Hash(0, what->value);
    }
    ulong DoReal(Real *what)
    {
        return Hash(1, *((ulong *) &what->value));
    }
    ulong DoText(Text *what)
    {
        return Hash(2, what->value);
    }
    ulong DoName(Name *what)
    {
        return Hash(3, what->value);
    }

    ulong DoBlock(Block *what)
    {
        return Hash(4, what->opening + what->closing);
    }
    ulong DoInfix(Infix *what)
    {
        return Hash(5, what->name);
    }
    ulong DoPrefix(Prefix *what)
    {
        return Hash(6, what->left->Do(this));
    }
    ulong DoPostfix(Postfix *what)
    {
        return Hash(7, what->right->Do(this));
    }
};

XL_END

#endif // TREE_HASH_H

