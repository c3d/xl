// ****************************************************************************
//  range.xs                                        XL - An extensible language
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

RANGE[value : type with COMPARISON[value]] has
// ----------------------------------------------------------------------------
//    Generic interface for ranges
// ----------------------------------------------------------------------------

    range : type with
        Low     : value
        High    : value

    Low:value..High:value                       as range

    First       R:range                         as value
    Last        R:range                         as value
    Length      R:range                         as value

    // Range arithmetic and lexicographic order on ranges
    use ARITHMETIC[range], COMPARISON[range]

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
