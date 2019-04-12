// *****************************************************************************
// string.xs                                                          XL project
// *****************************************************************************
//
// File description:
//
//     Interface for string operations
//
//     String are variable-length sequences of items with identical type
//     They own their contents, but slices can be extracted to reference
//     the content
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2019, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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

module STRING[item:type, access:type] with
// ----------------------------------------------------------------------------
//   Interface for string operations
// ----------------------------------------------------------------------------

    use SLICE[item, access]

    // The string type itself
    string : type with


    // Concatenation
    S1:slice & S2:slice         as text
    S :slice & C:character      as text
    C :character & S:slice      as text

    // Repeat a character
    C:character * S:size        as text
    S:size * C:character        as text


STRING has
// ----------------------------------------------------------------------------
//    Interface making it easy to create string types
// ----------------------------------------------------------------------------

    // Standard prefix notation for the general case
    slice [item:type, access:type]      is SLICE[item, type].slice

    // Common case where we use a reference to access items
    slice [item:type]                   is slice[item, ref item]

    // Convenience `slice of integer`  notation
    slice of item:type                  is slice[item]
