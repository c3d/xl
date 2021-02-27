// *****************************************************************************
// xl.system.memory.xs                                                XL project
// *****************************************************************************
//
// File description:
//
//     Interface for memory management
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

module XL.SYSTEM.MEMORY with
// ----------------------------------------------------------------------------
//   Interface for memory allocation and management
// ----------------------------------------------------------------------------

    type endianness             is one_of(BIG, LITTLE, STRANGE)
    type bit_count              is new natural
    type byte_count             is new natural
    type address                is new natural
    type offset                 is new integer
    type size                   is byte_count

    // Explicit conversions between bits and bytes
    byte_count(Bits:bit_count)  as byte_count
    bit_count(Bytes:byte_count) as bit_count

    // Size of values
    BitSize(Value)              as bit_count
    ByteSize(Value)             as byte_count is byte_count(BitSize(Value))
    BitAlign(Value)             as bit_count
    ByteAlign(Value)            as byte_count is byte_count(BitAlign(Value))

    // Memory allocation
    type heap
    Heap                        as heap
    Allocate(On:heap,Size:size) as address?
    Allocate(Size:size)         as address? is Allocate(Heap, Size)
    Free(Address:address)       as ok
