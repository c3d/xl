// ****************************************************************************
//  scanner.xs                      (C) 1992-2003 Christophe de Dinechin (ddd) 
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
// This document is confidential.
// Do not redistribute without written permission
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

import IO = XL.TEXT_IO

module XL.COMPILER.SCANNER
// ----------------------------------------------------------------------------
//   The module interface for scanning XL code
// ----------------------------------------------------------------------------

    type token is enumeration
    // ------------------------------------------------------------------------
    //    Token identification
    // ------------------------------------------------------------------------
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


    type scanner_data
    type scanner is pointer to scanner_data
    // ------------------------------------------------------------------------
    //    Type representing the scanner
    // ------------------------------------------------------------------------


    function NewScanner(file_name : text) return scanner
    // ------------------------------------------------------------------------
    //   Create a new scanner
    // ------------------------------------------------------------------------


    function NextToken(S : scanner) return token
    // ------------------------------------------------------------------------
    //    Parse the file until we get a complete token
    // ------------------------------------------------------------------------


    function Comment(S : scanner; EndOfComment : text) return text
    // ------------------------------------------------------------------------
    //   Scan a comment until the given end
    // ------------------------------------------------------------------------
