// *****************************************************************************
// xl.xs                                                              XL project
// *****************************************************************************
//
// File description:
//
//    The specification of the top-level XL modules
//
//   The XL module contains all the standard XL capabilities.
//   It is pre-loaded by any XL program, so that its content can be
//   used without any `use` statement.
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2019, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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

Version         is 0.0.1
URL             is "http://github.com/c3d/xl"
License         is "GPL v3"
Creator         is "Christophe de Dinechin <christophe@dinechin.org>"
Email           is "xl-devel@lists.sourceforge.net"

TYPES           as module  // Basic data types (integer, boolean, ...)
OPERATIONS      as module  // Elementary operations (arithmetic, ...)
DATA            as module  // Data structures (arrays, lists, ...)
PROGRAM         as module  // Program structures (loop, tests, ...)
ALGORITHMS      as module  // General algorithms (sort, find, ...)
MATH            as module  // Mathematics (functions, types, ...)
SYSTEM          as module  // System (memory, files, ...)
COMPILER        as module  // Compiler (scanner, parser, ...)
