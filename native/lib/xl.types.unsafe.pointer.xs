// *****************************************************************************
// xl.types.unsafe.pointer.xs                                        XL project
// *****************************************************************************
//
// File description:
//
//     C-style machine-level pointers
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

use MEM = XL.SYSTEM.MEMORY

module POINTER[item:type] with
// ----------------------------------------------------------------------------
//   Interface for machine-level pointers
// ----------------------------------------------------------------------------

    type pointer                        like MEM.address
    type offset                         is MEM.offset
    type size                           is MEM.size

    *P:pointer                          as item

    (in out P:pointer)++                as pointer
    ++(in out P:pointer)                as pointer
    (in out P:pointer)--                as pointer
    --(in out P:pointer)                as pointer

    P:pointer + O:offset                as pointer
    P:pointer - O:offset                as pointer
    P:pointer - Q:pointer               as offset

    out x:pointer := y:pointer          as pointer

    in out x:pointer += y:offset        as pointer
    in out x:pointer -= y:offset        as pointer

    P:pointer[O:offset]                 as value

    P:pointer.Something                 is (*P).Something

    bit_size  pointer                   as size
    bit_align pointer                   as size
    bit_size  X:pointer                 as size
    bit_align X:pointer                 as size


module POINTER with
// ----------------------------------------------------------------------------
//   Interface to easily create pointer types
// ----------------------------------------------------------------------------

    type pointer[item:type]             is POINTER[item].pointer
    type pointer to item:type           is pointer[item]
