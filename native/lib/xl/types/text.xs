// ****************************************************************************
//  text.xs                                         XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for text functions
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

import SLICE
import STRING
import MEMORY

type text[type character is UTF8] with
// ----------------------------------------------------------------------------
//    A basic representation for text
// ----------------------------------------------------------------------------

    as copiable
    as movable
    as clonable
    as deletable
    as comparable
    as sized
    as aligned

    type representation                 is string of character
    type slice                          is slice  of character

    with
        Text    : text
        Owned   : own text

    Length Text         as size         // Number of characters
    Size   Text         as size         // Number of bytes

    Text                as slice
