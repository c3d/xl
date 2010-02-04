#ifndef HASH_H
#define HASH_H

// ****************************************************************************
//  hash.h                                                    (C) Taodyne
//                                                            XL2 project
// ****************************************************************************
//
//   File Description:
//
//     Tools to run cryptographic hash functions over XL trees.
//     Hashing is a convenient way to verify the integrity of a tree.
//
//
//
//
//
// ****************************************************************************
// This program is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "tree.h"
#include "sha1.h"
#include <string.h>

XL_BEGIN

struct TreeHash : Action
// ----------------------------------------------------------------------------
//   Apply hash algorithm recursively on tree, updating each node's hash values
// ----------------------------------------------------------------------------
{
    TreeHash () {}
    Tree *DoInteger(Integer *what)
    {
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        sha1_write(&ctx, (byte *)&what->value, sizeof(what->value));
        sha1_final(&ctx);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        return what;
    }
    Tree *DoReal(Real *what)
    {
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        sha1_write(&ctx, (byte *)&what->value, sizeof(what->value));
        sha1_final(&ctx);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        return what;
    }
    Tree *DoText(Text *what)
    {
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        sha1_write(&ctx, (byte *)what->value.c_str(), what->value.length());
        sha1_final(&ctx);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        return what;
    }
    Tree *DoName(Name *what)
    {
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        sha1_write(&ctx, (byte *)what->value.c_str(), what->value.length());
        sha1_final(&ctx);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        return what;
   }

    Tree *DoBlock(Block *what)
    {
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        sha1_write(&ctx, (byte *)what->opening.c_str(), what->opening.length());
        what->child->Do(this);
        sha1_write(&ctx, (byte *)what->child->hash, HASH_SIZE);
        sha1_write(&ctx, (byte *)what->closing.c_str(), what->closing.length());
        sha1_final(&ctx);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        return what;
    }
    Tree *DoInfix(Infix *what)
    {
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        what->left->Do(this);
        sha1_write(&ctx, (byte *)what->left->hash, HASH_SIZE);
        what->right->Do(this);
        sha1_write(&ctx, (byte *)what->right->hash, HASH_SIZE);
        sha1_write(&ctx, (byte *)what->name.c_str(), what->name.length());
        sha1_final(&ctx);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        return what;
    }
    Tree *DoPrefix(Prefix *what)
    {
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        what->left->Do(this);
        sha1_write(&ctx, (byte *)what->left->hash, HASH_SIZE);
        what->right->Do(this);
        sha1_write(&ctx, (byte *)what->right->hash, HASH_SIZE);
        sha1_final(&ctx);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        return what;
    }
    Tree *DoPostfix(Postfix *what)
    {
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        what->left->Do(this);
        sha1_write(&ctx, (byte *)what->left->hash, HASH_SIZE);
        what->right->Do(this);
        sha1_write(&ctx, (byte *)what->right->hash, HASH_SIZE);
        sha1_final(&ctx);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        return what;
    }
    Tree *Do(Tree *what)
    {
        return NULL;
    }

};

XL_END

#endif // HASH_H
