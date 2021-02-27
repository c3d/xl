// *****************************************************************************
// xl.operations.bitwise.xs                                           XL project
// *****************************************************************************
//
// File description:
//
//     Specification for bitwise boolean operations
//
//     Bitwise operations apply on the individual bits in a data type.
//     Operations in this module are common to a variety of data types,
//     notably integer, natural and boolean, as well as arrays of such types
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

use MEM = XL.SYSTEM.MEMORY
use XL.TYPE.RANGE

module XL.OPERATIONS.BITWISE[bitwise:type] with
// ----------------------------------------------------------------------------
//   Specification for bitwise boolean operations
// ----------------------------------------------------------------------------

    type bitcount               is natural range 0..MEM.BitSize(bitwise)

    with
        Value   : in bitwise
        Left    : in bitwise
        Right   : in bitwise
        Target  : in out bitwise
        Range   : range of bitcount

    Left and  Right     as bitwise      // Boolean and
    Left or   Right     as bitwise      // Boolean or
    Left xor  Right     as bitwise      // Boolean exclusive or
    not Value           as bitwise      // Bitwise not

    // Traditional C-style operator notations
    Left  &   Right     as bitwise      is Left and Right
    Left  |   Right     as bitwise      is Left or  Right
    Left  ^   Right     as bitwise      is Left xor Right
    ~Value              as bitwise      is not Value
    !Value              as bitwise      is not Value

    // In-place operators with a default implementation
    Target &=  Right                    is Target := Target & Right
    Target |=  Right                    is Target := Target | Right
    Target ^=  Right                    is Target := Target ^ Right

    // Define a bit mask
    bitmask Range       as bitwise

    // 0 and 1 should always be convertible to bitwise
    0                   as bitwise
    1                   as bitwise


module XL.OPERATIONS.BITWISE is
// ----------------------------------------------------------------------------
//   Define the 'bitwise' generic type
// ----------------------------------------------------------------------------

    type bitwise where
        use XL.OPERTIONS.BITWISE[bitwise]
