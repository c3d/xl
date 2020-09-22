// *****************************************************************************
// configuration.xs                                                   XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2018-2020, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

ADDRESS_SIZE            as natural //
DATA_SIZE               as natural
ENDIANNESS              as MEMORY.endianness
BITS_PER_BYTE           as natural


use TYPE, UNSIGNED
use MEMORY

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
        ADDRESS_SIZE            as natural
        DATA_SIZE               as natural
        ENDIANNESS              as MEMORY.endianness
        BITS_PER_BYTE           as natural


    module TYPES with
    // ------------------------------------------------------------------------
    //   Native system types exposed by this module
    // ------------------------------------------------------------------------
        natural                as type
        integer                 as type
        address                 as another natural
        offset                  as another integer
        size                    as another natural

    // Make configuration and types visible directly in `SYSTEM`
    use CONFIGURATION, TYPES
