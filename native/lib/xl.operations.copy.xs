// *****************************************************************************
// xl.operations.copy.xs                                            XL project
// *****************************************************************************
//
// File description:
//
//     Interface for a type that can be copied
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

use XL.TYPES.IGNORABLE
use XL.TYPES.ERRORS

module XL.OPERATIONS.COPY[copiable:type] with
// ----------------------------------------------------------------------------
//  Interface for copy operations
// ----------------------------------------------------------------------------

    with
        Target : out copiable
        Source : in copiable

    Target :+ Source    as ignorable copiable?
    Copy(Source)                                is result :+ Source


module XL.OPERATIONS.COPY is
// ----------------------------------------------------------------------------
//  Implementation of the copiable type
// ----------------------------------------------------------------------------

    type copiable where
        use XL.OPERATIONS.COPY[copiable]
