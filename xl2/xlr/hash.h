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

struct TreeHashPruneAction : Action
// ----------------------------------------------------------------------------
//   Delete and reset all the hash pointers of a tree
// ----------------------------------------------------------------------------
{
#define ZAP(h) do { if ((h)) { delete[] (h); (h) = NULL; } } while (0)

    TreeHashPruneAction () {}
    Tree *DoInteger(Integer *what)
    {
        ZAP(what->hash);
        return what;
    }
    Tree *DoReal(Real *what)
    {
        ZAP(what->hash);
        return what;
    }
    Tree *DoText(Text *what)
    {
        ZAP(what->hash);
        return what;
    }
    Tree *DoName(Name *what)
    {
        ZAP(what->hash);
        return what;
    }

    Tree *DoBlock(Block *what)
    {
        ZAP(what->hash);
        what->child->Do(this);
        return what;
    }
    Tree *DoInfix(Infix *what)
    {
        ZAP(what->hash);
        what->left->Do(this);
        what->right->Do(this);
        return what;
    }
    Tree *DoPrefix(Prefix *what)
    {
        ZAP(what->hash);
        what->left->Do(this);
        what->right->Do(this);
        return what;
    }
    Tree *DoPostfix(Postfix *what)
    {
        ZAP(what->hash);
        what->left->Do(this);
        what->right->Do(this);
        return what;
    }
    Tree *Do(Tree *what)
    {
        return NULL;
    }

#undef ZAP
};

struct TreeHashAction : Action
// ----------------------------------------------------------------------------
//   Apply hash algorithm recursively on tree, updating each node's hash values
// ----------------------------------------------------------------------------
{
#define NEED_HASH(h) ((h) == NULL || (mode & Force) == Force)
#define ALLOC(h) do { if (!(h)) (h) = new byte[HASH_SIZE]; } while (0)

    enum Mode
    {
      Default = 0,
      Force = 1,   // Hash all nodes even those with a non-null hash
      Prune = 2,   // When done, clear hash values of children to save memory
      ForceAndPrune = 3,
    };

    TreeHashAction () : mode(0) {}
    TreeHashAction (int mode) : mode(mode) {}
    Tree *DoInteger(Integer *what)
    {
        if (!NEED_HASH(what->hash))
            return what;
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        sha1_write(&ctx, (byte *)&what->value, sizeof(what->value));
        sha1_final(&ctx);
        ALLOC(what->hash);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        return what;
    }
    Tree *DoReal(Real *what)
    {
        if (!NEED_HASH(what->hash))
            return what;
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        sha1_write(&ctx, (byte *)&what->value, sizeof(what->value));
        sha1_final(&ctx);
        ALLOC(what->hash);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        return what;
    }
    Tree *DoText(Text *what)
    {
        if (!NEED_HASH(what->hash))
            return what;
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        sha1_write(&ctx, (byte *)what->value.c_str(), what->value.length());
        sha1_final(&ctx);
        ALLOC(what->hash);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        return what;
    }
    Tree *DoName(Name *what)
    {
        if (!NEED_HASH(what->hash))
            return what;
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        sha1_write(&ctx, (byte *)what->value.c_str(), what->value.length());
        sha1_final(&ctx);
        ALLOC(what->hash);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        return what;
   }

    Tree *DoBlock(Block *what)
    {
        if (!NEED_HASH(what->hash))
            return what;
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        sha1_write(&ctx, (byte *)what->opening.c_str(), what->opening.length());
        if (NEED_HASH(what->child->hash))
            what->child->Do(this);
        sha1_write(&ctx, (byte *)what->child->hash, HASH_SIZE);
        sha1_write(&ctx, (byte *)what->closing.c_str(), what->closing.length());
        sha1_final(&ctx);
        ALLOC(what->hash);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        if ((mode & Prune) == Prune)
            what->child->Do(pruneAction);
        return what;
    }
    Tree *DoInfix(Infix *what)
    {
        if (!NEED_HASH(what->hash))
            return what;
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        if (NEED_HASH(what->left->hash))
            what->left->Do(this);
        sha1_write(&ctx, (byte *)what->left->hash, HASH_SIZE);
        if (NEED_HASH(what->right->hash))
            what->right->Do(this);
        sha1_write(&ctx, (byte *)what->right->hash, HASH_SIZE);
        sha1_write(&ctx, (byte *)what->name.c_str(), what->name.length());
        sha1_final(&ctx);
        ALLOC(what->hash);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        if ((mode & Prune) == Prune)
        {
            what->left->Do(pruneAction);
            what->right->Do(pruneAction);
        }
        return what;
    }
    Tree *DoPrefix(Prefix *what)
    {
        if (what->hash && (mode & Force) != Force)
            return what;
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        if (NEED_HASH(what->left->hash))
            what->left->Do(this);
        sha1_write(&ctx, (byte *)what->left->hash, HASH_SIZE);
        if (NEED_HASH(what->right->hash))
            what->right->Do(this);
        sha1_write(&ctx, (byte *)what->right->hash, HASH_SIZE);
        sha1_final(&ctx);
        ALLOC(what->hash);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        if ((mode & Prune) == Prune)
        {
            what->left->Do(pruneAction);
            what->right->Do(pruneAction);
        }
        return what;
    }
    Tree *DoPostfix(Postfix *what)
    {
        if (what->hash && (mode & Force) != Force)
            return what;
        kind k = what->Kind();
        HASH_CONTEXT ctx;
        sha1_init(&ctx);
        sha1_write(&ctx, (byte *)&k, sizeof(k));
        if (NEED_HASH(what->left->hash))
            what->left->Do(this);
        sha1_write(&ctx, (byte *)what->left->hash, HASH_SIZE);
        if (NEED_HASH(what->right->hash))
            what->right->Do(this);
        sha1_write(&ctx, (byte *)what->right->hash, HASH_SIZE);
        sha1_final(&ctx);
        ALLOC(what->hash);
        memcpy(what->hash, sha1_read(&ctx), HASH_SIZE);
        if ((mode & Prune) == Prune)
        {
            what->left->Do(pruneAction);
            what->right->Do(pruneAction);
        }
        return what;
    }
    Tree *Do(Tree *what)
    {
        return NULL;
    }

    int mode;
    TreeHashPruneAction pruneAction;

#undef NEED_HASH
#undef ALLOC
};

XL_END

#endif // HASH_H
