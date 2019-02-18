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

import ENUMERATION
import NUMBER

module BOOLEAN with
// ----------------------------------------------------------------------------
//    Interface for the boolean type
// ----------------------------------------------------------------------------

    boolean is enumeration (false, true)

    use NUMBER[number is boolean, Size is 1, Align is 1, Kind is INTEGER]
