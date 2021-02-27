// *****************************************************************************
// xl.operations.clone.xs                                             XL project
// *****************************************************************************
//
// File description:
//
//     Clone (deep copy) operation
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

type clonable[type source is clonable] with
// ----------------------------------------------------------------------------
//    A type that can be cloned
// ----------------------------------------------------------------------------
//    A clone is a deep copy of the source

    Clone Source:source                 as clonable

    // Indicate if cloning  can be made by simply copying the input
    CLONE_IS_COPY                       as boolean is true
