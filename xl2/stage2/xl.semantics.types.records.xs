// *****************************************************************************
// xl.semantics.types.records.xs                                      XL project
// *****************************************************************************
//
// File description:
//
//     Implementation of record types
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
// (C) 2003-2008,2015, Christophe de Dinechin <christophe@dinechin.org>
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


module XL.SEMANTICS.TYPES.RECORDS with
// ----------------------------------------------------------------------------
//    Interface of the module
// ----------------------------------------------------------------------------

    type any_type_data      is TY.any_type_data
    type any_type           is TY.any_type
    type declaration        is DCL.declaration
    type declaration_list   is string of declaration


    type record_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //    Information in a record description
    // ------------------------------------------------------------------------
        fields          : declaration_list
        symbols         : SYM.symbol_table
    type record_type is access to record_type_data


    // Create a parameter list
    procedure MakeFieldList (Fields      : PT.tree;
                             in out List : declaration_list)

    // Create a function type
    function NewRecordType(Source     : PT.tree;
                           Base       : PT.tree;
                           Fields     : PT.tree) return any_type

    function EnterType (Source   : PT.tree;
                        Base     : PT.tree;
                        Fields   : PT.tree) return PT.tree

    function GetRecordType(Record : PT.tree) return record_type
    function GetRecordBaseType(Record : PT.tree) return any_type
    function IsRecord (Record : PT.tree) return boolean
    procedure RecordDeclarations(Record   : PT.tree;
                                 Field    : PT.name_tree;
                                 kind     : text;
                                 out defs : PT.tree_list;
                                 out syms : SYM.symbol_table)
    function Index(Record : PT.tree;
                   Field  : PT.name_tree;
                   Input  : PT.tree) return BC.bytecode
    function EnterUsing (Record : PT.tree) return BC.bytecode

    function EnterDefaults(recName : PT.name_tree;
                           rtp : record_type) return BC.bytecode
    function EnterDefaultFunction(name : PT.tree;
                                  source : PT.tree) return BC.bytecode
