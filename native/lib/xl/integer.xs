// ****************************************************************************
//  integer.xs                                      XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     XL integer types and related arithmetic
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

INTEGER[value:type, size, align] has
// ----------------------------------------------------------------------------
//    A generic interface for integer types
// ----------------------------------------------------------------------------

    use ARITHMETIC[value], BITWISE[value], COMPARISON[value]
    use ASSIGN[value], COPY[value]
    use MEMORY

    // Functional notation, e.g. `bit_size integer`
    bit_size   value            is size
    bit_align  value            is align
    byte_size  value            is BitsToBytes size
    byte_align value            is BitsToBytes align

    // Functional notation for values of the type, e.g. `byte_size 0`
    bit_size   X:value          is size
    bit_align  X:value          is align
    byte_size  X:value          is byte_size value
    byte_align X:value          is byte_align value

    // Range of valid values for the type
    min_value  value            is 1 shl (size-1)
    max_value  value            is not min_value value

    // Type attributes, e.g. `integer.bit_size`
    value has
        bit_size                is bit_size value
        bit_align               is bit_align value
        byte_size               is byte_size value
        byte_align              is byte_align value
        signed                  is true

    // Add any processed type to the list of types
    INTEGER.types := INTEGER.types, value


INTEGER has
// ----------------------------------------------------------------------------
//   Definition of the basic integer types
// ----------------------------------------------------------------------------

    integer             is SYSTEM.integer
    var types           is integer

    integer8            : type
    integer16           : type
    integer32           : type
    integer64           : type
    integer128 i        : type

    use INTEGER[integer8,     8,   8]
    use INTEGER[integer16,   16,  16]
    use INTEGER[integer32,   32,  32]
    use INTEGER[integer64,   64,  64]
    use INTEGER[integer128, 128, 128]
