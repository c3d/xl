// *****************************************************************************
// xl.codegenerator.machine.xs                                        XL project
// *****************************************************************************
//
// File description:
//
//      Machine-dependent part of the code generator
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
import FT = XL.SEMANTICS.TYPES.FUNCTIONS
import RT = XL.SEMANTICS.TYPES.RECORDS
import FN = XL.SEMANTICS.FUNCTIONS
import PT = XL.PARSER.TREE
import BC = XL.BYTECODE


module XL.CODE_GENERATOR.MACHINE with
// ----------------------------------------------------------------------------
//    The interface of the machine-dependent part of the code generator
// ----------------------------------------------------------------------------

    // Computing machine names ("mangled" names in C++ parlance)
    function Name (Name : PT.name_tree;
                   Type : TY.any_type) return PT.name_tree

    // Checking that target has bytecode format
    function HasBytecode(input : PT.tree) return boolean

    // Expression code
    function MakeExpression(computation : BC.bytecode;
                            mname       : PT.name_tree) return BC.bytecode
    procedure SplitExpression(in out value  : PT.tree;
                              in out code   : PT.tree)
    procedure SetExpressionTarget(in out value  : PT.tree;
                                  mname         : PT.tree)

    // Interface for function declarations
    function Entry (f : FN.function) return BC.bytecode
    function EntryPointer (f : FT.function_type) return PT.name_tree
    function FunctionBody(f     : FN.function;
                          Iface : PT.tree;
                          Body  : PT.tree) return PT.tree

    // Interface for function calls
    type machine_args is string of PT.tree
    function FunctionCall (toCall  : FN.function;
                           margs   : machine_args;
                           target  : PT.tree;
                           ctors   : PT.tree;
                           dtors   : PT.tree) return BC.bytecode
    function RecordFunctionCall (Record : PT.tree;
                                 toCall   : FN.function;
                                 margs    : machine_args;
                                 target  : PT.tree;
                                 ctors    : PT.tree;
                                 dtors    : PT.tree) return BC.bytecode
    function EnterCall() return integer
    procedure AddCallDtors(dtor : PT.tree)
    procedure ExitCall(depth : integer; in out value : PT.tree)

    // Interface for record types
    function DeclareRecord(r : RT.record_type) return PT.name_tree
    function IndexRecord(Record  : PT.tree;
                         MField  : PT.tree;
                         MType   : PT.tree;
                         Type    : TY.any_type) return BC.bytecode

    // Declare other types
    function DeclareType(tp   : TY.any_type;
                         Name : PT.name_tree) return PT.name_tree

