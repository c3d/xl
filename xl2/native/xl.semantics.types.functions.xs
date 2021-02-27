// *****************************************************************************
// xl.semantics.types.functions.xs                                    XL project
// *****************************************************************************
//
// File description:
//
//      Description of function types
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
// (C) 2003-2007,2015,2020, Christophe de Dinechin <christophe@dinechin.org>
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


module XL.SEMANTICS.TYPES.FUNCTIONS with
// ----------------------------------------------------------------------------
//    Representation of function types
// ----------------------------------------------------------------------------

    type any_type_data      is TY.any_type_data
    type any_type           is TY.any_type
    type declaration        is DCL.declaration
    type declaration_list   is string of declaration


    type function_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //    Information in a function signature
    // ------------------------------------------------------------------------
    //    The return type is used as base type, since this is what the
    //    value returned by the function will be

        parameters      : declaration_list
        result_decl     : declaration
        symbols         : SYM.symbol_table
        outputs_count   : integer
        inputs_count    : integer
        variadic        : boolean
        iterator        : boolean

    type function_type is access to function_type_data


    // Create a function type
    function NewFnType(Source     : PT.tree;
                       Parms      : PT.tree;
                       ReturnType : PT.tree) return any_type
    function NewIteratorType(Source     : PT.tree;
                             Parms      : PT.tree) return any_type

    function EnterFnType (Source  : PT.tree;
                          Parms   : PT.tree;
                          Ret     : PT.tree) return PT.tree
    function EnterDefaults(fnName : PT.name_tree;
                           ftp    : any_type) return BC.bytecode
