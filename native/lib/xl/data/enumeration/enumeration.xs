// *****************************************************************************
// enumeration.xs                                                     XL project
// *****************************************************************************
//
// File description:
//
//     Definition of enumerations
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

use UNSIGNED

ENUMERATION[unsigned:type is unsigned] has
// ----------------------------------------------------------------------------
//   Interface for the enumeration types
// ----------------------------------------------------------------------------

    enumeration [type reptype] Body has
        type value

        text    Value:value     as text
        value   Text:text       as value
        reptype Value:value     as reptype
        value   Rep:reptype     as value

        enumeration_body Body,
            source First:name, Rep:reptype is
                source_code
                    [[First]] : value is reptype [[Rep]]

        enumeration_body Body, Code is
            Index : reptype := 0
            body Body
            body First, Rest is
                FirstSource is body First
                RestSource  is body Rest
                source_code
                    [[FirstSource]]
                    [[RestSource]]
            body First:name = Rep:reptype is
                Index := Rep
                body First
            body {First:name is Rep:reptype} is
                body First = Rep
            body First:name is
                Rep is Index++
                Code.source First, Rep

    use XL.UNSIGNED
    enumeration Body is enumeration[unsigned] Body
