// *****************************************************************************
// iterator.xs                                                        XL project
// *****************************************************************************
//
// File description:
//
//     Interface for iterators
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2019-2020, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

use FLAGS, INTEGER, BOOLEAN

ITERATOR has
// ----------------------------------------------------------------------------
//    Interface for iterators
// ----------------------------------------------------------------------------

    // A basic type storing the state of an interator (mutable by default)
    type iterator

    // Next step in an iterator
    Next Iteration:my iterator                  as nil

    // Check if iterator completed
    Done Iteration:iterator                     as boolean
    More Iteration:iterator                     as boolean is not Done Iteration

    // Implicit conversion from iterator to boolean
    Iteration:iterator                          as boolean is More Iteration

    // Looping over an iterator
    for It:iterator loop Body:code[Result:type] as Result


    RESTARTABLE has
    // ------------------------------------------------------------------------
    //   An iterator that can be restarted
    // ------------------------------------------------------------------------
        Restart Iterator:in out iterator        as nil


    BIDIRECTIONAL has
    // ------------------------------------------------------------------------
    //   An iterator that can go forward or backward
    // ------------------------------------------------------------------------
        Previous Iteration:in out iterator      as nil


    STEPPING[step:type] has
    // ------------------------------------------------------------------------
    //   An iterator that can advance in steps
    // ------------------------------------------------------------------------

        Advance Iteration:iterator by By:step   as iterator

        Iteration:iterator += Step:step         as iterator is
            Advance Iteration by Step

        if step like integer then
        Iteration:iterator -= Step:step         as iterator is
            Advance Iteration by -S

        for I:iterator by S:step loop Body
