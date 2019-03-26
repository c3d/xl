// ****************************************************************************
//  memory.xs                                       XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for memory management
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

import SYSTEM
use ENUMERATION

MEMORY has
// ----------------------------------------------------------------------------
//   Interface for memory allocation and management
// ----------------------------------------------------------------------------

    // Make system configuration constants visible in this scope
    use SYSTEM.CONFIGURATION
    use TYPES
    use BYTES

    type endianness             is enumeration(BIG, LITTLE, STRANGE)

    TYPES has
    // ------------------------------------------------------------------------
    //    Types related to memory
    // ------------------------------------------------------------------------

        // Types imported from `SYSTEM`
        address                         is SYSTEM.address
        offset                          is SYSTEM.offset
        size                            is SYSTEM.size

        // Types used for specific bits and byte operations
        index                           is another size
        bit_count                       is another size
        byte_count                      is another size
        bit_offset                      is another offset
        byte_offset                     is another offset

    Align Size:size, To:size            as size


    BYTES has
    // ------------------------------------------------------------------------
    //   Functions to convert between bits and bytes
    // ------------------------------------------------------------------------

        // Converts between number of bits and number of bytes
        byte_count  B:bit_count         as byte_count
        bit_count   B:byte_count        as bit_count
        byte_offset B:bit_offset        as byte_offset
        bit_offset  B:byte_count        as bit_offset


    SIZE[item:type] has
    // ------------------------------------------------------------------------
    //   Size information for a type
    // ------------------------------------------------------------------------
        bit_size   X:item               as bit_count
        byte_size  X:item               as byte_count


    SIZE[item:type, Size:bit_count] : SIZE[item] has
    // ------------------------------------------------------------------------
    //   Size information for a type with fixed size
    // ------------------------------------------------------------------------
        bit_size   X:item               is bit_count Size
        byte_size  X:item               is byte_count(bit_size X)


    ALIGN[item:type] has
    // ------------------------------------------------------------------------
    //   Memory alignment for a type
    // ------------------------------------------------------------------------
        bit_align  X:item               as bit_count
        byte_align X:item               as byte_count


    ALIGN[item:type, Align:bit_count] : ALIGN[item] has
    // ------------------------------------------------------------------------
    //   Alignemnt information for a type with fixed size
    // ------------------------------------------------------------------------
        bit_align  X:item               is bit_count Align
        byte_align X:item               is byte_count(bit_align X)
