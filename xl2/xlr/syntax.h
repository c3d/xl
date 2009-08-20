#ifndef SYNTAX_H
#define SYNTAX_H
// ****************************************************************************
//  syntax.h                        (C) 1992-2009 Christophe de Dinechin (ddd) 
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

#include <map>
#include "base.h"

struct XLTree;

typedef std::map<text, int>             priority_table;
typedef std::map<text, text>            comment_table;
typedef std::map<text, text>            text_delimiters_table;
typedef std::map<text, text>            block_table;


class XLSyntax
// ----------------------------------------------------------------------------
//   This is the execution environment for all trees
// ----------------------------------------------------------------------------
{
public:
    XLSyntax():
        priority(0), default_priority(0),
        statement_priority(100), function_priority(200) {}

public:
    // Managing priorities
    int                 InfixPriority(text n);
    void                SetInfixPriority(text n, int p);
    int                 PrefixPriority(text n);
    void                SetPrefixPriority(text n, int p);

    // Convenience notation to define priorities
    XLSyntax &         operator / (kstring opname)
    {
        SetInfixPriority(opname, priority);
        return *this;
    }
    XLSyntax &         operator / (int prio)
    {
        priority = prio;
        return *this;
    }
    XLSyntax &         operator | (kstring opname)
    {
        SetPrefixPriority(opname, priority);
        return *this;
    }
    XLSyntax &         operator | (int prio)
    {
        priority = prio;
        return *this;
    }
    XLSyntax &         Comment(text Begin, text End)
    {
        comments[Begin] = End;
        return *this;
    }
    XLSyntax &         TextDelimiter(text Begin, text End)
    {
        text_delimiters[Begin] = End;
        return *this;
    }
    bool                IsComment(text Begin, text &end);
    bool                IsTextDelimiter(text Begin, text &end);
    XLSyntax &         Block(text Begin, text End)
    {
        blocks[Begin] = End;
        return *this;
    }
    bool                IsBlock(text Begin, text &end);
    bool                IsBlock(char Begin, text &end)
    {
        return IsBlock(text(&Begin, 1), end);
    }

private:
    priority_table      infix_priority;
    priority_table      prefix_priority;
    comment_table       comments;
    text_delimiters_table text_delimiters;
    block_table         blocks;
    int                 priority;

public:
    int                 default_priority;
    int                 statement_priority;
    int                 function_priority;
};



#endif // SYNTAX_H
