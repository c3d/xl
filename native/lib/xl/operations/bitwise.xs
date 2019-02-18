// ****************************************************************************
//  xl.bitwise.xs                                  XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Specification for bitwise operations
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

use ASSIGNMENT

type bitwise with
// ----------------------------------------------------------------------------
//   Specification for bitwise arithmetic operations
// ----------------------------------------------------------------------------

    type bit_count              is unsigned

    with
        Value   : bitwise
        Left    : bitwise
        Right   : bitwise
        Owned   : copiable and own bitwise
        Shift   : bit_count
    do
        Left and  Right         as bitwise // Boolean and
        Left or   Right         as bitwise // Boolean or
        Left xor  Right         as bitwise // Boolean exclusive or

        // Shifts
        Left shl  Shift         as bitwise // Shift left
        Left shr  Shift         as bitwise // Shift right
        Left ashr Shift         as bitwise // Arithmetic (signed) shift right
        Left lshr Shift         as bitwise // Logical shift right

        // Rotations
        Left rol  Shift         as bitwise // Rotate left
        Left ror  Shift         as bitwise // Rotate right

        // Unary operators
        Not Value               as bitwise // Bitwise not

        // Traditional C-style operator notations
        Left  &   Right         as bitwise      is Left and Right
        Left  |   Right         as bitwise      is Left or  Right
        Left  ^   Right         as bitwise      is Left xor Right
        Left  <<  Shift         as bitwise      is Left shl Shift
        Left  >>  Shift         as bitwise      is Left shr Shift
        ~Value                  as bitwise      is Not Value
        !Value                  as bitwise      is Not Value

        // In-place operators with a default implementation
        Owned &=  Right         as nil          is Owned := Owned & Right
        Owned |=  Right         as nil          is Owned := Owned | Right
        Owned ^=  Right         as nil          is Owned := Owned ^ Right
        Owned <<= Shift         as nil          is Owned := Owned << Shift
        Owned >>= Shift         as nil          is Owned := Owned >> Shift

        // Endianness adjustments
        LittleEndian Value      as bitwise // Byte swap if big-endian
        BigEndian Value         as bitwise // Byte swap if little-endian

        CountOneBits     Value  as bit_count
        CountZeroBits    Value  as bit_count
        LowestBit        Value  as bit_count
        HighestBit       Value  as bit_count
