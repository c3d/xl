// ****************************************************************************
//  xl.errors.xs                    (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Errors for the XL compiler
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

module XL.ERRORS with

    type error is enumeration
        E_FileNotFound,
        E_ScanMixedIndent,
        E_ScanInconsistent,
        E_ScanDoubleUnder,
        E_ScanInvalidBase,
        E_ScanStringEOL,
        E_ParseMismatchParen

    procedure Report(E : error; file : text; line : integer)
