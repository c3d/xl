// ****************************************************************************
//  conversions.xs                                  XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for standard conversions
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

CONVERSIONS with
// ----------------------------------------------------------------------------
//    List of standard conversions
// ----------------------------------------------------------------------------

    use BOOLEAN, INTEGER, UNSIGNED, REAL, TEXT

    // Conversions with integer types
    for I in INTEGER.types, UNSIGNED.types loop

        // Conversion from another integer type
        for J in INTEGER.types, UNSIGNED.types loop

            // We always bring an explicit conversion.
            use CONVERSION[to_type is I, from_type is J]

            // We add implicit conversion if no data loss nor signedness change
            if I.size >= J.size and I.signed = J.signed then
                use CONVERSION[to_type is I, from_type is J].IMPLICIT

        // Conversion from and to a real type
        for R in REAL.types loop

            // Explicit conversion from integer to real
            use CONVERSION[to_type is R, from_type is I]

            // Explicit conversion from real to integer
            use CONVERSION[to_type is I, from_type is R]

            // Implicit conversion if size of mantissa fits value
