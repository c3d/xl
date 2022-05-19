// *****************************************************************************
// xl.pointer.heap.xs                                                 XL project
// *****************************************************************************
//
// File description:
//
//
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
// (C) 2007-2008,2015, Christophe de Dinechin <christophe@dinechin.org>
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

module XL.POINTER.HEAP with

    type memory_size is unsigned

    function Allocate(size : memory_size) return pointer is XL.BYTECODE.allocate_memory
    function New(value : pointer.item) return pointer is XL.BYTECODE.new_memory
    to Free (in out memory : pointer) is XL.BYTECODE.free_memory

    generic [type item]
    function Byte_Size(value : item) return memory_size is XL.BYTECODE.byte_size

    generic [type item]
    function Bit_Size(value : item) return memory_size is XL.BYTECODE.bit_size




