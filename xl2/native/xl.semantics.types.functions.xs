// ****************************************************************************
//  xl.semantics.types.functions.xs (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//      Description of function types
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


module XL.SEMANTICS.TYPES.FUNCTIONS with
// ----------------------------------------------------------------------------
//    Representation of function types
// ----------------------------------------------------------------------------

    type any_type_data  is TY.any_type_data
    type any_type       is TY.any_type
    type declaration    is DCL.declaration


    type function_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //    Information in a function signature
    // ------------------------------------------------------------------------

        parameters      : string of declaration
        return_type     : any_type

    type function_type is access to function_type_data
