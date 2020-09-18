// *****************************************************************************
// move.xs                                                            XL project
// *****************************************************************************
//
// File description:
//
//    Specification for data move operation
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

type movable[type source is movable] with
// ----------------------------------------------------------------------------
//    A type that can be moved
// ----------------------------------------------------------------------------
//    A move transfers a value from the source to the target
//    It will `delete` the target if necessary
//    It will undefine the source and define the target with the value
//    This is the 'scissor operator' as it cuts the source

    with
        Target  : out movable
        Source  : in  source
    do
        Target :< Source                as nil
        Move Target, Source             as nil  is Target :< Source

    // Indicate if move can be made bitwise
    BITWISE_MOVE                        as boolean is true
