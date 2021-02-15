#ifndef SYNTAX_H
#define SYNTAX_H
// *****************************************************************************
// syntax.h                                                           XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2003-2004,2009-2012,2014-2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
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

#include <map>
#include <set>
#include "base.h"

XL_BEGIN

struct Tree;
struct Scope;
struct Scanner;
struct ChildSyntax;

typedef std::map<text, int>             priority_table;
typedef std::map<text, text>            delimiter_table;
typedef std::map<text, ChildSyntax *>   subsyntax_table;
typedef std::set<text>                  token_set;

typedef Tree *(*eval_fn) (Scope *, Tree *);
typedef std::map<text, eval_fn>         importer_map;

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
    eval_fn             KnownImporter(text n);
    void                AddImporter(text n, eval_fn fn);

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
    importer_map        known_importers;
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
