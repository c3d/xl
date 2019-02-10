// ****************************************************************************
//  string.xs                                       XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for string operations
//
//     String are variable-length sequences of items with identical type
//     They own their contents, but slices can be extracted
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

STRING[item:type, access:type] has
// ----------------------------------------------------------------------------
//   Interface for string operations
// ----------------------------------------------------------------------------

    use SLICE[item, access]

    // The string type itself
    string : type with


    // Concatenation
    S1:slice & S2:slice         as text
    S :slice & C:character      as text
    C :character & S:slice      as text

    // Repeat a character
    C:character * S:size        as text
    S:size * C:character        as text


STRING has
// ----------------------------------------------------------------------------
//    Interface making it easy to create string types
// ----------------------------------------------------------------------------

    // Standard prefix notation for the general case
    slice [item:type, access:type]      is SLICE[item, type].slice

    // Common case where we use a reference to access items
    slice [item:type]                   is slice[item, ref item]

    // Convenience `slice of integer`  notation
    slice of item:type                  is slice[item]
