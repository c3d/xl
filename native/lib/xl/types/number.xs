// ****************************************************************************
//  number.xs                                       XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for describing numbers
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

import ARITHMETIC
import BITWISE
import COPY
import MOVE
import CLONE
import DELETE
import COMPARISON
import MEMORY


type number with
// ----------------------------------------------------------------------------
//    Type representing some kind of number (from integer to complex)
// ----------------------------------------------------------------------------

    // The 'number' type is primarily a shortcut for all interfaces below
    as ARITHMETIC.arithmetic
    as COPY.copiable
    as MOVE.movable
    as CLONE.clonable
    as DELETE.deletable
    as COMPARISON.equatable
    as COMPARISON.ordered
    as MEMORY.sized
    as MEMORY.aligned

    // Range of values for the type
    MinValue                as number
    MaxValue                as number
    Epsilon X:number        as number
