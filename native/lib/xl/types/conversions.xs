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

use BOOLEAN
use INTEGER
use INTEGER
use REAL
use TEXT
use SYMBOL

module CONVERSIONS with
// ----------------------------------------------------------------------------
//    List of standard conversions
// ----------------------------------------------------------------------------

    implicit To, From is
    // ------------------------------------------------------------------------
    //    Declare an implicit conversion in `CONVERSIONS`
    // ------------------------------------------------------------------------
        CONVERSIONS.{use CONVERSIONS[To,From].IMPLICIT}


    explicit To, From is
    // ------------------------------------------------------------------------
    //    Declare an explicit conversion in `CONVERSIONS`
    // ------------------------------------------------------------------------
        CONVERSIONS.{use CONVERSIONS[To,From]}


    // Add explicit conversions, make them implicit if no data loss
    for I in INTEGER.types, UNSIGNED.types loop
        for J in INTEGER.types, UNSIGNED.types loop
            explicit I, J
            if I.BitSize >= J.BitSize and I.Signed = J.Signed then
                implicit I, J

        // Conversion from and to a real type are explicit unless no data loss
        for R in REAL.types loop
            explicit R, I
            explicit I, R
            if R.MantissaBits >= J.BitSize then
                implicit R, I

        // Conversion from and to character types, always explicit
        for C in CHARACTER.types loop
            explicit C, I
            explicit I, C

        // Conversion from and to text
        explicit text, I
        explicit I, text

        // Conversion from and to boolean type
        explicit boolean, I
        explicit I, boolean

    // Conversions between real types
    for R in REAL.types loop
        for S in REAL.types loop
            explicit R, S
            if R.MantissaBits >= S.MantissaBits then
                if R.ExponentBits >= S.ExponentBits then
                    implicit R, S

        explicit text, R
        explicit R, text
