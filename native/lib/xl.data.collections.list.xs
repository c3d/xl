// *****************************************************************************
// xl.data.collections.list.xs                                        XL project
// *****************************************************************************
//
// File description:
//
//     Interface for lists
//
//     In XL, lists are comma-separated sequences of values
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2018-2020, Christophe de Dinechin <christophe@dinechin.org>
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

LIST[item:type] has
// ----------------------------------------------------------------------------
//    Interface for a list of items
// ----------------------------------------------------------------------------

    // Structure of a list
    list is either
        Head:item, Rest:list
        Item:item


LIST has
// ----------------------------------------------------------------------------
//    Interface for making it easy to create list types
// ----------------------------------------------------------------------------

    list[item:type]             is SUPER.LIST[item].list
    list of item:type           is list[item]
    list                        is list[anything]
