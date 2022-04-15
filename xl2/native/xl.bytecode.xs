// *****************************************************************************
// xl.bytecode.xs                                                     XL project
// *****************************************************************************
//
// File description:
//
//     This file defines the XL "bytecode", a particular form of XL tree
//     used as an intermediate language by the XL compiler. XL bytecode is
//     relatively machine independent, and intended to be usable as an
//     actual bytecode one day. The compiler performs most of its
//     optimizations on this bytecode, and then hands the bytecode over
//     to the machine-description layer, which turns it into real assembly.
//
//     Bytecode may sometimes coexist with a higher level description
//     In that case, it appears in the high-level as @ [bytecode],
//     where [bytecode] is an arbitrary tree.
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2003-2004,2015,2020, Christophe de Dinechin <christophe@dinechin.org>
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

module XL.BYTECODE with

    // By convention, we give a special name for bytecode trees
    type bytecode is PT.tree

