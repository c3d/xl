// ****************************************************************************
//  xl.scanner.xs                   (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     The scanner for the bootstrap compiler
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
//
//  XL scanning is quite simple. There are only five types of tokens:
//
//  - Integer or real numbers, beginning with a digit
//  - Names, beginning with a letter
//  - Strings, enclosed in single or double quotes
//  - Symbols, formed by consecutive sequences of punctuation characters
//  - Blanks and line separators
//
//
//  NUMBERS:
//
//  Numbers can be written in any base, using the '#' notation: 16#FF.
//  They can contain a decimal dot to specify real numbers: 5.21
//  They can contain single underscores to group digits: 1_980_000
//  They can contain an exponent introduced with the letter E: 1.31E6
//  The exponent can be negative, indicating a real number: 1.31E-6; 1E-3
//  Another '#' sign can be used before 'E', in particular when 'E' is
//  a digit of the base: 16#FF#E20
//  The exponent represents a power of the base: 16#FF#E2 is 16#FF00
//  Combinations of the above are valid: 16#FF_00.00_FF#E-5
//
//  NAMES:
//
//  Names begin with any letter, and are made of letters or digits: R19,Hello
//  Names can contain single underscores to group words: Big_Number
//  Names are not case- nor underscore-sensitive: Joe_Dalton=JOEDALTON
//
//
//  STRINGS:
//
//  Strings begin with a single or double quote, and terminate with the same
//  quote used to begin them. They cannot contain a line termination.
//  A quote character can be embedded in a string by doubling it.
//  "ABC" and 'def ghi' are examples of valid strings.
//
//
//  SYMBOLS:
//
//  Symbols are sequences of punctuation characters other than a quote that
//  are not separated by spaces. In symbols, the underscore is a significant
//  character. Examples of valid symbols include ++ , --->  %-%
//  Symbols are normally made of the longest possible sequence of punctuation
//  characters (being terminated by any space, digit, letter or quote).
//  However, the six "parenthese" characters ( ) [ ] { } always represent a
//  complete symbol by themselves.
//  Examples:
//      ---X is the token "---" followed by the token "X"
//      --((X)) is the token "--" followed by two tokens "(" followed by
//                 the token "X" followed by two tokens ")"
//
//
//  BLANKS:
//
//  In XL, indentation is significant, and represented internally by two
//  special forms of parentheses, denoted as 'indent' and 'end'.
//  Indentation can use space or tabs, but not both in the same source file.
//
//
//  COMMENTS:
//
//  The scanner doesn't decide what is a comment. This decision is taken by
//  the caller (normally the parser). The "Comment" function can be called,
//  and skips until an 'end of comment' token is found. For XL, this is
//  under-utilized, since an end-of-comment is always an end of line.
//  XL doesn't in the current definition feature multi-line comment. Because
//  multi-line comments are evil, that's why. See this comment for example.
//

import IO = XL.TEXT_IO

module XL.SCANNER with
// ----------------------------------------------------------------------------
//   The module interface for scanning XL code
// ----------------------------------------------------------------------------

    // Token identification
    type token is enumeration
        tokNONE,

        // Normal conditions
        tokEOF,                     // End of file marker
        tokINTEGER,                 // Integer number
        tokREAL,                    // Real number,
        tokSTRING,                  // Double-quoted string
        tokQUOTE,                   // Single-quoted string
        tokNAME,                    // Alphanumeric name
        tokSYMBOL,                  // Punctuation symbol
        tokNEWLINE,                 // New line
        tokPAROPEN,                 // Opening parenthese
        tokPARCLOSE,                // Closing parenthese
        tokINDENT,                  // Indentation
        tokUNINDENT,                // Unindentation (one per indentation)

        // Error conditions
        tokERROR                    // Some error happened (hard to reach)


    // Type representing the scanner
    type scanner_data
    type scanner is access to scanner_data

    // Create a new scanner
    function NewScanner(file_name : text) return scanner

    // Parse the file until we get a complete token
    function NextToken(S : scanner) return token


    // Scan a comment until the given end
    function Comment(S : scanner; EndOfComment : text) return text

    // Opening and closing parenthese
    function OpenParen(S : scanner) return integer
    procedure CloseParen (S : scanner; old_indent : integer)
