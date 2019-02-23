// ****************************************************************************
//  xl.bitwise.xs                                  XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Specification for bitwise boolean operations
//
//     Bitwise operations apply on the individual bits in a data type.
//     Operations in this module are common to a variety of data types,
//     notably integer, unsigned and boolean, as well as arrays of such types
//
//
//
//
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

type bitwise with
// ----------------------------------------------------------------------------
//   Specification for bitwise boolean operations
// ----------------------------------------------------------------------------

    with
        Value   : bitwise
        Left    : bitwise
        Right   : bitwise
        Owned   : own bitwise
        Shift   : bit_count
    do
        Left and  Right         as bitwise // Boolean and
        Left or   Right         as bitwise // Boolean or
        Left xor  Right         as bitwise // Boolean exclusive or

        Not Value               as bitwise // Bitwise not

        // Traditional C-style operator notations
        Left  &   Right         as bitwise      is Left and Right
        Left  |   Right         as bitwise      is Left or  Right
        Left  ^   Right         as bitwise      is Left xor Right
        ~Value                  as bitwise      is Not Value
        !Value                  as bitwise      is Not Value

        // In-place operators with a default implementation
        Owned &=  Right         as nil          is Owned := Owned & Right
        Owned |=  Right         as nil          is Owned := Owned | Right
        Owned ^=  Right         as nil          is Owned := Owned ^ Right
