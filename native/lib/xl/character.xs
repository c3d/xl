// *****************************************************************************
// character.xs                                                       XL project
// *****************************************************************************
//
// File description:
//
//     Interface for character data types
//
//
//
//
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

type character[Bits] with
// ----------------------------------------------------------------------------
//    Generic interface for character types
// ----------------------------------------------------------------------------

    as ENUMERTATED.enumerated
    as COPY.copiable
    as MOVE.movable
    as CLONE.clonable
    as DELETE.deletable
    as COMPARISON.equatable
    as COMPARISON.ordered
    as MEMORY.sized
    as MEMORY.aligned


type ASCII              is character[7]
type UTF8               is character[8]
type UTF16              is character[16]
type UTF32              is character[32]
type character          is UTF32