// *****************************************************************************
// bitshift.xs                                                        XL project
// *****************************************************************************
//
// File description:
//
//     Interface for bit shifts
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

type bitshift with
// ----------------------------------------------------------------------------
//   Specification for bit shift operations
// ----------------------------------------------------------------------------

    type bit_count              is unsigned

    with
        Value   : bitshift
        Left    : bitshift
        Right   : bitshift
        Owned   : copiable and own bitshift
        Shift   : bit_count
    do
        // Shifts
        Left shl  Shift         as bitshift // Shift left
        Left shr  Shift         as bitshift // Shift right
        Left ashr Shift         as bitshift // Arithmetic (signed) shift right
        Left lshr Shift         as bitshift // Logical shift right

        // Rotations
        Left rol  Shift         as bitshift // Rotate left
        Left ror  Shift         as bitshift // Rotate right

        // Traditional C-style operator notations
        Left  <<  Shift         as bitshift      is Left shl Shift
        Left  >>  Shift         as bitshift      is Left shr Shift

        // In-place operators with a default implementation
        Owned <<= Shift         as nil          is Owned := Owned << Shift
        Owned >>= Shift         as nil          is Owned := Owned >> Shift

        // Endianness adjustments
        LittleEndian Value      as bitshift // Byte swap if big-endian
        BigEndian Value         as bitshift // Byte swap if little-endian

        CountOneBits     Value  as bit_count
        CountZeroBits    Value  as bit_count
        LowestBit        Value  as bit_count
        HighestBit       Value  as bit_count
