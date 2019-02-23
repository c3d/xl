// ****************************************************************************
//  combinations.xs                                 XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for combinations of types
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

with
    T : type
    U : type
do

    // Return a type whose elements must belong to both input types
    T and U                     as type
    T  &  U                     as type         is T and U

    // Return a type whose elements belong to one of the input types
    T or  U                     as type
    T  |  U                     as type         is T or U

    // Return a type whose elements belong to only one of the input types
    T xor U                     as type         is (T or U) - (T and U)
    T  ^  U                     as type         is T xor U

    // Return a type which only has elements of first type not in second type
    T without U                 as type         is T and not U
    T  -  U                     as type         is T without U

    // Return a type whose elements are those that do not belong to input type
    not T:type                  as type
    ~T:type                     as type         is not T
    !T:type                     as type         is not T
