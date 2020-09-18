// *****************************************************************************
// arithmetic.xs                                                      XL project
// *****************************************************************************
//
// File description:
//
//     Specification for arithmetic operations
//
//     Arithmetic operations perform some of the most elementary operations,
//     like addition, multiplication, ...
//
//     Operations in this module are common to a variety of data types,
//     notably machine-level types such as integer, unsigned or real,
//     as well as higher-order types such as complex, vector and matrix.
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2019-2020, Christophe de Dinechin <christophe@dinechin.org>
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

use UNSIGNED

type arithmetic with
// ----------------------------------------------------------------------------
//   Specification of an arithmetic value type
// ----------------------------------------------------------------------------

    with
        Value   : arithmetic
        Left    : arithmetic
        Right   : arithmetic
        Owned   : own arithmetic
    do
        Left  +  Right          as arithmetic   // Addition
        Left  -  Right          as arithmetic   // Subtraction
        Left  *  Right          as arithmetic   // Multiplication
        Left  /  Right          as arithmetic   // Division
        Left rem Right          as arithmetic   // Remainder of division
        Left mod Right          as arithmetic   // Euclidean Modulo
        Left  ^  Right          as arithmetic   // Power
        Left  ^  Power:unsigned as arithmetic   // Positive power
        -Left                   as arithmetic   // Negation

        Abs  Value              as arithmetic   // Absolute value
        Sign Value              as arithmetic   // -1, 0 or 1

        ++Owned                 as arithmetic   // Pre-incrementation
        --Owned                 as arithmetic   // Pre-decrementation
        Owned++                 as arithmetic   // Post-incrementation
        Owned--                 as arithmetic   // Post-decrementation

        Owned += Right          as nil          // In-place addition
        Owned -= Right          as nil          // In-place subtraction
        Owned *= Right          as nil          // In-place multiplication
        Owned /= Right          as nil          // In-place division

    // Values 0 and 1 should always be convertible to arithmetic values
    0                           as arithmetic
    1                           as arithmetic
