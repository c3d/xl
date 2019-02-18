// ****************************************************************************
//  sequence.xs                                     XL - An extensible language
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

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
