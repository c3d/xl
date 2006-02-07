// ****************************************************************************
//  xl.semantics.overload.xs        (C) 1992-2004 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Interface for overload resolution
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
import BC = XL.BYTECODE

module XL.SEMANTICS.OVERLOAD with
// ----------------------------------------------------------------------------
//   Interface for overload resolution
// ----------------------------------------------------------------------------

    function Resolve(NameTree : PT.tree; Args : PT.tree) return BC.bytecode
    function IsFunction(T : PT.tree) return boolean
    procedure ArgsTreeToList(Args : PT.tree; in out List : PT.tree_list)
