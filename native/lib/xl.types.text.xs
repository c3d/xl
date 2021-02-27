// *****************************************************************************
// xl.types.text.xs                                                   XL project
// *****************************************************************************
//
// File description:
//
//     Interface for the text data type and basic functions
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

use XL.TYPES.SLICE
use XL.TYPES.STRING
use XL.SYSTEM.MEMORY

module XL.TYPES.TEXT[character:type] with
// ----------------------------------------------------------------------------
//    Interface for the text data type
// ----------------------------------------------------------------------------

    // Text type
    type text                   like string of character

    // Slices of text are slices of the representation
    type slice                  like SLICE.slice[text]

    with
        Text    : text
        Slice   : slice

    // Convert a text value to a slice. Most other operations use slices
    Text                as slice
