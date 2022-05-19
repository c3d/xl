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
// (C) 2003-2004,2015,2019, Christophe de Dinechin <christophe@dinechin.org>
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

#include <iostream>
#include "tree.h"
#include "parser.h"


// ============================================================================
//
//    Output: Show a possibly parenthesized rendering of the tree
//
// ============================================================================

uint XLTree::outputIndent = 0;
bool XLTree::outputDebug = false;
nl_indent_t nl_indent;
indent_t indent;
unindent_t unindent;

void XLName::Output(ostream &out)
// ----------------------------------------------------------------------------
//   Emit the representation of a name
// ----------------------------------------------------------------------------
{
    if (outputDebug && value == text(""))
        out << "NULL-NAME";
    else
        out << value;
}

void XLBlock::Output(ostream &o)
// ----------------------------------------------------------------------------
//   Emit a parenthese operator, including indent
// ----------------------------------------------------------------------------
{
    if (outputDebug)
    {
        if (opening == INDENT_MARKER)
            o << "(BLOCK "
              << indent << nl_indent
              << *child << unindent << nl_indent
              << "BLOCK)";
        else
            o << "(PAREN " << opening << *child << closing << " PAREN)";
    }
    else
    {
        if (opening == INDENT_MARKER)
            o << indent << nl_indent << *child << unindent << nl_indent;
        else
            o << opening << *child << closing;
    }
}


void XLPrefix::Output(ostream &o)
// ----------------------------------------------------------------------------
//   Emit a prefix operator
// ----------------------------------------------------------------------------
{
    if (outputDebug)
    {
        if (left->Kind() == xlBLOCK)
            o << '[' << *left << *right << ']';
        else
            o << '[' << *left<<' '<< *right << ']';
    }
    else
    {
        if (left->Kind() == xlBLOCK)
            o << *left << *right;
        else
            o << *left<<' '<< *right;
    }
}


void XLInfix::Output(ostream &o)
// ----------------------------------------------------------------------------
//   Emit an infix operator, including the newline operator
// ----------------------------------------------------------------------------
{
    text seq = "\n";

    if (name == seq)
    {
        o << *left;
        XLTree *tail = right;
        text op;
        while (tail->Kind() == xlINFIX)
        {
            XLInfix *infix = (XLInfix *) tail;
            if (infix->name != seq)
                break;
            o << nl_indent << *infix->left;
            tail = infix->right;
        }
        o << nl_indent << *tail;
    }
    else
    {
        if (outputDebug)
            o<< '(' << *left << ' ' << name << ' ' << *right << ')';
        else
            o<< *left << ' ' << name << ' ' << *right;
    }
}


void debug(XLTree *tree)
// ----------------------------------------------------------------------------
//    Emit for debugging purpose
// ----------------------------------------------------------------------------
{
    if (tree)
        std::cerr << *tree;
}
