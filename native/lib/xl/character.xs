// ****************************************************************************
//  character.xs                                    XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for character data types
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

type character[Bits] with
// ----------------------------------------------------------------------------
//    Generic interface for character types
// ----------------------------------------------------------------------------

    as ENUMERTATED.enumerated
    as COPY.copiable
    as MOVE.movable
    as CLONE.clonable
    as DELETE.deletable
    as COMPARISON.equatable
    as COMPARISON.ordered
    as MEMORY.sized
    as MEMORY.aligned


type ASCII              is character[7]
type UTF8               is character[8]
type UTF16              is character[16]
type UTF32              is character[32]
type character          is UTF32
