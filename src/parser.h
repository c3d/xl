#ifndef PARSER_H
#define PARSER_H
// ****************************************************************************
//  parser.h                        (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                            Activity project 
// ****************************************************************************
// 
//   File Description:
// 
//      ELIOT language parser
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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************
/*
  Parsing ELIOT is extremely simple. The source code is transformed into
  a tree with only three type of nodes and four types of leafs:

  The three node types are:
  - Prefix operator as "not" in "not A" or "+" in "+7"
  - Infix operator as "-" in "A-B" or "and" in "3 and 5"
  - Parenthese grouping as in "(A+B)" or "[D + E]".

  The four leaf types are:
  - Integer numbers such as 130 or 16#FE
  - Real numbers such as 0.1 or 10.4E-31
  - Text such as "Hello" or 'ABC'
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

  Operator precedence is defined by the eliot.syntax file.

  Comments and extraneous line separators are preserved in CommentsInfo nodes
  attached to the returned parse trees.
*/

#include "scanner.h"
#include "syntax.h"
#include "info.h"
#include <vector>


ELIOT_BEGIN

// ============================================================================
// 
//    The parser itself
// 
// ============================================================================

struct Errors;
struct CommentsInfo;
typedef std::vector<text> CommentsList;


class Parser
// ----------------------------------------------------------------------------
//   This parses an input file and returns a parse tree
// ----------------------------------------------------------------------------
{
public:
    Parser(kstring name, Syntax &stx, Positions &pos, Errors &err)
        : scanner(name, stx, pos, err),
          syntax(stx), errors(err), pending(tokNONE),
          openquote(), closequote(), comments(), commented(NULL),
          hadSpaceBefore(false), hadSpaceAfter(false), beginningLine(true) {}
    Parser(std::istream &input, Syntax &stx, Positions &pos, Errors &err,
           kstring name="<stream>")
        : scanner(input, stx, pos, err, name),
          syntax(stx), errors(err), pending(tokNONE),
          openquote(), closequote(), comments(), commented(NULL),
          hadSpaceBefore(false), hadSpaceAfter(false), beginningLine(true) {}
    Parser(Scanner &scanner, Syntax *stx)
        : scanner(scanner),
          syntax(stx ? *stx : scanner.InputSyntax()),
          errors(scanner.InputErrors()),
          pending(tokNONE),
          openquote(), closequote(), comments(), commented(NULL),
          hadSpaceBefore(false), hadSpaceAfter(false), beginningLine(true) {}

public:
    Tree *              Parse(text closing_paren = "");
    Scanner *           ParserScanner()         { return &scanner; }
    token_t             NextToken();
    void                AddComment(text c)      { comments.push_back(c); }
    void                AddComments(Tree *, bool before);

private:
    Scanner             scanner;
    Syntax &            syntax;
    Errors &            errors;
    token_t             pending;
    text                openquote, closequote;
    CommentsList        comments;
    Tree *              commented;
    bool                hadSpaceBefore, hadSpaceAfter, beginningLine;
};


struct CommentsInfo : Info
// ----------------------------------------------------------------------------
//   Information recorded about comments
// ----------------------------------------------------------------------------
{
    CommentsInfo() {}
    ~CommentsInfo() {}

public:
    CommentsList before, after;
};

ELIOT_END

#endif // PARSER_H
