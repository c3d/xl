// *****************************************************************************
// xl.types.enumerated.xs                                           XL project
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

module XL.TYPES.ENUMERATED[enumerated:type] with
// ----------------------------------------------------------------------------
//   Interface for operations on enumerated types
// ----------------------------------------------------------------------------

    use XL.OPERATIONS.COPY[enumerated]
    use XL.OPERATIONS.MOVE[enumerated]
    use XL.OPERATIONS.DELETE[enumerated]
    use XL.OPERATIONS.COMPARISON[enumerated]

    type representation like natural

    First               as enumerated
    Last                as enumerated
    Successor Value     as enumerated
    Predecessor Value   as enumerated


module XL.TYPES.ENUMERATED is
// ----------------------------------------------------------------------------
//   Defines the general enumerated type
// ----------------------------------------------------------------------------

    type enumerated where
        use XL.TYPES.ENUMERATED[enumerated]
