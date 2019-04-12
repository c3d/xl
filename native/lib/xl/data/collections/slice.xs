// *****************************************************************************
// slice.xs                                                           XL project
// *****************************************************************************
//
// File description:
//
//     Interface for slices, contiguous sequence of same-type elements
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
// (C) 2019, Christophe de Dinechin <christophe@dinechin.org>
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

use TYPE, ACCESS, REFERENCE, RANGE, ITERATOR, ERROR

module SLICE[item:type, access:type] with
// ----------------------------------------------------------------------------
//    Interface for slices, contiguous sequences of same-type elements
// ----------------------------------------------------------------------------

    // Types related to the slice
    size        is access.size
    index       is access.index
    range       is range of index

    // A slice holds the start of the run and its length
    type slice with
        start   as access
        length  as size

    range_error : like error with
        Index   : index         // Faulty index
        Range   : range         // Expected range

    // Accessing elements in a slice
    S:slice[I:index]            as ref character or range_error
    S:slice[R:range]            as slice         or range_error

    // Iterator over a slice
    V:var item in R:range       as iterator


SLICE has
// ----------------------------------------------------------------------------
//    An interface making it easy to create slice types
// ----------------------------------------------------------------------------

    // Standard prefix notation for the general case
    slice [item:type, access:type]      is SLICE[item, type].slice

    // Common case where we use a reference to access items
    slice [item:type]                   is slice[item, ref item]

    // Convenience `slice of integer`  notation
    slice of item:type                  is slice[item]
