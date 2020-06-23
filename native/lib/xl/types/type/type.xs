// *****************************************************************************
// type.xs                                                            XL project
// *****************************************************************************
//
// File description:
//
//     Interface for types
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


TYPE as module with
// ----------------------------------------------------------------------------
//    A module providing core type operations
// ----------------------------------------------------------------------------

    type as [[builtin type]] with
    // ------------------------------------------------------------------------
    //   A `type` is used to identify a set of values
    // ------------------------------------------------------------------------

        // Types
        bit_count               is MEM.bit_count
        byte_count              is MEM.byte_count
        source                  is PARSER.tree

        // Attributes of the type
        Name                    as source       // Compiler-generated name
        Pattern                 as source       // Pattern for the type
        Condition               as source       // Condition for the type (when clause)
        Constant                as boolean      // Value immutable over lifetime
        Mutable                 as boolean      // Value mutable over lifetime
        Compact                 as boolean      // Uses contiguous memory

        // Testing of values of the type
        Contains  Value         as boolean      // Checks if value belongs
        BitSize   Value:self    as bit_count    // Size in bits
        BitAlign  Value:self    as bit_count    // Alignment in bits
        ByteSize  Value:self    as byte_count   // Size in bytes
        ByteAlign Value:self    as byte_count   // Alignment in bytes


    // Type constructor
    type Pattern                as type         // Build type for Pattern

    // Elementary types
    nil                         as type
    anything                    as type

    // Combinaison of types
    T1:type or  T2:type         as type         // Values from one or the other
    T1:type and T2:type         as type         // Values from both types
    not T:type                  as type         // Value not from the type


    // Generic type creation
    any base:type               as type         // Polymorphic type
    some base:type              as type         // Types deriving from 'base'
    optional base:type          as type         // Optional type
    one_of Patterns             as type         // Enumeration type
    any_of Patterns             as type         // Flag type
    base:type with Declarations as type         // Data inheritance
    base:type when Condition    as type         // Type condition
    variable base:type          as type         // Make type mutable
    constant base:type          as type         // Make type constant
    own base:type               as type         // Owning type
    ref base:type               as type         // Reference to owned value
    in base:type                as type         // Type for input parameters
    out base:type               as type         // Type for output parameters
    in_out base:type            as type         // Type for input/output parameters
    io base:type                as type         // Type for input/output parameters

    base:type contains Value    as boolean      // Check if value is contained
    derived:type like base:type as nil          // State derivation relationship
