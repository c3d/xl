// *****************************************************************************
// syntax.cpp                                                         XL project
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
// (C) 2003-2004,2009-2011,2014-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
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

#include "syntax.h"
#include "scanner.h"
#include "tree.h"
#include "errors.h"
#include "main.h"

XL_BEGIN

// ============================================================================
//
//    Syntax used to parse trees
//
// ============================================================================

Syntax *Syntax::syntax = nullptr;


Syntax::Syntax(const Syntax &other)
// ----------------------------------------------------------------------------
//   Copy from another syntax
// ----------------------------------------------------------------------------
    : infix_priority(other.infix_priority),
      prefix_priority(other.prefix_priority),
      postfix_priority(other.postfix_priority),
      comment_delimiters(other.comment_delimiters),
      text_delimiters(other.text_delimiters),
      block_delimiters(other.block_delimiters),
      subsyntax_file(other.subsyntax_file),
      subsyntax(other.subsyntax),
      known_tokens(other.known_tokens),
      known_prefixes(other.known_prefixes),
      known_importers(other.known_importers),
      known_sugar(other.known_sugar),
      priority(other.priority),
      default_priority(other.default_priority),
      statement_priority(other.statement_priority),
      function_priority(other.function_priority)
{
    for (subsyntax_table::iterator i=subsyntax.begin();i!=subsyntax.end();i++)
        (*i).second = new ChildSyntax(*(*i).second);
}


Syntax::~Syntax()
// ----------------------------------------------------------------------------
//   Destroy the children syntaxen
// ----------------------------------------------------------------------------
{
    for (subsyntax_table::iterator i=subsyntax.begin();i!=subsyntax.end();i++)
        delete (*i).second;
}


int Syntax::InfixPriority(text n)
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


void Syntax::SetInfixPriority(text n, int p)
// ----------------------------------------------------------------------------
//   Define the priority for a given infix operator
// ----------------------------------------------------------------------------
{
    if (p)
        infix_priority[n] = p;
}


int Syntax::PrefixPriority(text n)
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


void Syntax::SetPrefixPriority(text n, int p)
// ----------------------------------------------------------------------------
//   Define the priority for a given prefix operator
// ----------------------------------------------------------------------------
{
    if (p)
        prefix_priority[n] = p;
}


int Syntax::PostfixPriority(text n)
// ----------------------------------------------------------------------------
//   Return postfix priority, which is either this or parent's
// ----------------------------------------------------------------------------
{
    if (postfix_priority.count(n))
    {
        int p = postfix_priority[n];
        if (p)
            return p;
    }
    return default_priority;
}


void Syntax::SetPostfixPriority(text n, int p)
// ----------------------------------------------------------------------------
//   Define the priority for a given postfix operator
// ----------------------------------------------------------------------------
{
    if (p)
        postfix_priority[n] = p;
}


bool Syntax::KnownToken(text n)
// ----------------------------------------------------------------------------
//   Check if the given symbol is known in any of the priority tables
// ----------------------------------------------------------------------------
{
    return known_tokens.count(n) > 0;
}


bool Syntax::KnownPrefix(text n)
// ----------------------------------------------------------------------------
//   Check if the given symbol is a known prefix to a possible token
// ----------------------------------------------------------------------------
{
    return known_prefixes.count(n) > 0;
}


eval_fn Syntax::KnownImporter(text n)
// ----------------------------------------------------------------------------
//   Check if the given symbol is a known importer
// ----------------------------------------------------------------------------
//   An importer is a prefix like [import] or [use] that must be processed
//   during parsing, declaration scanning and evaluation.
{
    auto found = known_importers.find(n);
    if (found != known_importers.end())
        return (*found).second;
    return nullptr;
}


void Syntax::AddImporter(text n, eval_fn callback)
// ----------------------------------------------------------------------------
//    Add an importer prefix
// ----------------------------------------------------------------------------
{
    known_importers[n] = callback;
}


