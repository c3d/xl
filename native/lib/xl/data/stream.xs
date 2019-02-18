// ****************************************************************************
//  stream.xs                                       XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     A stream can produce a sequence of values with identical type
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

module STREAM[item:type] with
// ----------------------------------------------------------------------------
//    Generic stream interface
// ----------------------------------------------------------------------------

    type stream with
        type item               is item
    Next Stream:stream          as item


module STREAM with
// ----------------------------------------------------------------------------
//    Stream type constructors
// ----------------------------------------------------------------------------

    stream[item:type]           is STREAM[item].stream
    stream of item:type         is stream[item]
