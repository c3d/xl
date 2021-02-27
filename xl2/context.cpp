// *****************************************************************************
// context.cpp                                                        XL project
// *****************************************************************************
//
// File description:
//
//     Description of an execution context
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
// (C) 2003-2004,2015,2019-2020, Christophe de Dinechin <christophe@dinechin.org>
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

#include "context.h"

// ============================================================================
//
//    XLContext class : Context of execution for trees
//
// ============================================================================

int XLContext::InfixPriority(text n)
// ----------------------------------------------------------------------------
//   Return infix priority, which is either this or parent's
// ----------------------------------------------------------------------------
{
    if (infix_priority.count(n))
    {
        int p = infix_priority[n];
        if (p)
            return p;
    }
    if (parent)
    {
        int p = parent->InfixPriority(n);
        if (p)
            infix_priority[n] = p;
        return p;
    }
    return default_priority;
}


void XLContext::SetInfixPriority(text n, int p)
// ----------------------------------------------------------------------------
//   Define the priority for a given infix operator
// ----------------------------------------------------------------------------
{
    if (p)
        infix_priority[n] = p;
}


int XLContext::PrefixPriority(text n)
// ----------------------------------------------------------------------------
//   Return prefix priority, which is either this or parent's
// ----------------------------------------------------------------------------
{
    if (prefix_priority.count(n))
    {
        int p = prefix_priority[n];
        if (p)
            return p;
    }
    if (parent)
    {
        int p = parent->PrefixPriority(n);
        if (p)
            prefix_priority[n] = p;
        return p;
    }
    return default_priority;
}


void XLContext::SetPrefixPriority(text n, int p)
// ----------------------------------------------------------------------------
//   Define the priority for a given prefix operator
// ----------------------------------------------------------------------------
{
    if (p)
        prefix_priority[n] = p;
}


XLTree *XLContext::Find(text name)
// ----------------------------------------------------------------------------
//   Recursively find the name
// ----------------------------------------------------------------------------
{
    for (XLContext *s = this; s; s = s->Parent())
    {
        XLTree *found = s->Symbol(name);
        if (found)
            return found;
    }
    return NULL;
}


bool XLContext::IsComment(text Begin, text &End)
// ----------------------------------------------------------------------------
//   Check if something is in the comments table
// ----------------------------------------------------------------------------
{
    for (XLContext *c = this; c; c = c->parent)
    {
        comment_table::iterator found = c->comments.find(Begin);
        if (found != c->comments.end())
        {
            End = found->second;
            return true;
        }
    }
    return false;
}


bool XLContext::IsTextDelimiter(text Begin, text &End)
// ----------------------------------------------------------------------------
//    Check if something is in the text delimiters table
// ----------------------------------------------------------------------------
{
    for (XLContext *c = this; c; c = c->parent)
    {
        comment_table::iterator found = c->text_delimiters.find(Begin);
        if (found != c->text_delimiters.end())
        {
            End = found->second;
            return true;
        }
    }
    return false;
}


bool XLContext::IsBlock(text Begin, text &End)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    for (XLContext *c = this; c; c = c->parent)
    {
        block_table::iterator found = c->blocks.find(Begin);
        if (found != c->blocks.end())
        {
            End = found->second;
            return true;
        }
    }
    return false;
}
