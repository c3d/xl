// *****************************************************************************
// copy.xs                                                            XL project
// *****************************************************************************
//
// File description:
//
//     Interface for a type that can be copied
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

type copiable[type source is copiable] with
// ----------------------------------------------------------------------------
//    A type that can be copied
// ----------------------------------------------------------------------------
//    A copy makes a non-destructive copy of the source into the target
//    It will `delete` the target if necessary
//    It will define the target, but will not undefine the source
//    If copy is too expensive, consider using a move `:<` instead

    with
        Target  : out copiable
        Source  : source
    do
        Target := Source                as nil
        Copy Target, Source             as nil  is Target := Source

    // Indicate if copy can be made bitwise
    BITWISE_COPY                        as boolean is true
