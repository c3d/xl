// *****************************************************************************
// xl.types.type.xs                                                  XL project
// *****************************************************************************
//
// File description:
//
//     Interface for the definition of types in XL
//
//     This module provides
//     - the `type` type itself, including key type attributes
//     - common operations on types
//     - a number of type constructors, notably to create subtypes
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2019-2020, Christophe de Dinechin <christophe@dinechin.org>
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

use PARSER = XL.COMPILER.PARSER
use CONFIG = XL.SYSTEM.CONFIGURATION
use MEM    = XL.SYSTEM.MEMORY
use LIFE   = XL.TYPES.LIFETIME


module TYPE with
// ----------------------------------------------------------------------------
//    A module providing core type operations
// ----------------------------------------------------------------------------

    // Aliases for types frequently used in this interface
    type bit_count              is MEM.bit_count
    type byte_count             is MEM.byte_count
    type source                 is PARSER.tree
    type lifetime               is LIFE.lifetime


    type type with
    // ------------------------------------------------------------------------
    //   A `type` is used to identify a set of values
    // ------------------------------------------------------------------------

        // Attributes of the type
        Name                    as source       // Compiler-generated name
        Pattern                 as source       // Pattern for the type
        Condition               as source       // 'when' clause
        Constant                as boolean      // Value immutable over lifetime
        Mutable                 as boolean      // Value mutable over lifetime
        Compact                 as boolean      // Uses contiguous memory
        Lifetime                as lifetime     // Static lifetime for type

        // Testing of values of the type
        Contains  Value         as boolean      // Checks if value belongs
        BitSize   Value:self    as bit_count    // Size in bits
        BitAlign  Value:self    as bit_count    // Alignment in bits
        ByteSize  Value:self    as byte_count   // Size in bytes
        ByteAlign Value:self    as byte_count   // Alignment in bytes


    // Type constructor
    matching Pattern            as type         // Build type for Pattern

    // Syntactic sugar for type declarations
    {type T}                    is {T as type}
    {type T matches P}          is {type T is matching P}

    // Fundamental types matching nothing or everything
    nil                         as type
    anything                    as type

    base:type contains Value    as boolean      // Check if value is contained
    derived:type like base:type as nil          // State derivation relationship
