// ****************************************************************************
//  parser.cpp                      (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                            Activity project 
// ****************************************************************************
// 
//   File Description:
// 
//     XL language parser
// 
//      See detailed description in parser.h
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

#include <vector>
#include <stdio.h>
#include "parser.h"
#include "scanner.h"
#include "errors.h"
#include "tree.h"


XL_BEGIN
// ============================================================================
// 
//    Parser class itself
// 
// ============================================================================

struct Pending
// ----------------------------------------------------------------------------
//   Pending expression while parsing
// ----------------------------------------------------------------------------
{
    Pending(text o, Tree *a, int p, ulong pos):
        opcode(o), argument(a), priority(p), position(pos) {}
    text   opcode;
    Tree  *argument;
    int    priority;
    ulong  position;
};


static inline text ErrorNameOf(text what)
// ----------------------------------------------------------------------------
//   Return a visible name for special characters
// ----------------------------------------------------------------------------
{
    if (what == "\n")
        return "<newline>";
    if (what == "\t")
        return "<tab>";
    if (what == "I+")
        return "<indent>";
    if (what == "I-")
        return "<unindent>";
    return what;
}


token_t Parser::NextToken()
// ----------------------------------------------------------------------------
//    Return the next token, skipping comments and gathering long text
// ----------------------------------------------------------------------------
{
    while (true)
    {
        token_t pend = pending;
        if (pend != tokNONE && pend != tokNEWLINE)
        {
            pending = tokNONE;
            return pend;
        }

        // Here, there's nothing pending or only a newline
        token_t result = scanner.NextToken();
        text opening, closing;
        switch(result)
        {
        case tokNAME:
        case tokSYMBOL:
            opening = scanner.TokenText();
            if (syntax.IsComment(opening, closing))
            {
                // Skip comments, keep looking to get the right indentation
                text comment = scanner.Comment(closing);
                if (closing == "\n" && pend == tokNONE)
                    pending = tokNEWLINE;
                continue;
            }
            else if (syntax.IsTextDelimiter(opening, closing))
            {
                text longText = scanner.Comment(closing);
                ulong tLen = closing.length();
                longText.erase(longText.length() - tLen, tLen);
                scanner.SetTextValue(longText);
                if (pend == tokNEWLINE)
                {
                    pending = tokSTRING;
                    return tokNEWLINE;
                }
                if (closing == "\n" && pend == tokNONE)
                    pending = tokNEWLINE;
                return tokSTRING;
            }

            // If the next token has a substatement infix priority,
            // this takes over any pending newline. Example: else
            if (pend == tokNEWLINE)
            {
                int prefixPrio = syntax.PrefixPriority(opening);
                if (prefixPrio == syntax.default_priority)
                {
                    int infixPrio = syntax.InfixPriority(opening);
                    if (infixPrio < syntax.statement_priority)
                        pending = pend = tokNONE;
                }    
            }
            break;
        case tokNEWLINE:
            // Combined with any previous pending indent
            pending = tokNEWLINE;
            continue;
        case tokUNINDENT:
            // Add newline if no infix
            pending = tokNEWLINE;
            return result;
        case tokINDENT:
            // Ignore pending newline when indenting
            pending = tokNONE;
            return result;
        default:
            break;
        } // switch (result)

        // If we have another token here and a pending newline, push
        // the other token back.
        if (pend != tokNONE)
        {
            pending = result;
            return pend;
        }

        return result;
    } // While loop
}


