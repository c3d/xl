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
// This program is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
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
        E_ParseMismatchParen,
        E_ParseTrailingOp

    procedure Report(E : error; pos : integer)
