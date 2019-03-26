// ****************************************************************************
//  data.xs                                         XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Data structures
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

module RECORD               // Records for holding data
module MODULE               // Module interface, specification, etc
module PACKED               // Packed data types
module ENUMERATION          // Enumerations
module FLAGS                // Boolean flags
module ACCESS               // Access to other data (refs, pointers, etc)
module POINTER              // Raw machine-level pointers
module REFERENCE            // Reference to another object (including counted)
module OWN                  // Owning pointer
module BOX                  // Boxing items
module TUPLE                // Tuples of values
module ATOMIC               // Data types with atomic operations
module SEQUENCE             // Sequence of items
module CONTAINER            // Container of items
module ARRAY                // Fixed-size sequence of same-type items
module STRING               // Variable-size sequence of items
module SLICE                // Slice of contiguous items
module LIST                 // List of items
module SET                  // Set of unique values
module STREAM               // Stream of items
module MAP                  // Mapping values
module REDUCE               // Reducing values
module FILTER               // Filtering values
