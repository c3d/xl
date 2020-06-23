// *****************************************************************************
// types.xs                                                           XL project
// *****************************************************************************
//
// File description:
//
//     Interface for the TYPES modules in XL
//
//     This defines
//
//
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

TYPE            as module       // Representation of types

COMBINATIONS    as module       // Combinations of types
LIFETIME        as module       // Lifetime of entities

NUMBER          as module       // Common aspects of numbers
ENUMERATED      as module       // Enumerated values

BOOLEAN         as module       // Boolean type (true / false)
INTEGER         as module       // Signed and unsigned integer types
REAL            as module       // Real numbers
DECIMAL         as module       // Decimal floating-point numbers
FIXED_POINT     as module       // Fixed-point numbers
CHARACTER       as module       // Representation of characters
TEXT            as module       // Text (sequence of characters)

CONVERSIONS     as module       // Standard conversions

RANGE           as module       // Range of elements (and range arithmetic)
ITERATOR        as module       // Iterator

UNSAFE          as module       // Unsafe data types
