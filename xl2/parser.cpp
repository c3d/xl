// *****************************************************************************
// parser.cpp                                                         XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2003-2004,2015,2019-2020, Christophe de Dinechin <christophe@dinechin.org>
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

#include <vector>
#include "parser.h"
#include "scanner.h"
#include "errors.h"
#include "tree.h"

#include <stdio.h>



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
    Pending(text o, XLTree *a, int p):
        opcode(o), argument(a), priority(p) {}
    text    opcode;
    XLTree *argument;
    int    priority;
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


XLTree *XLParser::Parse(text closing_paren)
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
    XLTree *result = NULL;
    XLTree *left = NULL;
    XLTree *right = NULL;
    text infix, name, spelling;
    text prefix = "";
    text comment_end;
    token_t tok;
    bool done = false;
    text opening, closing;
    int default_priority = context->default_priority;
    int function_priority = context->function_priority;
    int statement_priority = context->statement_priority;
    int prefix_priority, infix_priority, paren_priority;
    int result_priority = default_priority;
    std::vector<Pending> stack;
    bool new_statement = true;
    token_t pendingToken = tokNONE;

    while (!done)
    {
        // If no token pending, scan one
        if (pendingToken == tokNONE)
        {
            right = NULL;
            prefix_priority = default_priority;
            infix_priority = default_priority;
            tok = scanner.NextToken();
        }
        else if (pendingToken == tokNEWLINE)
        {
            // We have a pending newline:
            // skip actual token if it is also a newline
            tok = scanner.NextToken();

            if (tok == tokNEWLINE) // Skip repeated newline
            {
                continue;
            }
            else if (tok == tokSYMBOL || tok == tokNAME)
            {
                name = scanner.NameValue();
                if (context->IsComment(name, comment_end))
                {
                    // Got a comment with a pending newline: skip it
                    scanner.Comment(comment_end);
                    if (comment_end == "\n")
                        continue;
                }

                // Check if we got something like 'else'
                if (context->InfixPriority(name) >= statement_priority)
                {
                    // Otherwise, delay current token and process fake newline
                    pendingToken = tok;
                    tok = tokNEWLINE;
                }
                right = NULL;
                prefix_priority = default_priority;
                infix_priority = default_priority;
            }
            else if (tok == tokINDENT || tok == tokEOF)
            {
                // Something that is a newline itself is parsed directly
                pendingToken = tokNONE;
                right = NULL;
                prefix_priority = default_priority;
                infix_priority = default_priority;
            }
            else
            {
                // Something else is delayed until after we deal with newline
                pendingToken = tok; tok = tokNEWLINE;
                right = NULL;
                prefix_priority = default_priority;
                infix_priority = default_priority;
            }
        }
        else
        {
            // Other pending tokens are processed normally
            tok = pendingToken;
            pendingToken = tokNONE;
        }

        switch (tok)
        {
        case tokEOF:
        case tokERROR:
            done = true;
            break;

        case tokNATURAL:
            right = new XLNatural(scanner.IntegerValue());
            break;
        case tokREAL:
            right = new XLReal(scanner.RealValue());
            break;
        case tokTEXT:
        case tokQUOTE:
            right = new XLString(scanner.StringValue(),
                                tok == tokTEXT ? '"' : '\'');
            break;
        case tokNAME:
        case tokSYMBOL:
            name = scanner.NameValue();
            spelling = scanner.TokenText();
            if (context->IsComment(name, comment_end))
            {
                scanner.Comment(comment_end);
                if (comment_end == "\n")
                    pendingToken = tokNEWLINE;
                continue;
            }
            else if (!result)
            {
                // If this is the very first thing we see
                prefix_priority = context->PrefixPriority(name);
                right = new XLName(spelling);
                if (prefix_priority == default_priority)
                    prefix_priority = function_priority;
            }
            else if (left)
            {
                // This is the right of an infix operator
                // If we have "A and not B", where "not" has higher priority
                // than "and", we want to parse this as "A and (not B)", not
                // as "(A and not) B".
                prefix_priority = context->PrefixPriority(name);
                right = new XLName(spelling);
                if (prefix_priority == default_priority)
                    prefix_priority = function_priority;
            }
            else
            {
                // Complicated case: need to disambiguate infix and prefix
                infix_priority = context->InfixPriority(name);
                if (infix_priority != default_priority)
                {
                    // We got an infix
                    left = result;
                    infix = spelling;
                }
                else
                {
                    // No priority: take this as a prefix-op
                    prefix_priority = context->PrefixPriority(name);
                    right = new XLName(spelling);
                    if (prefix_priority == default_priority)
                        prefix_priority = function_priority;
                }
            }
            break;
        case tokNEWLINE:
            if (pendingToken == tokNONE)
            {
                tok = scanner.NextToken();
                if (tok == tokSYMBOL || tok == tokNAME)
                {
                    if (context->IsComment(scanner.NameValue(), comment_end))
                    {
                        // Followed by a comment:
                        // Can't decide just yet what indent we have,
                        // skip comment
                        scanner.Comment(comment_end);
                        continue;
                    }
                }
                // Otherwise, we'll deal with this other token next.
                pendingToken = tok;
            }
            // Consider newline as an infix operator
            infix = "\n";
            name = infix;
            infix_priority = context->InfixPriority(infix);
            left = result;
            break;
        case tokPARCLOSE:
            // Check for mismatched parenthese here...
            if (scanner.NameValue() != closing_paren)
                XLError(E_ParseMismatchParen,
                        scanner.FileName(), scanner.FileLine(),
                        scanner.NameValue(),
                        ErrorNameOf(closing_paren));
            done = true;
            break;
        case tokUNINDENT:
            // Check for mismatched unindent here...
            if (closing_paren != UNINDENT_MARKER)
                XLError(E_ParseMismatchParen,
                        scanner.FileName(), scanner.FileLine(),
                        text("unindent"),
                        ErrorNameOf(closing_paren));
            done = true;
            break;
        case tokPAROPEN:
        case tokINDENT:
            if (tok == tokINDENT)
            {
                opening = INDENT_MARKER;
                closing = UNINDENT_MARKER;
            }
            else
            {
                opening = scanner.NameValue()[0];
                if (!gContext.IsBlock(opening, closing))
                    XLError(E_ParseMismatchParen,
                            scanner.FileName(), scanner.FileLine(),
                            text("<internal error>"),
                            ErrorNameOf(closing_paren));
            }
            name = opening;
            paren_priority = context->InfixPriority(name);

            // Make sure 'foo.bar(x)' parses as '(foo.bar)(x)'
            if (result)
            {
                while (stack.size())
                {
                    Pending &prev = stack.back();
                    if (prev.priority < paren_priority)
                        break;
                    result_priority = prev.priority;
                    result = new XLInfix(prev.opcode, prev.argument, result);
                    stack.pop_back();
                }
            }

            // Parse the contents of the parenthese
            right = Parse(closing);
            if (!right)
                right = new XLName("");
            right = new XLBlock(right, opening, closing);

            // Parse 'if (A+B) < C then...' as if ((A+B) < C) then ...'
            // Parse 'A[B] := C' as '(A[B] := C)'
            if (result_priority == statement_priority)
                if (paren_priority > function_priority)
                    result_priority = paren_priority;

            if (tok == tokINDENT)
            {
                // Unindent to be seen as unindent followed by new-line
                // so that new-line operator combines lines
                pendingToken = tokNEWLINE;
            }
            break;
        default:
            MZ_ASSERT(!"Invalid token");
        } // Switch


        // Check what is the current result
        if (!result)
        {
            // First thing we parse...
            result = right;
            if (new_statement && prefix_priority == default_priority)
                prefix_priority = statement_priority;
            result_priority = prefix_priority;

            // We are now in the middle of an expression
            if (result)
                new_statement = false;
        }
        else if (left)
        {
            // Check if we had a statement separator
            new_statement = infix_priority < statement_priority;

            // We got left and infix-op, we are now looking for right
            // If we have 'A and not B', where 'not' has higher priority
            // than 'and', we want to finish parsing 'not B' first, rather
            // than seeing this as '(A and not) B'.
            if (prefix_priority != default_priority &&
                prefix_priority > infix_priority)
            {
                // Push "A and" in the above example
                stack.push_back(Pending(infix, left, infix_priority));
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
                        infix_priority>(prev.priority & ~1))
                        break;
                    if (prev.opcode == prefix)
                        left = new XLPrefix(prev.argument, left);
                    else
                        left = new XLInfix(prev.opcode, prev.argument, left);
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
                    stack.push_back(Pending(infix, left, infix_priority));
                    result = NULL;
                }
                left = NULL;
            }
        }
        else if (right)
        {
            // Check priorities for something like "A.B x,y" -> "(A.B) (x,y)"
            // Odd priorities are made right associative by turning the
            // low bit off for the previous priority
            while (stack.size())
            {
                Pending &prev = stack.back();
                if (!done &&
                    prev.priority != default_priority &&
                    result_priority>(prev.priority & ~1))
                    break;
                if (prev.opcode == prefix)
                    result = new XLPrefix(prev.argument, result);
                else
                    result = new XLInfix(prev.opcode, prev.argument, result);
                stack.pop_back();
            }

            // Check if new statement
            if (right->Kind() != xlBLOCK)
                if (stack.size() == 0 || stack.back().priority<statement_priority)
                    result_priority = statement_priority;

            // Push a recognized prefix op
            stack.push_back(Pending(prefix, result, result_priority));
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
                XLError(E_ParseTrailingOp,
                        scanner.FileName(), scanner.FileLine(),
                        last.opcode);
            result = last.argument;
            stack.pop_back();
        }

        while (stack.size())
        {
            // Some stuff remains on stack
            Pending &prev = stack.back();
            if (prev.opcode == prefix)
                result = new XLPrefix(prev.argument, result);
            else
                result = new XLInfix(prev.opcode, prev.argument, result);
            stack.pop_back();
        }
    }
    return result;
}
