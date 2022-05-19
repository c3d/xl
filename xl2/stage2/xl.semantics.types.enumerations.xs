// *****************************************************************************
// xl.semantics.types.enumerations.xs                                 XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2007,2015, Christophe de Dinechin <christophe@dinechin.org>
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
