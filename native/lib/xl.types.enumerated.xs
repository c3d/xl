// *****************************************************************************
// enumerated.xs                                                      XL project
// *****************************************************************************
//
// File description:
//
//     A type covering all enumerated values
//
//     Like in Ada, enumerated values include integer types (signed or not)
//     as well as enumerations. They can be used among other things to
//     build discrete ranges, to index arrays, or for iterations
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

type enumerated with
// ----------------------------------------------------------------------------
//    Interface for enumerated types
// ----------------------------------------------------------------------------

    as COPY.copiable
    as MOVE.movable
    as CLONE.clonable
    as DELETE.deletable
    as COMPARISON.equatable
    as COMPARISION.ordered
    as MEMORY.sized
    as MEMORY.aligned

    First                               as enumerated
    Last                                as enumerated
    Successor   Value:enumerated        as enumerated
    Predecessor Value:enumerated        as enumerated

    // The underlying representation type
    representation                      as type like natural
