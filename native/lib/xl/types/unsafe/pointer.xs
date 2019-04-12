// *****************************************************************************
// pointer.xs                                                         XL project
// *****************************************************************************
//
// File description:
//
//     Machine low-level pointers
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
// (C) 2018-2019, Christophe de Dinechin <christophe@dinechin.org>
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

POINTER[item:type] has
// ----------------------------------------------------------------------------
//   Interface for machine-level pointers
// ----------------------------------------------------------------------------

    use XL.ALIAS[value]

    type pointer
    type offset
    type size

    *P:pointer                          as alias[value]

    in out P:pointer++                  as pointer
    ++in out P:pointer                  as pointer
    in out P:pointer--                  as pointer
    --in out P:pointer                  as pointer

    P:pointer + O:offset                as pointer
    P:pointer - O:offset                as pointer
    P:pointer - Q:pointer               as offset

    out x:pointer := y:pointer          as pointer

    in out x:pointer += y:offset        as pointer
    in out x:pointer -= y:offset        as pointer

    P:pointer[O:offset]                 as value

    P:pointer.(Name:name)               is (*P).Name

    bit_size  pointer                   as size
    bit_align pointer                   as size
    bit_size  X:pointer                 as size
    bit_align X:pointer                 as size


POINTER has
// ----------------------------------------------------------------------------
//   Interface to easily create pointer types
// ----------------------------------------------------------------------------

    pointer[item:type]                  is POINTER[item].pointer
    pointer to item:type                is pointer[item]
