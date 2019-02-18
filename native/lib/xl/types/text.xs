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

module TEXT[character:type] with
// ----------------------------------------------------------------------------
//   Generic interface for text functions
// ----------------------------------------------------------------------------

    // A `text` is represented as a string of characters
    type text with
        character       is character
        slice           is slice of character
        Characters      as slice
        Length          is characters.Length

    // Implicit conversion from text to slice
    Text:text           as text.slice

    use CLONE[text]
    use COPY[text]
    use MOVE[text]
    use DELETE[text]
    use FORMAT[text]
