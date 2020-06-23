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

type text[type character is UTF8] with
// ----------------------------------------------------------------------------
//    A basic representation for text
// ----------------------------------------------------------------------------

    as copiable
    as movable
    as clonable
    as deletable
    as comparable
    as sized
    as aligned

    type representation                 is string of character
    type slice                          is slice  of character

    with
        Text    : text
        Owned   : own text

    Length Text         as size         // Number of characters
    Size   Text         as size         // Number of bytes

    Text                as slice
