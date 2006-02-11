// ****************************************************************************
//  xl.constants.xs                 (C) 1992-2004 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Interface for the constant evaluation of the XL compiler
// 
// 
// 
// 
// 
// 
// 
// 
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

import PT = XL.PARSER.TREE


module XL.CONSTANTS with
// ----------------------------------------------------------------------------
//    Functions used to evaluate if something evalutes as a constant
// ----------------------------------------------------------------------------

    // Return a tree representing the constant value, or nil
    function EvaluateConstant (value : PT.tree) return PT.tree

    // Entering and looking up constant names
    function NamedConstant(name : PT.name_tree) return PT.tree
    procedure EnterNamedConstant(name : PT.name_tree; value : PT.tree)
    function IsTrue (cst : PT.tree) return boolean
    function IsFalse (cst : PT.tree) return boolean
    function IsBoolean (cst : PT.tree) return boolean
