// ****************************************************************************
//  array.xs                                        XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for the `array` type
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

import SLICE
import STREAM


// The `array` type is a fixed-size collection of items of the same type
type array[item:type, index:enumerated, Range:range of index]

// Readable notation like `array[1..5] of integer`
array[Range:range] of item              is array[item, Range.value, Range]

// Readable notation like `array[2] of real`
array[Size:unsigned] of item            is array[0..Size-1] of item

// Type used to index the array
array.index                             is another array.size

// Index range type
array.range                             is range of array.index

// Actual index range associated to an array type
array.Range                             as array.range

with
    Array       : array
    Index       : array.index
    MyArray     : my array
    Variable    : local array.item
do
    // The range type associated with an array value
    Array.range                         is array.range

    // The index range associated with an array value
    Array.Range                         as array.range

    // The size of an array
    Array.Size                          is array.Range.Size

    // Implicitly convert an array to a slice
    Array                               as slice[array.item]

    // Implicitly convert an array to a stream
    Array                               as stream[array.item]

    // Implicitly convert an owned array to an owned slice
    MyArray                             as my slice[array.item]

    // Index an array, returning an element by value
    Array[Index]                        as array.item

    // Index an owned array, returning an owned element
    MyArray[Index]                      as my array.item

    // Create an iterator over array value
    Variable in Array                   as iterator
