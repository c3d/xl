// ****************************************************************************
//  xl.semantics.types.enumerations.xs      (C) 1992-2007 C. de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Enumeration types
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
import SYM = XL.SYMBOLS
import PT = XL.PARSER.TREE


module XL.SEMANTICS.TYPES.ENUMERATIONS with
// ----------------------------------------------------------------------------
//    Interface of the module
// ----------------------------------------------------------------------------

    type any_type_data      is TY.any_type_data
    type any_type           is TY.any_type
    type declaration        is DCL.declaration
    type declaration_list   is string of declaration


    type enumeration_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //    Information in an enumeration description
    // ------------------------------------------------------------------------
        names           : declaration_list
        symbols         : SYM.symbol_table
        lowest_value    : integer
        highest_value   : integer
    type enumeration_type is access to enumeration_type_data


    // Create a parameter list
    procedure MakeNamesList (Names        : PT.tree;
                             in out List  : declaration_list;
                             in out Index : integer)

    // Create a function type
    function NewEnumerationType(Source     : PT.tree;
                                Base       : PT.tree;
                                Names      : PT.tree) return any_type

    function EnterEnumerationType (Source   : PT.tree;
                                   Base     : PT.tree;
                                   Names    : PT.tree) return PT.tree

    function GetEnumerationType(En : PT.tree) return enumeration_type
    function IsEnumeration (En : PT.tree) return boolean

    function EnterDefaults(enName : PT.name_tree;
                           etp : enumeration_type) return BC.bytecode

    function Index(EnumName : PT.tree; ItemName : PT.name_tree) return PT.tree
