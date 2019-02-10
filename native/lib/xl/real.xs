// ****************************************************************************
//  real.xs                                         XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     The basic real number operations
//
//     Real numbers are actually stored using a floating-point representation
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

REAL[value:type, Size, Align] has
// ----------------------------------------------------------------------------
//    Generic interface for real numbers
// ----------------------------------------------------------------------------

    use ARITHMETIC[value]
    use BITWISE[value]
    use COMPARISON[value]
    use ASSIGNMENT[value]
    use MEMORY.bits_per_byte

    bit_size   value            is size
    bit_align  value            is align
    byte_size  value            is (size  + bits_per_byte - 1) / bits_per_byte
    byte_align value            is (align + bits_per_byte - 1) / bits_per_byte

    bit_size   X:value          is size
    bit_align  X:value          is align
    byte_size  X:value          is byte_size value
    byte_align X:value          is byte_align value

    CONSTANTS has
         quiet_NaN                   as value
         signaling_NaN               as value
         infinity                    as value
         min_value                   as value
         max_value                   as value
         epsilon X:value             as value
         pi                          as value
    use CONSTANTS

    // Type attributes, e.g. `integer.bit_size`
    value has
        bit_size                is bit_size value
        bit_align               is bit_align value
        byte_size               is byte_size value
        byte_align              is byte_align value
        signed                  is true
        use CONSTANTS

    REAL.types := REAL.types, value


REAL has
// ----------------------------------------------------------------------------
//   The base real numbers
// ----------------------------------------------------------------------------

    real                        is SYSTEM.real
    var types                   is real

    real16                      : type
    real32                      : type
    real64                      : type
    real80                      : type
    real128                     : type

    use REAL[real16,   16, 16]
    use REAL[real32,   32, 32]
    use REAL[real64,   64, 64]
    use REAL[real80,   80, 64]
    use REAL[real128, 128, 64]
