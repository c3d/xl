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

    type error_code is enumeration
        FileNotFound         = "File $1 not found"
        ScanMixedIndent      = "Mixed indentation"
        ScanInconsistent     = "Inconsistent indentation"
        ScanDoubleUnder      = "Double underscore"
        ScanInvalidBase      = "Invalid base (valid is 2..36)"
        ScanStringEOL        = "End of line in string"
        ParseMismatchParen   = "Mismatched parenthese: got $1, expected $2"
        ParseTrailingOp      = "Trailing opcode $1 ignored"
        InvalidEnum          = "Invalid enum"
        InvalidNamedEnum     = "Invalid named enum"
        NonexistentModule    = "Module '$1' was not found"

    // Signaling an error
    procedure Error (E : error_code; pos : integer; args : string of text)
    procedure Error (E : error_code; pos : integer)
    procedure Error (E : error_code; pos : integer; arg : text)
    procedure Error (E : error_code; pos : integer; arg : text; arg2 : text)
    procedure Error (E : error_code; pos : integer; arg : text; arg2 : text; arg3 : text)
