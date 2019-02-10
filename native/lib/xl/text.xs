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

TEXT[character:type] with
// ----------------------------------------------------------------------------
//   Generic interface for text functions
// ----------------------------------------------------------------------------

    use SLICE, STRING, MEMORY

    // 'text' inherits from a slice of characters
    slice                       is slice[character]
    text                        is string[character]

    use CLONE[text], COPY[text], MOVE[text], DELETE[text]
    use FORMAT[text]
