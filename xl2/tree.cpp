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
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

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
        if (opening == '\t')
            o << "(BLOCK "
              << indent << nl_indent
              << *child << unindent << nl_indent
              << "BLOCK)";
        else
            o << "(PAREN " << opening << *child << closing << " PAREN)";
    }
    else
    {
        if (opening == '\t')
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
        XLTree *tail = right, *c1, *c2;
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
