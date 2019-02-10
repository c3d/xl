// ****************************************************************************
//  slice.xs                                        XL - An extensible language
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

use TYPE, ACCESS, REFERENCE, RANGE, ITERATOR, ERROR

SLICE[item:type, access:type] has
// ----------------------------------------------------------------------------
//    Interface for slices, contiguous sequences of same-type elements
// ----------------------------------------------------------------------------

    // Types related to the slice
    size        is access.data_size
    index       is access.index
    index       is access.data_size
    range       is range of index

    // A slice holds the start of the run and its length
    slice : type with
        start   : access
        length  : size

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
