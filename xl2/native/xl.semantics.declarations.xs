// ****************************************************************************
//  xl.semantics.declarations.xs    (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Information stored about declarations by the compiler
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
import TY = XL.SEMANTICS.TYPES


module XL.SEMANTICS.DECLARATIONS with
// ----------------------------------------------------------------------------
//    Manipulating declaration-related information
// ----------------------------------------------------------------------------

    type declaration_data
    type declaration is access to declaration_data
    type declaration_data is PT.tree_info with
    // ------------------------------------------------------------------------
    //   Information stored about declarations
    // ------------------------------------------------------------------------

        // Flags set during semantics
        is_constant             : boolean
        is_variable             : boolean
        is_input                : boolean
        is_output               : boolean
        is_parameter            : boolean
        is_local                : boolean
        is_global               : boolean
        is_field                : boolean
        is_generic              : boolean
        is_generic_parameter    : boolean
        is_polymorphic          : boolean
        is_object               : boolean
        is_type                 : boolean
        is_subroutine           : boolean
        is_reference            : boolean
        is_instantiation        : boolean
        is_compiler_generated   : boolean

        // Other information recorded about a declaration
        source_tree             : PT.tree
        initializer             : PT.tree
        type_expression         : PT.tree
        name                    : PT.name_tree
        written_as              : PT.tree
        precondition            : PT.tree
        postcondition           : PT.tree
        declared_type           : TY.any_type
        interface               : declaration
        body                    : declaration
        frame_depth             : integer
