// ****************************************************************************
//  boolean.xs                                      XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     The basic boolean type, with two values, true and false
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

BOOLEAN has
// ----------------------------------------------------------------------------
//    Interface for the boolean type
// ----------------------------------------------------------------------------

    use ENUMERATION
    boolean is enumeration (false, true)

    use COMPARISON[boolean], BITWISE[boolean]
    use ASSIGN[boolean], COPY[boolean]
    use FORMAT[boolean]
