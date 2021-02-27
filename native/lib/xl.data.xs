// *****************************************************************************
// xl.data.xs                                                         XL project
// *****************************************************************************
//
// File description:
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
