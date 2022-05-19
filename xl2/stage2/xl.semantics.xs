// *****************************************************************************
// xl.semantics.xs                                                    XL project
// *****************************************************************************
//
// File description:
//
//      This is the semantics phase of the XL compiler
//      It takes an XL tree as input, and returns an XL tree as output
//      after checking that the semantics are correct.
//
//      The phase also performs what other compilers might call "expansion",
//      that is the generation of lower-level trees from high-level
//      constructs. The low-level constructs are found in XL.BYTECODE
//      and can be used to directly generate code.
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2003-2004,2015, Christophe de Dinechin <christophe@dinechin.org>
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

module XL.SEMANTICS with

    // Nothing really there nowadays :-)
    verbose : boolean := false

