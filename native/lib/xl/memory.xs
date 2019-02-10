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

import SYS = SYSTEM

MEMORY has
// ----------------------------------------------------------------------------
//   Interface for memory allocation and management
// ----------------------------------------------------------------------------

    // Types imported from `SYSTEM`
    address                     is SYS.address
    offset                      is SYS.offset
    size                        is SYS.size

    // Make system configuration constants visible in this scope
    use SYS.CONFIGURATION

    // Types used for specific bits and byte operations
    index                       is new size     // Index e.g. in array
    bit_count                   is new size     // Number of bits
    byte_count                  is new size     // Number of bytes
    bit_offset                  is new offset   // Offset in bits
    byte_offset                 is new offset   // Offset in bytes

    // Converts between number of bits and number of bytes
    byte_count  B:bit_count     as byte_count
    bit_count   B:byte_count    as bit_count
    byte_offset B:bit_offset    as byte_offset
    bit_offset  B:byte_count    as bit_offset
