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

    type any_type_data is PT.info_data with
    // ------------------------------------------------------------------------
    //   All that the compiler knows about a type
    // ------------------------------------------------------------------------

        // Flags set during semantics
        is_constant             : boolean
        is_variable             : boolean
        is_generic              : boolean
        is_polymorphic          : boolean
        is_subroutine           : boolean
        is_record               : boolean
        is_reference            : boolean
        is_instantiation        : boolean
        is_compiler_generated   : boolean

        // Other information recorded about a type
        type_uid                : integer // Cache for accelerated comparisons
        bit_size                : integer
        source_tree             : PT.tree
        name                    : PT.tree // Not always there, may be nil
        machine_type            : PT.name_tree

    type any_type is access to any_type_data


    // Evaluate a type expression
    function EvaluateType (type_expr : PT.tree) return any_type
    function TypeExpression(typeToken : any_type) return PT.tree

    // Check if two types are identical
    function SameType (t1 : any_type; t2: any_type) return boolean

    // Convert to a given type
    function Convert(expr : PT.tree; toType : any_type) return PT.tree
    function Convert(expr : PT.tree; toType : PT.tree) return PT.tree

    // Record the type in a tree
    procedure SetType(tree : PT.tree; type : any_type)
    function Type(tree : PT.tree) return any_type


    // The types for literals
    integer_type     : PT.tree := nil
    real_type        : PT.tree := nil
    boolean_type     : PT.tree := nil
    character_type   : PT.tree := nil
    text_type        : PT.tree := nil

    procedure InitializeTypes
