// ****************************************************************************
//  decimal.xs                                      XL - An extensible language
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

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
