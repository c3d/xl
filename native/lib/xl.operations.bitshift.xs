// *****************************************************************************
// xl.operations.bitshift.xs                                        XL project
// *****************************************************************************
//
// File description:
//
//     Interface for bit shifts
//
//     Bit shift move bits around
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

use MEM = XL.SYSTEM.MEMORY
use XL.TYPE.RANGE

module XL.OPERATIONS.BITSHIFT[shiftable:type] with
// ----------------------------------------------------------------------------
//   Specification for bit shift operations
// ----------------------------------------------------------------------------

    type bitcount               is natural range 0..MEM.BitSize(shiftable)

    with
        Value   : shiftable
        Left    : shiftable
        Right   : shiftable
        Target  : in out shiftable
        Shift   : bitcount
        Range   : range of bitcount

    // Shifts
    Left shl  Shift     as shiftable    // Shift left
    Left shr  Shift     as shiftable    // Shift right (based on signedness)
    Left ashr Shift     as shiftable    // Arithmetic (signed) shift right
    Left lshr Shift     as shiftable    // Logical shift right

    // Rotations
    Left rol  Shift     as shiftable    // Rotate left
    Left ror  Shift     as shiftable    // Rotate right

    // Endianness adjustments
    LittleEndian Value  as shiftable    // Byte swap if big-endian
    BigEndian Value     as shiftable    // Byte swap if little-endian

    CountOneBits  Value as bitcount
    CountZeroBits Value as bitcount
    FirstBitSet   Value as bitcount
    LastBitSet    Value as bitcount
    FirstBitClear Value as bitcount     is FirstBitSet(not Value)
    LastBitClear  Value as bitcount     is LastBitSet(not Value)

    // Extract a range of bit, e.g. 16#FF bits 1..3 is
    Value bits Range    as shiftable

    // 0 and 1 are valid shiftable values
    0                   as shiftable
    1                   as shiftable


module XL.OPERATIONS.BITSHIFT is
// ----------------------------------------------------------------------------
//   Define the shiftable generic type
// ----------------------------------------------------------------------------

    type shiftable where
        use XL.OPERATIONS.BITSHIFT[shiftable]
