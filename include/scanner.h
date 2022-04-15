#ifndef SCANNER_H
#define SCANNER_H
// *****************************************************************************
// scanner.h                                                          XL project
// *****************************************************************************
//
// File description:
//
//     Interface for the XL scanner
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
// (C) 2003-2004,2009-2012,2014-2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010-2011, Jérôme Forissier <jerome@taodyne.com>
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
/*
  XL scanning is quite simple. There are only five types of tokens:

  - Natural or real numbers, beginning with a digit
  - Names, beginning with a letter
  - Text, enclosed in single or double quotes
  - Symbols, formed by consecutive sequences of punctuation characters
  - Blanks and line separators


  NUMBERS:

  Numbers can be written in any base, using the '#' notation: 16#FF.
  They can contain a decimal dot to specify real numbers: 5.21
  They can contain single underscores to group digits: 1_980_000
  They can contain an exponent introduced with the letter E: 1.31E6
  The exponent can be negative, indicating a real number: 1.31E-6; 1E-3
  Another '#' sign can be used before 'E', in particular when 'E' is
  a digit of the base: 16#FF#E20
  The exponent represents a power of the base: 16#FF#E2 is 16#FF00
  Combinations of the above are valid: 16#FF_00.00_FF#E-5

  NAMES:

  Names begin with any letter, and are made of letters or digits: R19, Hello
  Names can contain single underscores to group words: Big_Number
  Names are not case-sensitive nor underscore-sensitive: Joe_Dalton=JOEDALTON


  TEXT:

  Text begins with a single or double quote, and terminate with the same
  quote used to begin them. It cannot contain a line termination.
  A quote character can be embedded in text by doubling it.
  "ABC" and 'def ghi' are examples of valid text.


  SYMBOLS:

  Symbols are sequences of punctuation characters other than a quote that
  are not separated by spaces. In symbols, the underscore is a significant
  character. Examples of valid symbols include ++ , ---> ( %-%
  Symbols are normally made of the longest possible sequence of punctuation
  characters (therefore being terminated by any space, digit, letter or quote).
  However, the six "parenthese" characters ( ) [ ] { } always represent a
  complete symbol by themselves.
  Examples:
      ---X is the token "---" followed by the token "X"
      --((X)) is the token "--" followed by two tokens "(" followed by
                 the token "X" followed by two tokens ")"


  BLANKS:

  In XL, indentation is significant, and represented internally by two
  special forms of parentheses, denoted as 'indent' and 'end'.
  Indentation can use space or tabs, but not both in the same source file.


  COMMENTS:

  The scanner doesn't decide what is a comment. This decision is taken by
  the caller (normally the parser). The "Comment" function can be called,
  and skips until an 'end of comment' token is found. For XL, this is
  under-utilized, since an end-of-comment is always an end of line.
  XL doesn't in the current definition feature multi-line comment. Because
  multi-line comments are evil, that's why. See this comment for example.
*/

#include "base.h"
#include <recorder/recorder.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

XL_BEGIN

struct Syntax;
struct Errors;

enum token_t
// ----------------------------------------------------------------------------
//   Possible token types
// ----------------------------------------------------------------------------
{
    tokNONE = 0,

    // Normal conditions
    tokEOF,                     // End of file marker
    tokNATURAL,                 // Natural number
    tokREAL,                    // Real number,
    tokTEXT,                    // Double-quoted text
    tokQUOTE,                   // Single-quoted text
    tokLONGTEXT,                // Long (multiline) text
    tokBINARY,                  // Binary data

    tokNAME,                    // Alphanumeric name
    tokSYMBOL,                  // Punctuation symbol
    tokNEWLINE,                 // New line
    tokPAROPEN,                 // Opening parenthese
    tokPARCLOSE,                // Closing parenthese
    tokINDENT,                  // Indentation
    tokUNINDENT,                // Unindentation (one per indentation)

    // Error conditions
    tokERROR                    // Some error happened (normally hard to reach)
};


typedef std::vector<uint> indent_list;
typedef ulong TreePosition;

struct Positions
// ----------------------------------------------------------------------------
//    Records the positions of various scanners
// ----------------------------------------------------------------------------
{
                        Positions(): positions(), current_position(0) {}
                        ~Positions() {}

    TreePosition        OpenFile(text name);
    void                CloseFile (TreePosition pos);

    void                GetFile(TreePosition pos, text *file, ulong *offset);
    void                GetInfo(TreePosition pos,
                                text *file, ulong *line,
                                ulong *column, text *source);

    TreePosition        Here()  { return current_position; }

private:
    struct Range
    {
        Range(text f, ulong s, ulong o = 0): file(f), start(s), offset(o) {}
        text    file;
        ulong   start;
        ulong   offset;
    };
    std::vector<Range>  positions;
    std::vector<Range>  files;
    ulong               current_position;
};


struct Scanner
// ----------------------------------------------------------------------------
//   Interface for invoking the scanner
// ----------------------------------------------------------------------------
{
public:
    Scanner(kstring fileName, Syntax &stx, Positions &pos, Errors &err);
    Scanner(std::istream &input, Syntax &stx, Positions &pos, Errors &err,
            kstring fileName = "<stream>");
    Scanner(const Scanner &parent);
    ~Scanner();

public:
    // Scanning
    token_t     NextToken(bool hungry = false, bool binary = false);
    text        Comment(text EndOfComment, bool stripIndent = true);

    // Access to scanned data
    text        TokenText()             { return tokenText; }
    text        NameValue()             { return textValue; }
    text        TextValue()             { return textValue; }
    double      RealValue()             { return realValue; }
    ulong       NaturalValue()          { return intValue; }
    uint        Base()                  { return base; }
    void        SetTextValue(text t)    { textValue = t; }
    void        SetTokenText(text t)    { tokenText = t; }

    // Access to location information
    uint        Indent()                { return indent; }
    void        SetPosition(ulong pos)  { position = pos; }
    void        SynchronizePosition()   { position = positions.Here(); }
    ulong       Position()              { return position; }
    bool        HadSpaceBefore()        { return hadSpaceBefore; }
    bool        HadSpaceAfter()         { return hadSpaceAfter; }

    // Indent management
    uint        OpenParen();
    void        CloseParen(uint old);

    // Get input of the scanner
    std::istream & Input()              { return input; }
    Positions    & InputPositions()     { return positions; }
    Errors       & InputErrors()        { return errors; }
    Syntax       & InputSyntax()        { return syntax; }

private:
    text           name;
    Syntax &       syntax;
    std::istream & input;
    text           tokenText;
    text           textValue;
    double         realValue;
    ulong          intValue;
    uint           base;
    indent_list    indents;
    uint           indent;
    int            indentChar;
    text           endMarker;
    ulong          position;
    ulong          lineStart;
    Positions &    positions;
    Errors &       errors;
    bool           checkingIndent;
    bool           settingIndent;
    bool           hadSpaceBefore;
    bool           hadSpaceAfter;
    bool           mustDeleteInput;
};

XL_END

RECORDER_DECLARE(scanner);

#endif // SCANNER_H
