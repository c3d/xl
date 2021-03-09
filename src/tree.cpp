// *****************************************************************************
// tree.cpp                                                           XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2003-2004,2009-2011,2014-2017,2019-2021, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2011, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
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
#include "context.h"
#include "renderer.h"
#include "options.h"
#include "errors.h"
#include "types.h"
#include "rewrites.h"
#include "main.h"

#include <sstream>
#include <cassert>
#include <iostream>

XL_BEGIN

// ============================================================================
//
//    Class Tree
//
// ============================================================================

TreePosition Tree::NOWHERE = Tree::UNKNOWN_POSITION;
kstring Tree::KindName[KIND_COUNT] =
// ----------------------------------------------------------------------------
//   The names of the tree kinds for debugging purpose
// ----------------------------------------------------------------------------
{
    "NATURAL", "REAL", "TEXT", "NAME",
    "BLOCK", "PREFIX", "POSTFIX", "INFIX"
};


Tree::~Tree()
// ----------------------------------------------------------------------------
//   Delete the tree and associated data
// ----------------------------------------------------------------------------
{
    Info *next = nullptr;
    assert (info != (Info *) 0xD00DEL && "Please report this in bug #922");
    for (Info *i = info; i; i = next)
    {
        XL_ASSERT(i->owner == this);
        next = i->next;
        i->Delete();
    }
}


Tree::operator text()
// ----------------------------------------------------------------------------
//   Conversion of a tree to standard text representation
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << this;
    return out.str();
}


int Tree::Compare(Tree *left, Tree *right, bool recurse)
// ----------------------------------------------------------------------------
//   Compare trees and return negative, null or positive relative order
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
    case NATURAL:
    {
        Natural *li = (Natural *) left;
        Natural *ri = (Natural *) right;
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
        text &lt = ln->value;
        text &rt = rn->value;
        return lt < rt ? -1 : lt > rt ? 1 : 0;
    }
    case INFIX:
    {
        Infix *li = (Infix *) left;
        Infix *ri = (Infix *) right;
        text &lt = li->name;
        text &rt = ri->name;
        if (lt < rt)
            return -2;
        else if (lt > rt)
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
        tree->tag = (pos << POSBITS) | kind;
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



// ============================================================================
//
//   Recursion-free tree iterator
//
// ============================================================================

Tree *Tree::Iterator::operator()()
// ----------------------------------------------------------------------------
//    Return the next tree, following a normal program structure
// ----------------------------------------------------------------------------
{
    while (current)
    {
        while (Block *block = current->AsBlock())
            current = block->child;
        if (Infix *seq = IsSequence(current))
        {
            current = seq->left;
            next.push_back(seq->right);
        }
        else
        {
            Tree *result = current;
            if (next.size())
            {
                current = next.back();
                next.pop_back();
            }
            else
            {
                current = nullptr;
            }
            return result;
        }
    }
    return current;
}



// ============================================================================
//
//   Case / underscore normalization
//
// ============================================================================

text Normalize(text spelling)
// ----------------------------------------------------------------------------
//   Normalize input according to XL rules
// ----------------------------------------------------------------------------
{
    if (Opt::caseSensitive)
        return spelling;
    text name;
    for (auto c : spelling)
        if (c != '_')
            name += tolower(c);
    return name;
}


void SourceTree(Tree *tree)
// ----------------------------------------------------------------------------
//    Mark source code with FROM_SOURCE to indicate we must compile it
// ----------------------------------------------------------------------------
{
    if (!tree)
        return;

    tree->MarkSourceTree();
    switch(tree->Kind())
    {
    case INFIX:
        SourceTree(((Infix *) tree)->left);
        SourceTree(((Infix *) tree)->right);
        break;
    case PREFIX:
        SourceTree(((Prefix *) tree)->left);
        SourceTree(((Prefix *) tree)->right);
        break;
    case POSTFIX:
        SourceTree(((Postfix *) tree)->left);
        SourceTree(((Postfix *) tree)->right);
        break;
    case BLOCK:
        SourceTree(((Block *) tree)->child);
        break;
    default:
        break;
    }
}



// ============================================================================
//
//    Garbage collection initialization
//
// ============================================================================

INIT_GC;

INIT_ALLOCATOR(Tree);
INIT_ALLOCATOR(Natural);
INIT_ALLOCATOR(Real);
INIT_ALLOCATOR(Text);
INIT_ALLOCATOR(Name);
INIT_ALLOCATOR(Block);
INIT_ALLOCATOR(Prefix);
INIT_ALLOCATOR(Postfix);
INIT_ALLOCATOR(Infix);

INIT_ALLOCATOR(Context);
INIT_ALLOCATOR(Scope);
INIT_ALLOCATOR(Scopes);
INIT_ALLOCATOR(Rewrite);
INIT_ALLOCATOR(Rewrites);
INIT_ALLOCATOR(Closure);

INIT_ALLOCATOR(Types);
INIT_ALLOCATOR(RewriteCalls);
INIT_ALLOCATOR(RewriteCandidate);


XL_END
