// *****************************************************************************
// operations.xs                                                      XL project
// *****************************************************************************
//
// File description:
//
//     Common operations on data
//
//     These are typically operations implemented by the CPU on its
//     built-in data types (e.g. integer, pointers, etc)
//
//     They can of course also be implemented on higher-order types
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

module COPY                 // Copy operations
module MOVE                 // Move operations
module CLONE                // Clone operations
module DELETE               // Delete operation
module ARITHMETIC           // Arithmetic operations, e.g. +, -, *, /
module BITWISE              // Bitwise operations, e.g. and, or, xor
module BITSHIFT             // Bitwise shifts and rotations
module COMPARISON           // Comparisons, e.g. <, >, =
