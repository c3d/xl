// *****************************************************************************
// real.xs                                                            XL project
// *****************************************************************************
//
// File description:
//
//     Floating-point representation for real-numbers
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

type real[MantissaBits, ExponentBits] with
// ----------------------------------------------------------------------------
//    A generic interface for real types
// ----------------------------------------------------------------------------

    // Interfaces that 'real' implements
    as number

    // Implement the necessary interface for `type`
    BitSize                     as bit_count
    BitAlign                    as bit_count

    // Special IEEE-754 "values"
    Infinity                    as real
    QuietNaN                    as real
    SignalingNaN                as real
    QuietNaN     Data:unsigned  as real
    SignalingNaN Data:unsigned  as real
    IsInfinity   Value:real     as boolean
    IsNaN        Value:real     as boolean


type real digits Digits exponent Exponent
type real mantissa MantissaBits Digits exponent ExponentBits


// Standard IEEE-754 types
type real16                     is real[ 11,  5]
type real32                     is real[ 24,  8]
type real64                     is real[ 53, 11]
type real128                    is real[113, 15]
