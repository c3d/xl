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
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
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

    type any_type_data      is TY.any_type_data
    type any_type           is TY.any_type
    type declaration        is DCL.declaration
    type declaration_list   is string of declaration


    type generic_type_data
    type generic_type is access to generic_type_data
    type generic_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //    Information in a generic type
    // ------------------------------------------------------------------------
        parameters      : declaration_list
        symbols         : SYM.symbol_table
        context         : SYM.symbol_table


    // Access to the current generic context
    function IsGenericContext() return boolean
    function IsGenericType (tp : any_type) return boolean
    function IsGenericDeclaration (decl : declaration) return boolean
    function DeclGenericType (decl : declaration) return generic_type
    function NonGenericType (tp : any_type) return any_type
    function MakeGeneric(tp : any_type) return any_type
    procedure AddGenericDependency (tp : any_type)

    // Enter a generic declaration
    function EnterGeneric(Parms : PT.tree;
                          Decls : PT.tree) return PT.tree

