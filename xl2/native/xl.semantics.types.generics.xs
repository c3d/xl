// ****************************************************************************
//  xl.semantics.types.generics.xs  (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Description of generic types
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

import TY = XL.SEMANTICS.TYPES
import DCL = XL.SEMANTICS.DECLARATIONS
import PT = XL.PARSER.TREE


module XL.SEMANTICS.TYPES.GENERICS with
// ----------------------------------------------------------------------------
//    Representation of generic types
// ----------------------------------------------------------------------------

    function EnterGenericDecls(Decls  : PT.tree) return PT.tree
