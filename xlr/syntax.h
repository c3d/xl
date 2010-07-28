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
#include <set>
#include "base.h"

XL_BEGIN

struct Tree;
struct Scanner;

typedef std::map<text, int>             priority_table;
typedef std::map<text, text>            delimiter_table;
typedef std::set<text>                  token_set;


class Syntax
// ----------------------------------------------------------------------------
//   This is the execution environment for all trees
// ----------------------------------------------------------------------------
{
public:
    Syntax():
        priority(0), default_priority(0),
        statement_priority(100), function_priority(200) {}
    Syntax(Scanner &scanner):
        priority(0), default_priority(0),
        statement_priority(100), function_priority(200)
    {
        ReadSyntaxFile(scanner);
    }
    Syntax(kstring name);

public:
    // Managing priorities
    int                 InfixPriority(text n);
    void                SetInfixPriority(text n, int p);
    int                 PrefixPriority(text n);
    void                SetPrefixPriority(text n, int p);
    int                 PostfixPriority(text n);
    void                SetPostfixPriority(text n, int p);
    bool                KnownToken(text n);

    // Read a complete syntax file (xl.syntax)
    void                ReadSyntaxFile (Scanner &scanner, uint indents = 1);

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
    token_set           known_tokens;
    int                 priority;

    int                 default_priority;
    int                 statement_priority;
    int                 function_priority;

    static Syntax *     syntax;
};

XL_END

#endif // SYNTAX_H
