// *****************************************************************************
// number.xs                                                          XL project
// *****************************************************************************
//
// File description:
//
//     Interface for describing numbers
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

use ARITHMETIC
use BITWISE
use COPY
use MOVE
use CLONE
use DELETE
use COMPARISON
use MEMORY


type number with
// ----------------------------------------------------------------------------
//    Type representing some kind of number (from integer to complex)
// ----------------------------------------------------------------------------

    // The 'number' type is primarily a shortcut for all interfaces below
    as ARITHMETIC.arithmetic
    as COPY.copiable
    as MOVE.movable
    as CLONE.clonable
    as DELETE.deletable
    as COMPARISON.comparable
    as MEMORY.sized
    as MEMORY.aligned

    // Range of values for the type
    MinValue                as number
    MaxValue                as number
    Epsilon Value:number    as number
