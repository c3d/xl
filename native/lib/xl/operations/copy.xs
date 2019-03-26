// ****************************************************************************
//  copy.xs                                         XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for a type that can be copied
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

type copiable[type source is copiable] with
// ----------------------------------------------------------------------------
//    A type that can be copied
// ----------------------------------------------------------------------------
//    A copy makes a non-destructive copy of the source into the target
//    It will `delete` the target if necessary
//    It will define the target, but will not undefine the source
//    If copy is too expensive, consider using a move `:<` instead

    with
        Target  : out copiable
        Source  : source
    do
        Target := Source                as nil
        Copy Target, Source             as nil  is Target := Source

    // Indicate if copy can be made bitwise
    BITWISE_COPY                        as boolean is true
