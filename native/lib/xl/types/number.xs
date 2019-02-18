// ****************************************************************************
//  number.xs                                       XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for describing numbers
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

use MEMORY, ENUMERATION, LIST, FLAG

module NUMBER[number    : type,
              Size      : bit_count,
              Align     : bit_count,
              Kind      : flag(INTEGER, SIGNED, FLOAT, FIXED, UNORDERED)] with
// ----------------------------------------------------------------------------
//   Interface for aspects common to all numbers
// ----------------------------------------------------------------------------

    number has
    // ------------------------------------------------------------------------
    //    Attributes for number types (in addition to basic type attributes)
    // ------------------------------------------------------------------------
        use type               // Expose all attributes of `type`
        use Kind               // Expose all `Kind` bits

        MinValue                as number
        MaxValue                as number
        Epsilon X:number        as number

    // Numbers support some kind of arithmetic
    use ARITHMETIC[number]

    // Bitwise arithmetic for integer numbers
    use BITWISE[number]         if Kind.INTEGER
    use ASSIGN[number]
    use COPY[number]

    // Comparisons have an order, except for ordered types
    use COMPARISON[number].EQUALITY
    use COMPARISON[number]      if not Kind.UNORDERED

    // Numbers have a fixed memory size
    use MEMORY.SIZE [number, Size]
    use MEMORY.ALIGN[number, Align]
