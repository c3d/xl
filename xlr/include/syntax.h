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
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
// ****************************************************************************

#include <map>
#include <set>
#include "base.h"

XL_BEGIN

struct Tree;
struct Scanner;
struct ChildSyntax;

typedef std::map<text, int>             priority_table;
typedef std::map<text, text>            delimiter_table;
typedef std::map<text, ChildSyntax *>   subsyntax_table;
typedef std::set<text>                  token_set;


struct Syntax
// ----------------------------------------------------------------------------
//   Describe the syntax table (typically read from xl.syntax)
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
    Syntax(kstring name):
        priority(0), default_priority(0),
        statement_priority(100), function_priority(200)
    {
        ReadSyntaxFile(name);
    }
    Syntax(const Syntax &other);
    ~Syntax();

public:
    // Managing priorities
    int                 InfixPriority(text n);
    void                SetInfixPriority(text n, int p);
    int                 PrefixPriority(text n);
    void                SetPrefixPriority(text n, int p);
    int                 PostfixPriority(text n);
    void                SetPostfixPriority(text n, int p);
    bool                KnownToken(text n);
    bool                KnownPrefix(text n);

    // Read a complete syntax file (xl.syntax)
    void                ReadSyntaxFile (Scanner &scanner, uint indents = 1);
    void                ReadSyntaxFile (text filename, uint indents = 1);

    // Defining delimiters
    void                CommentDelimiter(text Begin, text End);
    void                TextDelimiter(text Begin, text End);
    void                BlockDelimiter(text Begin, text End);

    bool                IsComment(text Begin, text &end);
    bool                IsTextDelimiter(text Begin, text &end);
    bool                IsBlock(text Begin, text &end);
    bool                IsBlock(char Begin, text &end);
    Syntax *            HasSpecialSyntax(text Begin, text &end);

public:
    priority_table      infix_priority;
    priority_table      prefix_priority;
    priority_table      postfix_priority;
    delimiter_table     comment_delimiters;
    delimiter_table     text_delimiters;
    delimiter_table     block_delimiters;
    delimiter_table     subsyntax_file;
    subsyntax_table     subsyntax;
    token_set           known_tokens;
    token_set           known_prefixes;
    int                 priority;

    int                 default_priority;
    int                 statement_priority;
    int                 function_priority;

    static Syntax *     syntax;
};


struct ChildSyntax : Syntax
// ----------------------------------------------------------------------------
//   Child syntax of a top-level syntax
// ----------------------------------------------------------------------------
{
    ChildSyntax() : Syntax(), filename(), delimiters() {}
    ChildSyntax(text fn) : Syntax(fn.c_str()), filename(fn), delimiters() {}
    ChildSyntax(const ChildSyntax &other)
        : Syntax(other),
          filename(other.filename), delimiters(other.delimiters) {}
    text                filename;
    delimiter_table     delimiters;
};

XL_END

#endif // SYNTAX_H
