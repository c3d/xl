// ****************************************************************************
//  xl.bitwise.xs                                  XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Specification for bitwise operations
//
//
//
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

interface XL.BITWISE[type value] is
// ----------------------------------------------------------------------------
//   Specification for bitwise arithmetic operations
// ----------------------------------------------------------------------------
//   Bitwise operations apply on the individual bits in a data type.
//   Operations in this module are common to a variety of data types,
//   notably integer, unsigned and boolean, as well as arrays of such types

    type bit_count              is unsigned

    // Bitwise operations
    X:value and  Y:value        as value // Boolean and
    X:value or   Y:value        as value // Boolean or
    X:value xor  Y:value        as value // Boolean exclusive or

    // Shifts
    X:value shl  Y:bit_count    as value // Shift left
    X:value shr  Y:bit_count    as value // Shift right
    X:value ashr Y:bit_count    as value // Arithmetic (signed) shift right
    X:value lshr Y:bit_count    as value // Logical shift right

    // Rotations
    X:value rol  Y:bit_count    as value // Rotate left
    X:value ror  Y:bit_count    as value // Rotate right

    little_endian X:value       as value // Byte swap if big-endian
    big_endian X:value          as value // Byte swap if little-endian
    not X:value                 as value // Bitwise not

    count_one_bits   X:value    as bit_count
    count_zero_bits  X:value    as bit_count
    lowest_bit       X:value    as bit_count
    highest_bit      X:value    as bit_count
