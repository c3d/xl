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

XL_BEGIN

struct Tree;

typedef std::map<text, int>             priority_table;
typedef std::map<text, text>            delimiter_table;


class Syntax
// ----------------------------------------------------------------------------
//   This is the execution environment for all trees
// ----------------------------------------------------------------------------
{
public:
    Syntax():
        priority(0), default_priority(0),
        statement_priority(100), function_priority(200) {}
    Syntax(kstring filename):
        priority(0), default_priority(0),
        statement_priority(100), function_priority(200)
    {
        ReadSyntaxFile(filename);
    }

public:
    // Managing priorities
    int                 InfixPriority(text n);
    void                SetInfixPriority(text n, int p);
    int                 PrefixPriority(text n);
    void                SetPrefixPriority(text n, int p);
    int                 PostfixPriority(text n);
    void                SetPostfixPriority(text n, int p);

    // Read a complete syntax file (xl.syntax)
    void                ReadSyntaxFile (kstring filename);

    // Defining delimiters
    void                CommentDelimiter(text Begin, text End);
    void                TextDelimiter(text Begin, text End);
    void                BlockDelimiter(text Begin, text End);

    bool                IsComment(text Begin, text &end);
    bool                IsTextDelimiter(text Begin, text &end);
    bool                IsBlock(text Begin, text &end);
    bool                IsBlock(char Begin, text &end);

public:
    priority_table      infix_priority;
    priority_table      prefix_priority;
    priority_table      postfix_priority;
    delimiter_table     comment_delimiters;
    delimiter_table     text_delimiters;
    delimiter_table     block_delimiters;
    int                 priority;

    int                 default_priority;
    int                 statement_priority;
    int                 function_priority;

    static Syntax *     syntax;
};

XL_END

#endif // SYNTAX_H
