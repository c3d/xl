// *****************************************************************************
// xl.types.range.xs                                                  XL project
// *****************************************************************************
//
// File description:
//
//     Interface for ranges of values
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

use COMPARISON
use ITERATOR

module RANGE[value:type] with
// ----------------------------------------------------------------------------
//    Generic interface for ranges
// ----------------------------------------------------------------------------

    // Items in a range must be comparable
    require COMPARISON[value]

    // Definition of the range type
    type range with
        Low     as value
        High    as value
        Length  as value

    Low:value..High:value                       as range

    First       R:range                         as value
    Last        R:range                         as value
    Length      R:range                         as value

    // Range arithmetic and lexicographic order on ranges
    use ARITHMETIC[range]
    use COMPARISON[range]

    // Inherit some traits from the value type
    use COPY[range]   when COPY[value]
    use MOVE[range]   when MOVE[value]
    use ASSIGN[range] when ASSIGN[value]

    R1:range contains R2:range                  as boolean
    R1:range touches  R2:range                  as boolean
    R1:range inter    R2:range                  as range
    R1:range union    R2:range                  as range

    R1:range below    R2:range                  as boolean
    R1:range above    R2:range                  as boolean

    // Range iterator
    use ITERATOR
    V:var[value] in R:range                     as iterator

    // Boolean range checking
    V:value in R:range                          as boolean
    R:range contains V:value                    as boolean

    // Range offset and scaling
    R:range + Offset:value                      as range
    R:range - Offset:value                      as range
    R:range * Scale :value                      as range
    R:range / Scale :value                      as range

    Offset:value + R:range                      as range
    Scale :value * R:range                      as range


RANGE has
// ----------------------------------------------------------------------------
//    Common interface making it easy to build range types
// ----------------------------------------------------------------------------

    // Normal prefix notation
    range[value:type]   is RANGE[value].range

    // Convenience notation, e.g. `range of integer`
    range of value:type is range[value]
