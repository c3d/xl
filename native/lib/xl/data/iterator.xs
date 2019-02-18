// ****************************************************************************
//  iterator.xs                                     XL - An extensible language
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

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
