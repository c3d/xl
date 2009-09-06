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
#include "scanner.h"
#include "tree.h"
#include "errors.h"

XL_BEGIN

// ============================================================================
// 
//    Syntax used to parse trees
// 
// ============================================================================

Syntax *Syntax::syntax = NULL;

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


void Syntax::ReadSyntaxFile(kstring filename)
// ----------------------------------------------------------------------------
//   Parse the syntax description table
// ----------------------------------------------------------------------------
{
    enum State
    {
        inUnknown, inPrefix, inInfix, inPostfix,
        inComment, inCommentDef,
        inText, inTextDef,
        inBlock, inBlockDef
    };

    State       state = inUnknown;
    text        txt, entry;
    token_t     tok = tokNAME;
    int         priority = 0;
    Syntax      baseSyntax;
    Positions   basePositions;
    Errors      errors(&basePositions);
    Scanner     scanner(filename, baseSyntax, basePositions, errors);

    while(tok != tokEOF)
    {
        tok = scanner.NextToken(true);

        if (tok == tokSYMBOL)
        {
            text t = scanner.TextValue();
            uint i, len = t.length();
            for (i = 1; i < len; i++)
            {
                text sub = t.substr(0, i);
                known_tokens.insert(sub);
            }
        }

        switch(tok)
        {
        case tokEOF:
            break;
        case tokINTEGER:
            priority = scanner.IntegerValue();
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
            }
            break;
        default:
            break;
        }
    }
}

XL_END
