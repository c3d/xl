// ****************************************************************************
//  assignment.xs                                   XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Specification for copy, move and arithmetic operations
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

use ARITHMETIC

type copiable[type source is copiable] with
// ----------------------------------------------------------------------------
//    Copy assignment
// ----------------------------------------------------------------------------
//    A copy assignment makes a copy of the source into the target.
//    It will `delete` the target if necessary
//    It will `copy` the source if necessary, which may be expensive
//    If copy is too expensive, consider using a move `<-` instead

    with
        Target  : out copiable
        Source  : source
    do
        Target := Source                as nil

    // Indicate if copy can be made bitwise
    BITWISE_COPY                        as boolean is true


type movable[type source is movable] with
// ----------------------------------------------------------------------------
//    Move assignment
// ----------------------------------------------------------------------------
//    A move assignment transfers the copy from the source to the target
//    It will `delete` the target if necessary
//    It will undefine the source and define the target with the value

    with
        Target  : out movable
        Source  : in  source
    do
        Target <- Source                as nil

    // Indicate if move can be made bitwise
    BITWISE_MOVE                        as boolean is true
