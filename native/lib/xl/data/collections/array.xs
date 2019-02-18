// ****************************************************************************
//  array.xs                                        XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for the `array` type
//
//     An array is a fixed-size collection of items of the same type,
//     which is guaranteed to be contiguous in memory
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

// Size of an array type, i.e. number of elements
array.Size                              as array.range.size

// Actual index range associated to an array type
array.Range                             as array.range

with
    Array       : array
    Index       : array.index
    OwnArray    : own array
    Variable    : local array.item
do
    // The range type associated with an array value
    Array.range                         is array.range

    // The index range associated with an array value
    Array.Range                         as array.range

    // The size of an array
    Array.Size                          is array.Range.Size

    // Implicitly convert an owned array to an owned slice
    OwnArray                            as own slice[array.item]

    // Index an array, returning an element by value
    Array[Index]                        as array.item

    // Index an owned array, returning an owned element
    OwnArray[Index]                     as own array.item

    // Create an iterator over array value
    Variable in Array                   as iterator
