// ****************************************************************************
//  xl.math.xs                                      XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Basic math functions
//
//
//
//
//
//
//
//
// ****************************************************************************
//  (C) 2018 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

module XL.MATH with

    // Basic math functions
    abs   X:integer             as integer
    abs   X:real                as real
    sqrt  X:real                as real

    sin   X:real                as real
    cos   X:real                as real
    tan   X:real                as real
    asin  X:real                as real
    acos  X:real                as real
    atan  X:real                as real
    atan  Y:real, X:real        as real

    sinh  X:real                as real
    cosh  X:real                as real
    tanh  X:real                as real
    asinh X:real                as real
    acosh X:real                as real
    atanh X:real                as real

    exp   X:real                as real
    expm1 X:real                as real
    log   X:real                as real
    log10 X:real                as real
    log2  X:real                as real
    log1p X:real                as real

    pi                                          is 3.1415926535897932384626433
