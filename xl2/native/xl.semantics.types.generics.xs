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

    type any_type_data  is TY.any_type_data
    type any_type       is TY.any_type
    type declaration    is DCL.declaration


    type generic_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //    Information in a generic type declaration
    // ------------------------------------------------------------------------

        parameters      : string of declaration
        base_type       : any_type
        validation      : PT.tree       // if "compiles A < B"
        when_spec       : PT.tree       // when size(T) < 3
        for_spec        : PT.tree       // for pointer to T

    type generic_type is access to generic_type_data
