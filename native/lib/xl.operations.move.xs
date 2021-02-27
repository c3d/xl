// *****************************************************************************
// xl.operations.move.xs                                              XL project
// *****************************************************************************
//
// File description:
//
//    Specification for move operations
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

use XL.TYPES.IGNORABLE
use XL.TYPES.ERRORS

module XL.OPERATIONS.MOVE[moveable:type] with
// ----------------------------------------------------------------------------
//  Interface for move operations
// ----------------------------------------------------------------------------
//  A move transfers a value from the source to the target
//  It will `delete` the target if necessary
//  It will undefine the source and define the target with the value
//  This is the 'scissor operator' as it cuts the source

    with
        Target : out moveable
        Source : in out moveable

    Target :< Source    as ignorable moveable?
    Move(Source)                                is result :< Source


module XL.OPERATIONS.MOVE is
// ----------------------------------------------------------------------------
//  Implementation of the moveable type
// ----------------------------------------------------------------------------

    type moveable where
        use XL.OPERATIONS.MOVE[moveable]
