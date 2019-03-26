// ****************************************************************************
//  bitshift.xs                                     XL - An extensible language
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

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
