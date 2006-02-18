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


    type generic_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //    Information in a generic type
    // ------------------------------------------------------------------------
        parameters      : declaration_list
        context         : SYM.symbol_table
        symbols         : SYM.symbol_table
        validation      : PT.tree
    type generic_type is access to generic_type_data


    type generic_info_data is PT.info_data with
    // ------------------------------------------------------------------------
    //   Information about a context where generics are being declared
    // ------------------------------------------------------------------------
        context         : SYM.symbol_table
        symbols         : SYM.symbol_table
        generic_context : SYM.symbol_table
        parameters      : declaration_list
        initializer     : PT.tree
        validation      : PT.tree
        variadicity     : PT.tree
        generic_types   : string of generic_type
        rest            : PT.tree
    type generic_info is access to generic_info_data


    // Access to the current generic context
    function IsGenericContext() return boolean
    function IsGenericType (tp : any_type) return boolean
    function IsGenericDeclaration (decl : declaration) return boolean
    function DeclGenericType (decl : declaration) return generic_type
    function NonGenericType (tp : any_type) return any_type
    function MakeGeneric(tp : any_type) return any_type
    procedure AddGenericDependency (tp : any_type)
    procedure AddVariadicity (anchor : PT.tree)
    function VariadicExpression() return PT.tree
    function VariadicDeclarations() return PT.tree

    // Enter a generic declaration
    function EnterGenericDecl(Decl  : PT.tree) return PT.tree
