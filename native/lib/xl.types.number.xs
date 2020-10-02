// *****************************************************************************
// xl.types.number.xs                                               XL project
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

module XL.TYPES.NUMBER[number:type] with
// ----------------------------------------------------------------------------
//    Type representing some kind of number (from integer to complex)
// ----------------------------------------------------------------------------

    use XL.OPERATIONS.ARITHMETIC[number]
    use XL.OPERATIONS.COPY[number]
    use XL.OPERATIONS.MOVE[number]
    use XL.OPERATIONS.DELETE[number]
    use XL.OPERATIONS.COMPARE[number]

    type number with
        min                     as number
        max                     as number
        epsilon X:number        as number

        as arithmetic
        as copiable
        as movable
        as clonable
        as deletable
        as comparable
        as sized
        as aligned
