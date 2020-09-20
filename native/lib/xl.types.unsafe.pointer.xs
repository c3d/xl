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
//   Part of the interface is only avaiable for fixed-size types, i.e.
//   types where the size of an item does not depend on its value

    type pointer                        like MEM.address
    type offset                         like MEM.offset
    type size                           like MEM.size
    FixedSize                           is MEM.FixedSizeType(item)

    with
        Pointer         : pointer
        Base            : pointer
        PointerVariable : in out pointer
        Target          : out pointer
        Source          : pointer
        Offset          : offset
        Value           : item


    *Pointer                            as item
    *Pointer := Value                   as item

    PointerVariable++                   as pointer
    ++PointerVariable                   as pointer
    PointerVariable--                   as pointer      when FixedSize
    --PointerVariable                   as pointer      when FixedSize

    Pointer + Offset                    as pointer
    Pointer - Offset                    as pointer      when FixedSize
    Pointer - Base                      as offset       when FixedSize

    Target := Source                    as pointer

    PointerVariable += Offset           as pointer
    PointerVariable -= Offset           as pointer      when FixedSize

    // Shortcuts for "array access" and "implicit dereference"
    Pointer[Offset]                     as value        is *(Pointer + Offset)
    Pointer.Something                                   is (*Pointer).Something


module POINTER is
// ----------------------------------------------------------------------------
//   A module to easily create pointer types
// ----------------------------------------------------------------------------

    type pointer[item:type]             is POINTER[item].pointer
    type pointer to item:type           is pointer[item]
