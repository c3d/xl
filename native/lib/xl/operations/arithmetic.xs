// ****************************************************************************
//  arithmetic.xs                                   XL - An extensible language
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

use UNSIGNED
use ASSIGNMENT

type arithmetic with
// ----------------------------------------------------------------------------
//   Specification of an arithmetic value type
// ----------------------------------------------------------------------------

    with
        Value   : arithmetic
        Left    : arithmetic
        Right   : arithmetic
        Owned   : copiable and own arithmetic
    do
        Left  +  Right          as arithmetic // Addition
        Left  -  Right          as arithmetic // Subtraction
        Left  *  Right          as arithmetic // Multiplication
        Left  /  Right          as arithmetic // Division
        Left rem Right          as arithmetic // Remainder
        Left mod Right          as arithmetic // Modulo
        Left  ^  Right          as arithmetic // Power
        Left  ^  Right:unsigned as arithmetic // Positive power
        -Left                   as arithmetic // Negation

        Abs  Value              as arithmetic // Absolute value
        Sign Value              as arithmetic // Sign

        ++Owned                 as arithmetic // Pre-incrementation
        --Owned                 as arithmetic // Pre-decrementation
        Owned++                 as arithmetic // Post-incrementation
        Owned--                 as arithmetic // Post-decrementation

        Owned += Right          as nil  is Owned := Owned + Right
        Owned -= Right          as nil  is Owned := Owned - Right
        Owned *= Right          as nil  is Owned := Owned * Right
        Owned /= Right          as nil  is Owned := Owned / Right
