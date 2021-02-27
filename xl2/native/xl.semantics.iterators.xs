// *****************************************************************************
// xl.semantics.iterators.xs                                          XL project
// *****************************************************************************
//
// File description:
//
//     Implementation of iterators and iterator overloading
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
// (C) 2003-2006,2015,2020, Christophe de Dinechin <christophe@dinechin.org>
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
import FN = XL.SEMANTICS.FUNCTIONS
import TY = XL.SEMANTICS.TYPES
import SYM = XL.SYMBOLS


module XL.SEMANTICS.ITERATORS with
// ----------------------------------------------------------------------------
//    Interface of the semantics of basic C++-style iterators
// ----------------------------------------------------------------------------

    type declaration            is FN.declaration
    type declaration_list       is FN.declaration_list
    type function_type          is FN.function_type
    type any_type               is TY.any_type


    type iterator_data is FN.function_data with
    // ------------------------------------------------------------------------
    //   An iterator is essentially seen as a special function
    // ------------------------------------------------------------------------
        declaration_context     : SYM.symbol_table
    type iterator is access to iterator_data

    function GetIterator (input : PT.tree) return iterator
    procedure SetIterator (input : PT.tree; it : iterator)

    function EnterIterator (FnIface : PT.tree) return PT.tree
    function EnterIterator (Names   : PT.tree;
                            FnType  : any_type;
                            Init    : PT.tree;
                            Iface   : PT.tree) return PT.tree
    function InvokeIterator(Input   : PT.tree;
                            Base    : PT.tree;
                            iter    : iterator;
                            Args    : PT.tree_list;
                            ctors   : PT.tree;
                            dtors   : PT.tree) return BC.bytecode
