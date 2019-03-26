// ****************************************************************************
//  operations.xs                                   XL - An extensible language
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

module COPY                 // Copy operations
module MOVE                 // Move operations
module CLONE                // Clone operations
module DELETE               // Delete operation
module ARITHMETIC           // Arithmetic operations, e.g. +, -, *, /
module BITWISE              // Bitwise operations, e.g. and, or, xor
module BITSHIFT             // Bitwise shifts and rotations
module COMPARISON           // Comparisons, e.g. <, >, =
