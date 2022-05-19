// *****************************************************************************
// xl.semantics.declarations.xs                                       XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2004-2008,2015, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

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
    function Assignable(Tgt: declaration; Src: PT.tree) return integer
    function Assignable(Tgt: declaration; Src: TY.any_type) return integer
    function MatchInterface(iface : declarations;
                            body  : SYM.symbol_table) return boolean
    function ConstructorNames(tp : TY.any_type) return PT.tree_list
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

