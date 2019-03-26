// ****************************************************************************
//  boolean.xs                                      XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     The boolean type, with two values, true and false
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

import ENUMERATED
import BITWISE
import COMPARISON


type boolean with
// ----------------------------------------------------------------------------
//    Description of the boolean type
// ----------------------------------------------------------------------------

    false                       as boolean
    true                        as boolean

    as ENUMERATED.enumerated
    as BITWISE.bitwise
    as COMPARISON.equatable
    as COMPARISON.ordered
