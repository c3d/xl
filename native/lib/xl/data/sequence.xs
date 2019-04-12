// *****************************************************************************
// sequence.xs                                                        XL project
// *****************************************************************************
//
// File description:
//
//     A sequence is a comma-separated list of items
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

use TYPE

SEQUENCE[item:type] has
// ----------------------------------------------------------------------------
//    Interface for sequences of items
// ----------------------------------------------------------------------------

    // A sequence is a comma-separated list of items
    sequence is either
        Head, Tail
        Item

    // Sequences with fixed type
    sequence[item:type] is either
        Head:item, Tail:seqence[item]
        Item:item
    sequence of item:type is sequence[item]

    // Sequences implement iterators
    use ITERATOR

    // Iterating over a typeless sequence
    X:var anything in Sequence:sequence         as iterator
