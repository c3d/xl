// *****************************************************************************
// decimal.xs                                                         XL project
// *****************************************************************************
//
// File description:
//
//     Decimal floating-point representation for real number
//
//     Real numbers are actually stored using a floating-point representation
//     XL is designed to support floating-point types based on IEEE-754
//
//
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

type decimal[Bits, Digits, ExponentMax] with
// ----------------------------------------------------------------------------
//    A generic interface for decimal floating-point types
// ----------------------------------------------------------------------------

    // Interfaces that 'decimal' implements
    as number

    // Implement the necessary interface for `type`
    BitSize                     as bit_count
    BitAlign                    as bit_count

    // Special IEEE-754 "values"
    Infinity                    as decimal
    QuietNaN                    as decimal
    SignalingNaN                as decimal
    QuietNaN     Data:unsigned  as decimal
    SignalingNaN Data:unsigned  as decimal
    IsInfinity   Value:decimal  as boolean
    IsNaN        Value:decimal  as boolean


type decimal digits Digits exponent Exponent


// Standard IEEE-754 types
type decimal32                  is decimal[ 32,  7,  96]
type decimal64                  is decimal[ 64, 16, 384]
type decimal128                 is decimal[128, 34,6144]
