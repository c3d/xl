// ****************************************************************************
//  system.xs                                       XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for system configuration
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

use TYPE, UNSIGNED
import MEMORY

module SYSTEM with
// ----------------------------------------------------------------------------
//    Interface for system configuration
// ----------------------------------------------------------------------------

    // Declaration for builtin and runtime operations
    builtin Operation           as anything
    runtime Operation           as anything

    module CONFIGURATION with
    // ------------------------------------------------------------------------
    //    System configuration (filled by the compiler)
    // ------------------------------------------------------------------------
        ADDRESS_SIZE            as unsigned
        DATA_SIZE               as unsigned
        ENDIANNESS              as MEMORY.endianness
        BITS_PER_BYTE           as unsigned


    module TYPES with
    // ------------------------------------------------------------------------
    //   Native system types exposed by this module
    // ------------------------------------------------------------------------
        unsigned                as type
        integer                 as type
        address                 as another unsigned
        offset                  as another integer
        size                    as another unsigned

    // Make configuration and types visible directly in `SYSTEM`
    use CONFIGURATION, TYPES
