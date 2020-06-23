// *****************************************************************************
// combinations.xs                                                    XL project
// *****************************************************************************
//
// File description:
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
