// ****************************************************************************
//  type.xs                                         XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for types
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

TYPE with
// ----------------------------------------------------------------------------
//    Interface for the `TYPE` module
// ----------------------------------------------------------------------------

    use BITWISE, MEMORY, TEXT, BOOLEAN, SYMBOLS

    type with
    // ------------------------------------------------------------------------
    //   Interface for the `type` type
    // ------------------------------------------------------------------------
         name                   as text         // The name of the type if any
         Contains   Value       as boolean      // True if Value belongs
         BitSize    Value       as bit_count    // Number of bits
         BitAlign   Value       as bit_count    // Alignment in bits
         ByteSize   Value       as byte_count   // Number of bytes
         ByteAlign  Value       as byte_count   // Alignment in bytes

    // Indicate that `type` is itself a type
    type : type

    // Return the type associated to a value
    type Value                  as type

    // Return a derived type (with implicit conversion)
    like T:type                 as type

    // Return an incompatible derived type (no implicit conversion)
    new T:type                  as type

    // Return a type that is either one of the children forms
    either Forms                as type

    // Check if a type is like another type
    T1:type like T2:type        as boolean

    // Check if a type has some specific property
    T:type where Cond:boolean   as boolean
    T:type with  Cond           as boolean is T where defined Cond

    // Union, exclusive union and intersection of two types
    T1:type or  T2:type         as type
    T1:type xor T2:type         as type
    T1:type and T2:type         as type

    // A specific type holding no value
    nil                         : type

    // A specific type that can hold anything
    anything                    : type
