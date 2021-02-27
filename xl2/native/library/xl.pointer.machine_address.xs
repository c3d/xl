// *****************************************************************************
// xl.pointer.machine_address.xs                                      XL project
// *****************************************************************************
//
// File description:
//
//     Implementation of machine addresses
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
// (C) 2003-2004,2006-2007,2015,2020, Christophe de Dinechin <christophe@dinechin.org>
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

use XL.POINTER

module XL.POINTER.MACHINE_ADDRESS with

    function Pointer(addr : integer) return pointer                                    is XL.BYTECODE.int_to_ptr
    function Pointer(addr : unsigned) return pointer                                   is XL.BYTECODE.uint_to_ptr
    function Integer(ptr : pointer) return integer                                     is XL.BYTECODE.ptr_to_int
    function Unsigned(ptr : pointer) return unsigned                                   is XL.BYTECODE.ptr_to_uint

