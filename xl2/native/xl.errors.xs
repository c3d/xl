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

import PT = XL.PARSER.TREE

module XL.ERRORS with

    // Saving and restoring errors
    procedure PushErrorContext()
    function PopErrorContext() return boolean
    procedure DisplayLastErrors()
    function GetLastErrorsText() return text

    // Signaling an error
    procedure Error (E : text; pos : integer; args : string of text)

    // With 'text' parameters
    procedure Error (E : text; pos : integer)
    procedure Error (E : text; pos : integer; arg : text)
    procedure Error (E : text; pos : integer; arg : text; arg2 : text)
    procedure Error (E : text; pos : integer;
                     arg : text; arg2 : text; arg3 : text)

    // With 'tree' parameters (pos deduced from arg1)
    procedure Error (E : text; arg : PT.tree)
    procedure Error (E : text; arg : PT.tree; arg2 : text)
    procedure Error (E : text; arg : PT.tree; arg2 : PT.tree)
    procedure Error (E : text; arg : PT.tree; arg2 : text; arg3 : text)
    procedure Error (E : text; arg : PT.tree; arg2 : text; arg3 : PT.tree)
    procedure Error (E : text; arg : PT.tree; arg2 : PT.tree; arg3 : text)
    procedure Error (E : text; arg : PT.tree; arg2 : PT.tree; arg3 : PT.tree)

    error_count : integer := 0
