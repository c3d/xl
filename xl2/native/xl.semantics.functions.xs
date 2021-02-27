// *****************************************************************************
// xl.semantics.functions.xs                                          XL project
// *****************************************************************************
//
// File description:
//
//     Implementation of functions and function overloading
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
// (C) 2003-2008,2015,2020, Christophe de Dinechin <christophe@dinechin.org>
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

import DCL = XL.SEMANTICS.DECLARATIONS
import FT = XL.SEMANTICS.TYPES.FUNCTIONS
import TY = XL.SEMANTICS.TYPES
import SYM = XL.SYMBOLS


module XL.SEMANTICS.FUNCTIONS with
// ----------------------------------------------------------------------------
//    Interface of the semantics of basic C++-style functions
// ----------------------------------------------------------------------------

    type declaration            is FT.declaration
    type declaration_list       is FT.declaration_list
    type function_type          is FT.function_type
    type any_type               is TY.any_type


    type function_data is DCL.declaration_data with
    // ------------------------------------------------------------------------
    //   The function holds the parameters and return type
    // ------------------------------------------------------------------------
        machine_interface       : PT.tree
        preconditions           : PT.tree_list
        postconditions          : PT.tree_list
        symbols                 : SYM.symbol_table // For locals
        result_machine_name     : PT.name_tree
    type function is access to function_data

    function GetFunction (input : PT.tree) return function
    procedure SetFunction (input : PT.tree; fn : function)

    function EnterFunction (FnIface : PT.tree) return PT.tree
    function EnterFunction (Names   : PT.tree;
                            FnType  : any_type;
                            Init    : PT.tree;
                            Iface   : PT.tree) return PT.tree
    function EnterBuiltinFunction(input : PT.tree;
                                  Interface : PT.tree;
                                  BuiltinName : PT.tree) return PT.tree
    function IsConstructor (F : function) return boolean
