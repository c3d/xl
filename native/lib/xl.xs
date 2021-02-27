// *****************************************************************************
// xl.xs                                                              XL project
// *****************************************************************************
//
// File description:
//
//    The specification of the top-level XL modules
//
//    The XL module contains all the standard XL capabilities.
//    It is pre-loaded by any XL program, so that its content can be
//    used without any `use` statement.
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2019-2021, Christophe de Dinechin <christophe@dinechin.org>
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

Version         is "0.0.1"
URL             is "http://github.com/c3d/xl"
License         is "GPL v3"
Creator         is "Christophe de Dinechin <christophe@dinechin.org>"
Email           is "xl-devel@lists.sourceforge.net"

module TYPES                // Basic data types (integer1, boolean, ...)
module OPERATIONS           // Elementary operations (arithmetic, ...)
module DATA                 // Data structures (arrays, lists, ...)
module PROGRAM              // Program structures (loop, tests, ...)
module ALGORITHMS           // General algorithms (sort, find, ...)
module MATH                 // Mathematics (functions, types, ...)
module SYSTEM               // System (memory, files, ...)
module COMPILER             // Compiler (scanner, parser, ...)
