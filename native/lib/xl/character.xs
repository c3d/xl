// ****************************************************************************
//  character.xs                                    XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for character data types
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

CHARACTER[value:type, size, align] with
// ----------------------------------------------------------------------------
//    A generic interface for character types
// ----------------------------------------------------------------------------

    use COMPARISON[value], ASSIGN[value], COPY[value]
    use MEMORY

    // Functional notation, e.g. `bit_size unsigned`
    bit_size   value            is size
    bit_align  value            is align
    byte_size  value            is BitsToBytes size
    byte_align value            is BitsToBytes align

    // Functional notation for values of the type, e.g. `byte_size unsigned 0`
    bit_size   X:value          is size
    bit_align  X:value          is align
    byte_size  X:value          is byte_size  value
    byte_align X:value          is byte_align value

    // Range of valid values for the type
    min_value  value            is 0
    max_value  value            is (1 shl size) - 1

    CHARACTER.types := CHARACTER.types, value


CHARACTER with
// ----------------------------------------------------------------------------
//   Definition of the basic character types
// ----------------------------------------------------------------------------

    character                   is UNICODE

    ASCII                       : type
    UNICODE                     : type
    EBCDIC                      : type

    use CHARACTER[ASCII,      8,   8]
    use CHARACTER[UNICODE,   32,  32]
    use CHARACTER[EBCDIC,     8,   8]
