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

use PRS    = XL.COMPILER.PARSER
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
    type source                 is PRS.tree
    type lifetime               is LIFE.lifetime

    // Forms that are treated specially by the type system
    type annotation             is matching(I:PRS.infix when I.name in ":","as")
    type definition             is matching(I:PRS.infix when I.name = "is")
    type condition              is matching(I:PRS.infix when I.name = "when")


    type type with
    // ------------------------------------------------------------------------
    //   A `type` is used to identify a set of values
    // ------------------------------------------------------------------------

        // Attributes of the type
        Pattern                 as source       // Pattern for the type
        Condition               as source       // 'when' clause
        Attributes              as source       // 'constant', 'mutable', ...
        Bases                   as source       // List of bases
        Lifetime                as lifetime     // Static lifetime for type

    // Type constructor, building a type from a pattern
    matching Pattern            as type

    // Fundamental operator actually defining the types
    Value in Type:type          as boolean

    // Syntactic sugar for type declarations
    macro type T               is T as type
    macro type T matches P     is type T is matching P

    // Fundamental types matching nothing or everything
    nil                         as type
    anything                    as type

    // Derived ways you can check if a value belongs to a type
    Type:type contains Value    as boolean      is Value in Type
    Value matches Pattern       as boolean

    // State a derivation relationship
    Derived:ref type like Base:type as ref type
