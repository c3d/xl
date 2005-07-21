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
    type declaration_data is PT.info_data with
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
        is_builtin              : boolean

        // Other information recorded about a declaration
        name                    : PT.name_tree
        machine_name            : PT.name_tree
        type_source             : PT.tree
        type                    : TY.any_type
        machine_type            : PT.name_tree
        initializer             : PT.tree
        written_as              : PT.tree
        precondition            : PT.tree
        postcondition           : PT.tree
        interface               : declaration
        body                    : declaration
        frame_depth             : integer


    function Lookup (NameTerminal : PT.tree) return BC.bytecode
    function ProcessDeclaration (Names : PT.tree;
                                 Type  : PT.tree;
                                 Value : PT.tree;
                                 HasIs : boolean) return PT.tree

    function Declare (Name : PT.name_tree;
                      Type : TY.any_type) return declaration
    function DeclareTree (Name : PT.name_tree;
                          Type : TY.any_type) return PT.tree