eval_fn Syntax::KnownSugar(text n)
// ----------------------------------------------------------------------------
//   Check if the given symbol is a known sugar prefix
// ----------------------------------------------------------------------------
//   A sugar is a quasi-no-op prefix that can be put in patterns.
//   For example, [to Copy is ...] being the same as [Copy is ...]
{
    auto found = known_sugar.find(n);
    if (found != known_sugar.end())
        return (*found).second;
    return nullptr;
}


void Syntax::AddSugar(text n, eval_fn callback)
// ----------------------------------------------------------------------------
//   Add a sugar prefix
// ----------------------------------------------------------------------------
{
    known_sugar[n] = callback;
}


void Syntax::SetDefaultSugar(eval_fn sugar)
// ----------------------------------------------------------------------------
//   Set the defaut sugar evaluation function
// ----------------------------------------------------------------------------
{
    default_sugar = sugar;
}


void Syntax::CommentDelimiter(text Begin, text End)
// ----------------------------------------------------------------------------
//   Define comment syntax
// ----------------------------------------------------------------------------
{
    comment_delimiters[Begin] = End;
}


void Syntax::TextDelimiter(text Begin, text End)
// ----------------------------------------------------------------------------
//   Define comment syntax
// ----------------------------------------------------------------------------
{
    text_delimiters[Begin] = End;
}


void Syntax::BlockDelimiter(text Begin, text End)
// ----------------------------------------------------------------------------
//   Define comment syntax
// ----------------------------------------------------------------------------
{
    block_delimiters[Begin] = End;
}


bool Syntax::IsComment(text Begin, text &End)
// ----------------------------------------------------------------------------
//   Check if something is in the comments table
// ----------------------------------------------------------------------------
{
    delimiter_table::iterator found = comment_delimiters.find(Begin);
    if (found != comment_delimiters.end())
    {
        End = found->second;
        return true;
    }
    return false;
}


bool Syntax::IsTextDelimiter(text Begin, text &End)
// ----------------------------------------------------------------------------
//    Check if something is in the text delimiters table
// ----------------------------------------------------------------------------
{
    delimiter_table::iterator found = text_delimiters.find(Begin);
    if (found != text_delimiters.end())
    {
        End = found->second;
        return true;
    }
    return false;
}


bool Syntax::IsBlock(text Begin, text &End)
// ----------------------------------------------------------------------------
//   Return true if we are looking at a block
// ----------------------------------------------------------------------------
{
    delimiter_table::iterator found = block_delimiters.find(Begin);
    if (found != block_delimiters.end())
    {
        End = found->second;
        return true;
    }
    return false;
}


bool Syntax::IsBlock(char Begin, text &End)
// ----------------------------------------------------------------------------
//   Return true if we are looking at a block
// ----------------------------------------------------------------------------
{
    return IsBlock(text(&Begin, 1), End);
}


Syntax *Syntax::HasSpecialSyntax(text Begin, text &End)
// ----------------------------------------------------------------------------
//   Returns a child syntax if any is applicable
// ----------------------------------------------------------------------------
{
    // Find associated syntax file
    delimiter_table::iterator found = subsyntax_file.find(Begin);
    if (found == subsyntax_file.end())
        return nullptr;

    // Find associated child syntax
    text filename = (*found).second;
    ChildSyntax *child = subsyntax[filename];
    assert(child && child->filename == filename);

    // Find delimiters in that child syntax
    delimiter_table::iterator dfound = child->delimiters.find(Begin);
    if (dfound == child->delimiters.end())
        return nullptr;            // Defensive codeing, should not happen

    // Success, return the syntax we found
    End = (*dfound).second;
    return child;
}


