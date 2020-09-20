// ****************************************************************************
//  xl.types.generics.xs                                          XL project
// ****************************************************************************
//
//   File Description:
//
//     Generic type constructors that apply to practically any base type
//
//
//
//
//
//
//
//
// ****************************************************************************
//   (C) 2020 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
// ****************************************************************************
//   This file is part of XL.
//
//   XL is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   XL is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with XL.  If not, see <https://www.gnu.org/licenses/>.
// ****************************************************************************

module XL.TYPES.GENERICS with
// ----------------------------------------------------------------------------
//   Interface for generic types
// ----------------------------------------------------------------------------

    with
        base as type

    // Mutability
    variable base               as type         // Make type mutable
    constant base               as type         // Make type constant

    // Refining a type
    base with Declarations      as type         // Data inheritance
    base when Condition         as type         // Type condition

    // Parameter passing
    in base                     as type         // Input parameters
    out base:                   as type         // Output parameters
    inout base:type             as type         // Input/output parameters

    // Ownership
    own base                    as type         // Owning type
    ref base                    as type         // Reference to owned value

    // Polymorphism
    any base                    as type         // Polymorphic type
    some base                   as type         // Types deriving from 'base'
    optional base               as type         // Optional type
    one_of Patterns             as type         // Enumeration type
    any_of Patterns             as type         // Flag type
