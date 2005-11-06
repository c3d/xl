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
import SYM = XL.SYMBOLS


module XL.SEMANTICS.TYPES with
// ----------------------------------------------------------------------------
//   Implements data type representation
// ----------------------------------------------------------------------------

    type any_type_data
    type any_type is access to any_type_data

    // Accessing the type in a parse tree (for values)
    procedure SetType(tree : PT.tree; type : any_type)
    function GetType(tree : PT.tree) return any_type

    // Accessing the type in a parse tree (for type names)
    procedure SetDefinedType(tree : PT.tree; type : any_type)
    function GetDefinedType(tree : PT.tree) return any_type

    // Creating a new type
    function NewType(Name : PT.tree) return any_type
    function ConstantType(base : any_type) return any_type
    function VariableType(base : any_type) return any_type

    // Finding a type in the type table
    procedure EnterSignature(sig : text; tp : any_type)
    function SignatureType(sig : text) return any_type

    // Type attributes
    function IsConstant (tp: any_type) return boolean
    function IsVariable (tp: any_type) return boolean
    function BaseType (tp: any_type) return any_type
    function IsConstedType (tp: any_type) return boolean
    function NonConstedType (tp: any_type) return any_type
    function MachineName(tp: any_type) return PT.name_tree
    function Source(tp : any_type) return PT.tree

    // Evaluate a type expression
    function EvaluateType (type_expr : PT.tree) return any_type

    // Entering a named type in the symbol table
    function EnterType(Name : PT.tree; Value : PT.tree) return PT.tree

    // Named types
    function NamedType (tname : PT.name_tree) return any_type
    function IsTypeName(type_expr : PT.tree) return boolean

    // Check if two types are identical
    function SameType (t1 : any_type; t2: any_type) return boolean
    function InterfaceMatch (iface: any_type; body: any_type) return boolean

    // Convert to a given type
    function Convert(expr : PT.tree; toType : any_type) return PT.tree
    function Convert(expr : PT.tree; toType : PT.tree) return PT.tree

    // The built-in types
    integer_type                : PT.tree := nil
    real_type                   : PT.tree := nil
    boolean_type                : PT.tree := nil
    character_type              : PT.tree := nil
    text_type                   : PT.tree := nil
    record_type                 : PT.tree := nil
    module_type                 : PT.tree := nil

    // The literal types
    type_of_types               : any_type := nil
    const_boolean_type          : any_type := nil
    integer_literal_type        : any_type := nil
    real_literal_type           : any_type := nil
    character_literal_type      : any_type:= nil
    text_literal_type           : any_type := nil

    procedure InitializeTypes

    type any_type_data is PT.info_data with
    // ------------------------------------------------------------------------
    //    Information stored in a type
    // ------------------------------------------------------------------------
        base            : any_type
        machine_name    : PT.name_tree
        interface_match : function (iface: any_type;
                                    body: any_type) return boolean
        name            : PT.tree

    type source_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //    Information to create a type from source ('type X')
    // -----------------------------------------------------------------------
    //  These are the types created by NewType
    //  A source type can be created without a value
        interface               : any_type
        implementation          : any_type
    type source_type is access to source_type_data


    type constvar_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //    Information to create a 'constant' or 'variable' type
    // ------------------------------------------------------------------------
        is_constant             : boolean
    type constvar_type is access to constvar_type_data


    type code_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //    Information regarding a code body
    // ------------------------------------------------------------------------
        symbols                 : SYM.symbol_table
    type code_type is access to code_type_data


    type type_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //    Information regarding a type
    // ------------------------------------------------------------------------
        symbols                 : SYM.symbol_table
    type type_type is access to type_type_data
