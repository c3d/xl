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
        validation      : PT.tree       // if "compiles A < B"
        when_spec       : PT.tree       // when size(T) < 3
        for_spec        : PT.tree       // for pointer to T
        symbols         : SYM.symbol_table

    type generic_type is access to generic_type_data

    // Create a function type
    function NewGenType(Source     : PT.tree;
                        Base       : PT.tree;
                        Parms      : PT.tree;
                        Valid      : PT.tree;
                        WhenSpec   : PT.tree;
                        ForSpec    : PT.tree) return generic_type

    function EnterGenType (Source     : PT.tree;
                           Base       : PT.tree;
                           Parms      : PT.tree;
                           Valid      : PT.tree;
                           WhenSpec   : PT.tree;
                           ForSpec    : PT.tree) return PT.tree
