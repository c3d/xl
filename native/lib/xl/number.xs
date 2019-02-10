// ****************************************************************************
//  number.xs                                       XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for describing numbers
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

use MEMORY, ENUMERATION, SEQUENCE

NUMBER with
// ----------------------------------------------------------------------------
//   Interface for aspects common to all numbers
// ----------------------------------------------------------------------------

    // List of number types
    types                       : sequence of type
    Register T:type             as nil

    kind is enumeration(INTEGER, FIXED, FLOAT, OTHER)


NUMBER[number   : type,         // Type being described
       Kind     : NUMBER.kind,  // Kind of number
       Signed   : boolean,      // Has a sign bit
       Size     : bit_count,    // Size in bits
       Align    : bit_count,    // Alignment in bits
       Scaling  : bit_offset    // Scaling bits
      ] with
// ----------------------------------------------------------------------------
//    Interface for a generic description of numbers
// ----------------------------------------------------------------------------

    // Type attributes, e.g. `integer.bit_size`
    number has
        bit_size                is Size
        bit_align               is Align
        byte_size               is MEMORY.BitsToBytes(BitSize)
        byte_align              is MEMORY.BitsToBytes(BitAlign)
        signed                  is Signed
        floating_point          is ExpBits > 0
        fixed_point             is not floating_point and Scaling <> 0


    // Numbers support some kind of arithmetic
    use ARITHMETIC[number]

    // Numbers can be compared
    use COMPARISON[number]

    // Integrat
    BITWISE[number], COMPARISON[number]
    use ASSIGN[number], COPY[number]

    // Functional notation, e.g. `bit_size integer`
    bit_size   number            is size
    bit_align  number            is align
    byte_size  number            is BitsToBytes size
    byte_align number            is BitsToBytes align

    // Functional notation for numbers of the type, e.g. `byte_size 0`
    bit_size   X:number          is size
    bit_align  X:number          is align
    byte_size  X:number          is byte_size number
    byte_align X:number          is byte_align number

    // Range of valid numbers for the type
    min_number  number            is 1 shl (size-1)
    max_number  number            is not min_number number

    // Append the type to the list of numbers
    NUMBER.types := NUMBER.types, number
