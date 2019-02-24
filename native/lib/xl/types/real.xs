// ****************************************************************************
//  real.xs                                         XL - An extensible language
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

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
