// *****************************************************************************
// bitwise.xs                                                         XL project
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

type bitwise with
// ----------------------------------------------------------------------------
//   Specification for bitwise boolean operations
// ----------------------------------------------------------------------------

    with
        Value   : bitwise
        Left    : bitwise
        Right   : bitwise
        Owned   : own bitwise
        Shift   : bit_count
    do
        Left and  Right         as bitwise // Boolean and
        Left or   Right         as bitwise // Boolean or
        Left xor  Right         as bitwise // Boolean exclusive or
        Not Value               as bitwise // Bitwise not

        // Traditional C-style operator notations
        Left  &   Right         as bitwise      is Left and Right
        Left  |   Right         as bitwise      is Left or  Right
        Left  ^   Right         as bitwise      is Left xor Right
        ~Value                  as bitwise      is Not Value
        !Value                  as bitwise      is Not Value

        // In-place operators with a default implementation
        Owned &=  Right         as nil          is Owned := Owned & Right
        Owned |=  Right         as nil          is Owned := Owned | Right
        Owned ^=  Right         as nil          is Owned := Owned ^ Right
