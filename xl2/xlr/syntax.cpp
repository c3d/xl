// ****************************************************************************
//  syntax.cpp                      (C) 1992-2009 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Description of syntax information used to parse XL trees
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
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "syntax.h"

// ============================================================================
// 
//    XLSyntax class: Syntax used to parse trees
// 
// ============================================================================

int XLSyntax::InfixPriority(text n)
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
    return default_priority;
}


void XLSyntax::SetInfixPriority(text n, int p)
// ----------------------------------------------------------------------------
//   Define the priority for a given infix operator
// ----------------------------------------------------------------------------
{
    if (p)
        infix_priority[n] = p;
}


int XLSyntax::PrefixPriority(text n)
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
    return default_priority;
}


void XLSyntax::SetPrefixPriority(text n, int p)
// ----------------------------------------------------------------------------
//   Define the priority for a given prefix operator
// ----------------------------------------------------------------------------
{
    if (p)
        prefix_priority[n] = p;
}


bool XLSyntax::IsComment(text Begin, text &End)
// ----------------------------------------------------------------------------
//   Check if something is in the comments table
// ----------------------------------------------------------------------------
{
    comment_table::iterator found = comments.find(Begin);
    if (found != comments.end())
    {
        End = found->second;
        return true;
    }
    return false;
}


bool XLSyntax::IsTextDelimiter(text Begin, text &End)
// ----------------------------------------------------------------------------
//    Check if something is in the text delimiters table
// ----------------------------------------------------------------------------
{
    comment_table::iterator found = text_delimiters.find(Begin);
    if (found != text_delimiters.end())
    {
        End = found->second;
        return true;
    }
    return false;
}


bool XLSyntax::IsBlock(text Begin, text &End)
// ----------------------------------------------------------------------------
//   Return true if we are looking at a block
// ----------------------------------------------------------------------------
{
    block_table::iterator found = blocks.find(Begin);
    if (found != blocks.end())
    {
        End = found->second;
        return true;
    }
    return false;
}