void Syntax::ReadSyntaxFile(Scanner &scanner, uint indents)
// ----------------------------------------------------------------------------
//   Parse the syntax description table
// ----------------------------------------------------------------------------
{
    enum State
    {
        inUnknown, inSugar, inPrefix, inInfix, inPostfix,
        inComment, inCommentDef,
        inText, inTextDef,
        inBlock, inBlockDef,
        inSyntaxName, inSyntax, inSyntaxDef,
    };

    State       state = inUnknown;
    text        txt, entry;
    token_t     tok = tokNAME;
    int         priority = 0;
    bool        done = false;
    ChildSyntax *childSyntax = nullptr;

    while(tok != tokEOF && !done)
    {
        tok = scanner.NextToken(true);

        if (tok == tokSYMBOL || state >= inComment)
        {
            text t = scanner.TextValue();
            uint i, len = t.length();
            for (i = 1; i < len; i++)
            {
                text sub = t.substr(0, i);
                known_prefixes.insert(sub);
            }
            known_tokens.insert(t);
        }

        switch(tok)
        {
        case tokEOF:
            break;
        case tokNATURAL:
            priority = scanner.NaturalValue();
            break;
        case tokINDENT:
        case tokPAROPEN:
            indents++;
            break;
        case tokUNINDENT:
        case tokPARCLOSE:
            indents--;
            if (!indents)
                done = true;
            break;
        case tokNAME:
        case tokSYMBOL:
        case tokSTRING:
        case tokQUOTE:
            txt = scanner.TextValue();

            if (txt == "NEWLINE")
                txt = "\n";
            else if (txt == "INDENT")
                txt = Block::indent;
            else if (txt == "UNINDENT")
                txt = Block::unindent;

            if (txt == "INFIX")
                state = inInfix;
            else if (txt == "SUGAR")
                state = inSugar;
            else if (txt == "PREFIX")
                state = inPrefix;
            else if (txt == "POSTFIX")
                state = inPostfix;
            else if (txt == "BLOCK")
                state = inBlock;
            else if (txt == "COMMENT")
                state = inComment;
            else if (txt == "TEXT")
                state = inText;
            else if (txt == "SYNTAX")
                state = inSyntaxName;

            else if (txt == "STATEMENT")
                statement_priority = priority;
            else if (txt == "FUNCTION")
                function_priority = priority;
            else if (txt == "DEFAULT")
                default_priority = priority;

            else switch(state)
            {
            case inUnknown:
                break;
            case inSugar:
                known_sugar[txt] = default_sugar;
                // Fallthrough on purpose
            case inPrefix:
                prefix_priority[txt] = priority;
                break;
            case inPostfix:
                postfix_priority[txt] = priority;
                break;
            case inInfix:
                infix_priority[txt] = priority;
                break;
            case inComment:
                entry = txt;
                state = inCommentDef;
                break;
            case inCommentDef:
                comment_delimiters[entry] = txt;
                state = inComment;
                break;
            case inText:
                entry = txt;
                state = inTextDef;
                break;
            case inTextDef:
                text_delimiters[entry] = txt;
                state = inText;
                break;
            case inBlock:
                entry = txt;
                state = inBlockDef;
                infix_priority[entry] = priority;
                break;
            case inBlockDef:
                block_delimiters[entry] = txt;
                block_delimiters[txt] = "";
                infix_priority[txt] = priority;
                state = inBlock;
                break;
            case inSyntaxName:
                if (txt.find(".syntax") == txt.npos)
                    txt += ".syntax";
                if (txt.find("/") == txt.npos)
                    txt = MAIN->SearchLibFile(txt);
                childSyntax = subsyntax[txt];
                if (!childSyntax)
                {
                    childSyntax = new ChildSyntax(txt);
                    subsyntax[txt] = childSyntax;
                }
                state = inSyntax;
                break;
            case inSyntax:
                entry = txt;
                state = inSyntaxDef;
                break;
            case inSyntaxDef:
                childSyntax->delimiters[entry] = txt;
                subsyntax_file[entry] = childSyntax->filename;
                state = inSyntax;
                break;
            }
            break;
        default:
            break;
        }
    }
}


void Syntax::ReadSyntaxFile (text filename, uint indents)
// ----------------------------------------------------------------------------
//   Read a syntax directly from a syntax file
// ----------------------------------------------------------------------------
{
    Syntax    baseSyntax;
    Positions basePositions;
    Errors    errors;
    Scanner   scanner(filename.c_str(), baseSyntax, basePositions, errors);
    ReadSyntaxFile(scanner, indents);
}

XL_END
