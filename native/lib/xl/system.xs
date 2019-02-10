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

use INTEGER, UNSIGNED, ENUMERATION

SYSTEM has
// ----------------------------------------------------------------------------
//    Interface for system configuration
// ----------------------------------------------------------------------------

    // Declaration for builtin and runtime operations
    builtin Operation           as anything
    runtime Operation           as anything

    // Base types exposed by this module
    unsigned                    : type
    integer                     : type
    address                     : like unsigned
    offset                      : like integer
    size                        : like unsigned

    CONFIGURATION has
    // ------------------------------------------------------------------------
    //    System configuration (filled by the compiler)
    // ------------------------------------------------------------------------
        ADDRESS_SIZE            as unsigned
        DATA_SIZE               as unsigned
        ENDIANNESS              as enumeration (BIG, LITTLE, STRANGE)
        BITS_PER_BYTE           as unsigned

    // Make configuration visible in `SYSTEM`
    use CONFIGURATION
