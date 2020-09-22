// *****************************************************************************
// text.xs                                                            XL project
// *****************************************************************************
//
// File description:
//
//     Interface for text functions
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

use SLICE
use STRING
use MEMORY

module XL.TYPES.TEXT[type character] with
// ----------------------------------------------------------------------------
//    A module handling the text data types
// ----------------------------------------------------------------------------

    // Represent text using a string of character
    type text                           is string of character

    // Slices of text are slices of the representation
    type slice                          is slice[text]

    with
        Text    : text
        Slice   : slice

    // Convert a text value to a slice. Most other operations use slices
    Text                as slice
