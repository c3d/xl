#ifndef PARSER_H
#define PARSER_H
// ****************************************************************************
//  parser.h                        (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                            Activity project 
// ****************************************************************************
// 
//   File Description:
// 
//      XL language parser
// 
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
/*
  Parsing XL is extremely simple. The source code is transformed into
  a tree with only three type of nodes and four types of leafs:

  The three node types are:
  - Prefix operator as "not" in "not A" or "+" in "+7"
  - Infix operator as "-" in "A-B" or "and" in "3 and 5"
  - Parenthese grouping as in "(A+B)" or "[D + E]".

  The four leaf types are:
  - Integer numbers such as 130 or 16#FE
  - Real numbers such as 0.1 or 10.4E-31
  - String/quotes such as "Hello" or 'ABC'
  - Name/symbols such as ABC or --->

  High-level program structure is also represented by these same nodes:
  - A sequence of statements on a same line is a semi-colon infix-op:
        Do; Redo
  - A sequence of statements on multiple line is a "new-line" infix-op:
        Do
        Redo
  - A sequence of parameters is made of "comma" infix-op, and a statement
    is a prefix-op with this sequence of parameters as argument:
        WriteLn A,B
  - By default, a sequence of tokens is parsed using prefix operator,
    unless a token is recognized as an infix operator.
        A and B or C
    parses by default as
        A(and(B(or(C))))
    but if 'and' and 'or' are declared as infix operators, it parses as
        ((A and B) or C)
    or
        (A and (B or C))
    depending on the relative precedences of 'and' and 'or'.

  With this scheme, only infix operators need to be declared. In some
  contexts, a name declared as being an infix operator still parses prefix,
  for instance in (-A-B), where the first minus is a prefix.
  Any name or symbol is valid to identify a prefix or infix operator.

  Operator precedence is defined by declaration order. The most recently
  defined operator has a higher precedence than any operator defined
  previously. An operator declaration only indicates that a given name
  or symbol is an operator, but doesn't give its meaning. In other words,
  an operator declaration only declares how the operator parses,
  but not its semantics.
 */

#include "scanner.h"
#include "context.h"



// ============================================================================
// 
//    The parser itself
// 
// ============================================================================

class XLParser
// ----------------------------------------------------------------------------
//   This parses an input file and returns a parse tree
// ----------------------------------------------------------------------------
{
public:
    XLParser(kstring name, XLContext *ctx):
        scanner(name), context(ctx) {}

public:
    XLTree *            Parse(text closing_paren = "");
    XLScanner *         Scanner() { return &scanner; }

private:
    XLScanner           scanner;
    XLContext *         context;
};


#endif // PARSER_H
