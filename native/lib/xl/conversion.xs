// ****************************************************************************
//  conversion.xs                                   XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for implicit or explicit conversions
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

CONVERSION[to_type : type, from_type : type] with
// ----------------------------------------------------------------------------
//    Interface implementing a conversion
// ----------------------------------------------------------------------------
    to_type F:from_type         as to_type


    IMPLICIT with
    // ------------------------------------------------------------------------
    //   Implicit conversion
    // ------------------------------------------------------------------------
        F:from_type             as to_type
