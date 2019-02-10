// ****************************************************************************
//  unsigned.xs                                     XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for unsigned types
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

UNSIGNED[value:type, size, align] has
// ----------------------------------------------------------------------------
//    A generic interface for unsigned types
// ----------------------------------------------------------------------------

    use ARITHMETIC[value], BITWISE[value], COMPARISON[value]
    use ASSIGN[value], COPY[value]

    // Type attributes, e.g. `integer.bit_size`
    value has
        import MEMORY
        bit_size                is size
        bit_align               is align
        byte_size               is MEMORY.BitsToByte size
        byte_align              is MEMORY.BitsToByte align
        min_value               is 0
        max_value               is (1 shl size) - 1
        signed                  is false

    // Functional notation, e.g. `bit_size unsigned`
    bit_size   value            is size
    bit_align  value            is align
    byte_size  value            is BitsToBytes size
    byte_align value            is BitsToBytes align

    // Functional notation for values of the type, e.g. `byte_size unsigned 0`
    bit_size   X:value          is size
    bit_align  X:value          is align
    byte_size  X:value          is byte_size value
    byte_align X:value          is byte_align value

    // Range of valid values for the type
    min_value  value            is 0
    max_value  value            is (1 shl size) - 1

    // Add any processed type to the list of types
    UNSIGNED.types := INTEGER.types, value


UNSIGNED has
// ----------------------------------------------------------------------------
//   Definition of the basic unsigned types
// ----------------------------------------------------------------------------

    unsigned            is SYSTEM.unsigned
    var types           is unsigned

    unsigned8           : type
    unsigned16          : type
    unsigned32          : type
    unsigned64          : type
    unsigned128         : type

    use UNSIGNED[unsigned8,     8,   8]
    use UNSIGNED[unsigned16,   16,  16]
    use UNSIGNED[unsigned32,   32,  32]
    use UNSIGNED[unsigned64,   64,  64]
    use UNSIGNED[unsigned128, 128, 128]
