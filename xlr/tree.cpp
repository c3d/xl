// ****************************************************************************
//  tree.cpp                        (C) 1992-2003 Christophe de Dinechin (ddd)
//                                                            XL2 project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of the parse tree elements
//
//
//
//
//
//
//
//
// ****************************************************************************
// This program is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include <sstream>
#include <cassert>
#include <iostream>
#include "tree.h"
#include "context.h"
#include "renderer.h"
#include "opcodes.h"
#include "options.h"
#include "sha1.h"
#include "hash.h"
#include "errors.h"

XL_BEGIN

// ============================================================================
//
//    Class Tree
//
// ============================================================================

Tree::~Tree()
// ----------------------------------------------------------------------------
//   Delete the hash if we have one
// ----------------------------------------------------------------------------
{
    Info *next = NULL;
    assert (info != (Info *) 0xD00DEL && "Please report this in bug #922");
    for (Info *i = info; i; i = next)
    {
        next = i->next;
        i->Delete();
    }
    info = (Info *) 0xD00DEL;
}


Tree::operator text()
// ----------------------------------------------------------------------------
//   Conversion of a tree to text
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << this;
    return out.str();
}


int Tree::Compare(Tree *left, Tree *right, bool recurse)
// ----------------------------------------------------------------------------
//   Return true if two trees are equal
// ----------------------------------------------------------------------------
{
    if (left == right)
        return 0;
    if (!left)
        return -4;
    if (!right)
        return 4;

    kind lk = left->Kind();
    kind rk = right->Kind();
    if (lk != rk)
        return lk < rk ? -3 : 3;

    switch(lk)
    {
    case INTEGER:
    {
        Integer *li = (Integer *) left;
        Integer *ri = (Integer *) right;
        return li->value < ri->value ? -1 : li->value > ri->value ? 1 : 0;
    }
    case REAL:
    {
        Real *lr = (Real *) left;
        Real *rr = (Real *) right;
        return lr->value < rr->value ? -1 : lr->value > rr->value ? 1 : 0;
    }
    case TEXT:
    {
        Text *lt = (Text *) left;
        Text *rt = (Text *) right;
        if (lt->opening < rt->opening || lt->closing < rt->closing)
            return -2;
        if (lt->opening > rt->opening || lt->closing > rt->closing)
            return  2;
        return lt->value < rt->value ? -1 : lt->value > rt->value ? 1 : 0;
    }
    case NAME:
    {
        Name *ln = (Name *) left;
        Name *rn = (Name *) right;
        return ln->value < rn->value ? -1 : ln->value > rn->value ? 1 : 0;
    }
    case INFIX:
    {
        Infix *li = (Infix *) left;
        Infix *ri = (Infix *) right;
        if (li->name < ri->name)
            return -2;
        else if (li->name > ri->name)
            return 2;
        if (recurse)
        {
            if (int cmpLeft = Compare(li->left, ri->left))
                return cmpLeft;
            if (int cmpRight = Compare(li->right, ri->right))
                return cmpRight;
        }
        return 0;
    }
    case PREFIX:
    {
        Prefix *lp = (Prefix *) left;
        Prefix *rp = (Prefix *) right;
        if (recurse)
        {
            if (int cmpLeft = Compare(lp->left, rp->left))
                return cmpLeft;
            if (int cmpRight = Compare(lp->right, rp->right))
                return cmpRight;
        }
        return 0;
    }
    case POSTFIX:
    {
        Postfix *lp = (Postfix *) left;
        Postfix *rp = (Postfix *) right;
        if (recurse)
        {
            if (int cmpLeft = Compare(lp->left, rp->left))
                return cmpLeft;
            if (int cmpRight = Compare(lp->right, rp->right))
                return cmpRight;
        }
        return 0;
    }
    case BLOCK:
    {
        Block *lb = (Block *) left;
        Block *rb = (Block *) right;
        if (lb->opening < rb->opening || lb->closing < rb->closing)
            return -2;
        if (lb->opening > rb->opening || lb->closing > rb->closing)
            return  2;
        if (!recurse)
            return 0;
        return Compare(lb->child, rb->child);
    }
    }

    return 0;
}


void Tree::SetPosition(TreePosition pos, bool recurse)
// ----------------------------------------------------------------------------
//   Set the position for the tree and possibly its children
// ----------------------------------------------------------------------------
{
    Tree *tree = this;
    do
    {
        ulong kind = tree->Kind();
        tree->tag = (pos << KINDBITS) | kind;
        if (recurse)
        {
            switch(kind)
            {
            case INFIX:
                ((Infix *) tree)->left->SetPosition(pos, recurse);
                tree = ((Infix *) tree)->right;
                break;
            case PREFIX:
                ((Prefix *) tree)->left->SetPosition(pos, recurse);
                tree = ((Prefix *) tree)->right;
                break;
            case POSTFIX:
                ((Postfix *) tree)->right->SetPosition(pos, recurse);
                tree = ((Postfix *) tree)->left;
                break;
            case BLOCK:
                tree = ((Block *) tree)->child;
                break;
            default:
                recurse = false;
                break;
            }
        }
    } while(recurse);
}


text Block::indent   = "I+";
text Block::unindent = "I-";
text Text::textQuote = "\"";
text Text::charQuote = "'";

XL_END

