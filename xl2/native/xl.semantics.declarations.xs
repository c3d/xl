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
// * Revision   : $Revision: 361 $
// * Date       : $Date: 2007-12-20 10:07:49 +0100 (Thu, 20 Dec 2007) $
// ****************************************************************************

import PT = XL.PARSER.TREE
import TY = XL.SEMANTICS.TYPES
import SYM = XL.SYMBOLS


module XL.SEMANTICS.DECLARATIONS with
// ----------------------------------------------------------------------------
//    Manipulating declaration-related information
// ----------------------------------------------------------------------------

    type declaration_data
    type declaration is access to declaration_data
    type declarations is string of declaration

    function EnterDeclaration (Names        : PT.tree;
                               Type         : PT.tree;
                               Value        : PT.tree;
                               IsDefinition : boolean) return PT.tree
    function Declare(Name : PT.name_tree; tp: TY.any_type) return declaration
    function GetDeclaration(decl : PT.tree) return declaration
    procedure SetDeclaration(decl : PT.tree; info : declaration)
    function Lookup (NameTerminal : PT.tree) return BC.bytecode
    procedure SetLookupResult (NameTerminal : PT.tree; Value : BC.bytecode)
    function Assignable(Tgt: declaration; Src: PT.tree) return boolean
    function Assignable(Tgt: declaration; Src: TY.any_type) return boolean
    function MatchInterface(iface : declarations;
                            body  : SYM.symbol_table) return boolean
    function CallConstructor (decl : declaration) return PT.tree
    function ConstructorCode (decl : declaration) return PT.tree
    function CallDestructor (name : PT.tree) return PT.tree
    procedure CallDestructors (table : SYM.symbol_table)
    function ScopeDestructors (inner : SYM.symbol_table;
                               outer : SYM.symbol_table;
                               excl  : boolean) return BC.bytecode
    procedure DoNotDelete (table : SYM.symbol_table; decl : declaration)


    type declaration_data is PT.info_data with
    // ------------------------------------------------------------------------
    //   Information stored about declarations
    // ------------------------------------------------------------------------

        // Source elements in a declaration
        name                    : PT.name_tree
        type                    : TY.any_type
        initializer             : PT.tree

        // Compiler-generated information
        machine_name            : PT.name_tree
        frame_depth             : integer
        is_input                : boolean
        is_output               : boolean
        is_variable             : boolean
        is_parameter            : boolean
        is_local                : boolean
        is_global               : boolean
        is_field                : boolean
        is_generic_parm         : boolean
        is_builtin              : boolean
        implementation          : declaration

