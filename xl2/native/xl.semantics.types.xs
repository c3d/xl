// ****************************************************************************
//  xl.semantics.types.xs           (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Type system for XL
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


module XL.SEMANTICS.TYPES with
// ----------------------------------------------------------------------------
//   Implements data type representation
// ----------------------------------------------------------------------------

    type any_type_data is PT.tree_info with
    // ------------------------------------------------------------------------
    //   All that the compiler knows about a type
    // ------------------------------------------------------------------------

        // Flags set during semantics
        is_constant             : boolean
        is_variable             : boolean
        is_generic              : boolean
        is_polymorphic          : boolean
        is_subroutine           : boolean
        is_reference            : boolean
        is_instantiation        : boolean
        is_compiler_generated   : boolean

        // Other information recorded about a type
        type_uid                : integer // Cache for accelerated comparisons
        bit_size                : integer
        source_tree             : PT.tree
        name                    : PT.tree // Not always there, may be nil

    type any_type is access to any_type_data


    // Evaluate a type
        


    // The kind used for type info
    type_info_kind   : integer := PT.AllocateInfoKind()

    // The types for literals
    integer_type     : PT.tree := nil
    real_type        : PT.tree := nil
    boolean_type     : PT.tree := nil
    character_type   : PT.tree := nil
    text_type        : PT.tree := nil