Tree *Parser::Parse(text closing)
// ----------------------------------------------------------------------------
//   Parse input
// ----------------------------------------------------------------------------
/* XL parsing is not very difficult, but a bit unusual, because it is based
   solely on dynamic information and not, for instance, on keywords.
   Consider the following cases, where p is "prefix-op" and i is "infix-op"
     Write A
       Parses as p(Write,A).
     A and B
       Parses as i(and,A,B) if 'and' has a priority,
              as p(A,p(and,B)) otherwise
     Write -A,B
       This parses as (Write-A),B since "-" has a priority.
       I wish I could fix that one...
       The correct XL syntax is: Write (-A),B
       We hope that semantic will catch such a case later and let us know...
 */
{
    Tree *               result             = NULL;
    Tree *               left               = NULL;
    Tree *               right              = NULL;
    text                 prefix             = "";
    bool                 done               = false;
    int                  default_priority   = syntax.default_priority;
    int                  function_priority  = syntax.function_priority;
    int                  statement_priority = syntax.statement_priority;
    int                  result_priority    = default_priority;
    int                  prefix_priority    = 0;
    int                  postfix_priority   = 0;
    int                  infix_priority     = 0;
    int                  paren_priority     = syntax.InfixPriority(closing);
    bool                 is_expression      = false;
    bool                 new_statement      = true;
    bool                 line_continuation  = false;
    ulong                pos                = 0;
    uint                 extra_indents      = 0;
    uint                 old_indent         = 0;
    text                 infix, name, spelling;
    text                 comment_end;
    token_t              tok;
    text                 blk_opening, blk_closing;
    std::vector<Pending> stack;

    // When inside a () block, we are in 'expression' mode right away
    if (closing != "" && paren_priority > statement_priority)
    {
        new_statement = false;
        is_expression = true;
    }

    while (!done)
    {
        // Scan next token
        right = NULL;
        prefix_priority = infix_priority = default_priority;
        tok = NextToken();

        // Check if we are dealing with a trailing operator (at end of line)
        if (line_continuation)
        {
            if (tok == tokINDENT)
            {
                extra_indents++;
                tok = tokNEWLINE;
            }
        }
        else if (extra_indents > 0)
        {
            if (tok == tokUNINDENT)
            {
                extra_indents--;
                tok = tokNEWLINE;
            }
        }

        // Check token result
        pos = scanner.Position();
        switch(tok)
        {
        case tokEOF:
        case tokERROR:
            done = true;
            if (closing != "" && closing != Block::unindent)
                errors.Error("Unexpected end of text, expected '$1'",
                             scanner.Position(), closing);
            break;
        case tokINTEGER:
            right = new Integer(scanner.IntegerValue(), pos);
            prefix_priority = function_priority;
            break;
        case tokREAL:
            right = new Real(scanner.RealValue(), pos);
            prefix_priority = function_priority;
            break;
        case tokSTRING:
        case tokQUOTE:
            char separator = scanner.TokenText()[0];
            name = text(1, separator);
            right = new Text(scanner.TextValue(), name, name, pos);
            if (!result && new_statement)
                is_expression = false;
            break;
        case tokNAME:
        case tokSYMBOL:
            name = scanner.TokenText();
            spelling = scanner.TextValue();
            if (!result)
            {
                prefix_priority = syntax.PrefixPriority(name);
                right = new Name(spelling, pos);
                if (prefix_priority == default_priority)
                    prefix_priority = function_priority;
                if (new_statement && tok == tokNAME)
                    is_expression = false;
            }
            else if (left)
            {
                // This is the right of an infix operator
                // If we have "A and not B", where "not" has
                // higher priority than "and", we want to
                // parse this as "A and (not B)" rather than as
                // "(A and not) B"
                prefix_priority = syntax.PrefixPriority(name);
                right = new Name(spelling, pos);
                if (prefix_priority == default_priority)
                    prefix_priority = function_priority;
            }
            else
            {
                // Complicated case: need to discriminate infix and prefix
                infix_priority = syntax.InfixPriority(name);
                if (infix_priority != default_priority)
                {
                    // We got an infix
                    left = result;
                    infix = name;
                }
                else
                {
                    postfix_priority = syntax.PostfixPriority(name);
                    if (postfix_priority != default_priority)
                    {
                        // We have a postfix operator
                        right = new Name(spelling, pos);
                        right = new Postfix(result, right, pos);
                        prefix_priority = postfix_priority;
                        result = NULL;
                    }
                    else
                    {
                        // No priority: take this as a prefix by default
                        prefix_priority = syntax.PrefixPriority(name);
                        right = new Name(spelling, pos);
                        if (prefix_priority == default_priority)
                        {
                            prefix_priority = function_priority;
                            if (new_statement && tok == tokNAME)
                                is_expression = false;
                        }
                    }
                }
            }
            break;
        case tokNEWLINE:
            // Consider new-line as an infix operator
            infix = "\n";
            name = infix;
            infix_priority = syntax.InfixPriority(infix);
            left = result;
            break;
        case tokPARCLOSE:
            // Check for mismatched parenthese here
            if (scanner.TokenText() != closing)
                errors.Error("Mismatched parentheses: got '$1', expected '$2'",
                             pos, scanner.TokenText(), closing);
            done = true;
            break;
        case tokUNINDENT:
            // Check for mismatched blocks here
            if (closing != Block::unindent)
                errors.Error("Mismatched identation, expected '$1'",
                             pos, closing);
            done = true;
            break;
        case tokINDENT:
            scanner.SetTokenText(Block::indent);
            // Fall-through
        case tokPAROPEN:
            blk_opening = scanner.TokenText();
            if (!syntax.IsBlock(blk_opening, blk_closing))
                errors.Error("Unknown parenthese type: '$1' (internal)",
                             pos, blk_opening);
            if (tok == tokPAROPEN)
                old_indent = scanner.OpenParen();
            name = blk_opening;
            paren_priority = syntax.InfixPriority(name);

            // Just like for names, parse the contents of the parentheses
            prefix_priority = paren_priority;
            infix_priority = default_priority;
            right = Parse(blk_closing);
            if (tok == tokPAROPEN)
                scanner.CloseParen(old_indent);
            if (!right)
                right = new Name("", pos); // Case where we have ()
            right = new Block(right, blk_opening, blk_closing, pos);
            break;
        default:
            if (true)
            {
                char buffer[20];
                sprintf(buffer, "%u", tok);
                errors.Error("Internal error: unknown token $1 ($2)",
                             pos, scanner.TokenText(), buffer);
            }
            break;
        } // switch(tok)

        // Check what is the current result
        line_continuation = false;
        if (!result)
        {
            // First thing we parse
            result = right;
            result_priority = prefix_priority;

            // We are now in the middle of an expression
            if (result && result_priority >= statement_priority)
                new_statement= false;
        }
        else if (left)
        {
            // Check if we had a statement separator
            if (infix_priority < statement_priority)
            {
                new_statement = true;
                is_expression = true;
            }

            // We got left and infix-op, we are now looking for right
            // If we have 'A and not B', where 'not' has higher priority
            // than 'and', we want to finish parsing 'not B' first, rather
            // than seeing this as '(A and not) B'.
            if (prefix_priority != default_priority)
            {
                // Push "A and" in the above example
                ulong st_pos = new_statement ? left->Position() : pos;
                stack.push_back(Pending(infix, left, infix_priority, st_pos));
                if (infix_priority > default_priority)
                    line_continuation = true;
                left = NULL;

                // Start over with "not"
                result = right;
                result_priority = prefix_priority;
            }
            else
            {
                while (stack.size())
                {
                    Pending &prev = stack.back();

                    // Check priorities compared to stack
                    // A + B * C, we got '*': keep "A+..." on stack
                    // Odd priorities are made right-associative by
                    // turning the low-bit off in the comparison below
                    if (!done &&
                        prev.priority != default_priority &&
                        infix_priority > (prev.priority & ~1))
                        break;
                    if (prev.opcode == prefix)
                        left = new Prefix(prev.argument, left, prev.position);
                    else
                        left = new Infix(prev.opcode, prev.argument, left,
                                         prev.position);
                    stack.pop_back();
                }

                // Now, we want to restart with the rightmost operand
                if (done)
                {
                    // End of text: the result is what we just got
                    result = left;
                }
                else
                {
                    // Something like A+B+C, just got second +
                    ulong st_pos = new_statement ? left->Position() : pos;
                    stack.push_back(Pending(infix,left,infix_priority,st_pos));
                    if (infix_priority > default_priority)
                        line_continuation= true;
                    result = NULL;
                }
                left = NULL;
            }
        }
        else if (right)
        {
            // Check if we had a low-priority prefix (e.g. pragmas)
            if (prefix_priority < statement_priority)
            {
                new_statement = true;
                is_expression = true;
            }

            // Check priorities for something like "A.B x,y" -> "(A.B) (x,y)"
            // Odd priorities are made right associative by turning the
            // low bit off for the previous priority
            if (prefix_priority <= result_priority)
            {
                while (stack.size())
                {
                    Pending &prev = stack.back();
                    if (!done &&
                        prev.priority != default_priority &&
                        result_priority > (prev.priority & ~1))
                        break;
                    if (prev.opcode == prefix)
                        result = new Prefix(prev.argument, result,
                                            prev.position);
                    else
                        result = new Infix(prev.opcode, prev.argument,
                                           result, prev.position);
                    stack.pop_back();
                }
            }

            // Check if new statement
            if (!is_expression)
                if (right->Kind() != BLOCK)
                    if (result_priority > statement_priority)
                        if (stack.size() == 0)
                            result_priority = statement_priority;
                        else
                            if (stack.back().priority < statement_priority)
                                result_priority = statement_priority;
            
            // Push a recognized prefix op
            stack.push_back(Pending(prefix,result,result_priority,pos));
            result = right;
            result_priority = prefix_priority;
        }
    } // While(!done)

    if (stack.size())
    {
        if (!result)
        {
            Pending &last = stack.back();
            if (last.opcode != text("\n"))
                errors.Error("Trailing opcode '$1' ignored",
                             pos, last.opcode);
            result = last.argument;
            stack.pop_back();
        }

        // Check if some stuff remains on stack
        while (stack.size())
        {
            Pending &prev = stack.back();
            if (prev.opcode == prefix)
                result = new Prefix(prev.argument, result, prev.position);
            else
                result = new Infix(prev.opcode, prev.argument,
                                   result, prev.position);
            stack.pop_back();
        }
    }
    return result;
}

XL_END
