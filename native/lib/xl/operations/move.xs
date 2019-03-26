// ****************************************************************************
//  move.xs                                         XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//    Specification for data move operation
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

type movable[type source is movable] with
// ----------------------------------------------------------------------------
//    A type that can be moved
// ----------------------------------------------------------------------------
//    A move transfers a value from the source to the target
//    It will `delete` the target if necessary
//    It will undefine the source and define the target with the value
//    This is the 'scissor operator' as it cuts the source

    with
        Target  : out movable
        Source  : in  source
    do
        Target :< Source                as nil
        Move Target, Source             as nil  is Target :< Source

    // Indicate if move can be made bitwise
    BITWISE_MOVE                        as boolean is true
