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
// This document is confidential.
// Do not redistribute without written permission
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

import IO = XL.TEXT_IO

module XL.SCANNER
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
    type scanner is pointer to scanner_data

    // Create a new scanner
    function NewScanner(file_name : text) return scanner

    // Parse the file until we get a complete token
    function NextToken(S : scanner) return token


    // Scan a comment until the given end
    function Comment(S : scanner; EndOfComment : text) return text
