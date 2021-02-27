// *****************************************************************************
// xl.parser.xs                                                       XL project
// *****************************************************************************
//
// File description:
//
//     The XL parser
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
// (C) 2003-2004,2015,2020, Christophe de Dinechin <christophe@dinechin.org>
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
//
//  Parsing XL is extremely simple. The source code is transformed into
//  a tree with only three type of nodes and four types of leafs:
//
//  The three node types are:
//  - Prefix operator as "not" in "not A" or "+" in "+7"
//  - Infix operator as "-" in "A-B" or "and" in "3 and 5"
//  - Parenthese grouping as in "(A+B)" or "[D + E]".
//
//  The four leaf types are:
//  - Integer numbers such as 130 or 16#FE
//  - Real numbers such as 0.1 or 10.4E-31
//  - Text such as "Hello" or 'ABC'
//  - Name/symbols such as ABC or --->
//
//  High-level program structure is also represented by these same nodes:
//  - A sequence of statements on a same line is a semi-colon infix-op:
//        Do; Redo
//  - A sequence of statements on multiple line is a "new-line" infix-op:
//        Do
//        Redo
//  - A sequence of parameters is made of "comma" infix-op, and a statement
//    is a prefix-op with this sequence of parameters as argument:
//        WriteLn A,B
//  - By default, a sequence of tokens is parsed using prefix operator,
//    unless a token is recognized as an infix operator.
//        A and B or C
//    parses by default as
//        A(and(B(or(C))))
//    but if 'and' and 'or' are declared as infix operators, it parses as
//        ((A and B) or C)
//    or
//        (A and (B or C))
//    depending on the relative precedences of 'and' and 'or'.
//
//  With this scheme, only infix operators need to be declared. In some
//  contexts, a name declared as being an infix operator still parses prefix,
//  for instance in (-A-B), where the first minus is a prefix.
//  Any name or symbol is valid to identify a prefix or infix operator.
//
//  Operator precedence is defined by declaration order. The most recently
//  defined operator has a higher precedence than any operator defined
//  previously. An operator declaration only indicates that a given name
//  or symbol is an operator, but doesn't give its meaning. In other words,
//  an operator declaration only declares how the operator parses,
//  but not its semantics.
//

import PT = XL.PARSER.TREE

module XL.PARSER with

    // Mapping of the types
    type symbol_table is map[text, PT.tree]
    type priority_table is map[text, integer]
    type comment_table is map[text, text]
    type text_delimiters_table is map[text, text]
    type block_table is map[text, text]


    // Data actualy used by the parser
    type parser_data is record with
         scanner            : SC.scanner
         infix_priority     : priority_table
         prefix_priority    : priority_table
         comments           : comment_table
         text_delimiters    : text_delimiters_table
         blocks             : block_table
         symbols            : symbol_table
         priority           : integer
         statement_priority : integer
         function_priority  : integer
         default_priority   : integer
    type parser is access to parser_data

    // Creating a parser
    function NewParser(name : text) return parser
    procedure ReadSyntaxFile (P : PR.parser; syntax : text)

    // Parsing
    function Parse(P : parser) return PT.tree
    function Parse(P : parser; closing : text) return PT.tree

    // Internal names for the indent blocks
    INDENT_MARKER   : text := "I+"
    UNINDENT_MARKER : text := "I-"
