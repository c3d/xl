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

ITERATOR[kind:flags(RESTARTABLE,                // Can restart iterator
                    BIDIRECTIONAL,              // Both directions
                    STEPPING)] with             // Can advance multiple steps
// ----------------------------------------------------------------------------
//    Interface for iterators
// ----------------------------------------------------------------------------

    // A basic type storing the state of an interator (mutable by default)
    iterator : type

    // Next step in an iterator
    Next I:iterator                             as Iterator
    ++I:iterator                                as iterator is Next I

    // Check if iterator completed
    Done I:iterator                             as boolean
    More I:iterator                             as boolean is not Done I
    I:iterator                                  as boolean is More I

    // Looping over an iterator
    for I:iterator loop Body                    as nil


    RESTARTABLE[iterator:like iterator] with
    // ------------------------------------------------------------------------
    //   An iterator that can be restarted
    // ------------------------------------------------------------------------
        Restart I:iterator                      as nil


    BIDIRECTIONAL[iterator:like iterator] with
    // ------------------------------------------------------------------------
    //   An iterator that can go forward or backward
    // ------------------------------------------------------------------------
        Previous I:iterator                     as iterator
        --I:iterator                            as iterator is Previous I


    // The step type is signed only for bidirectional iterators
    step is if kind.BIDIRECTIONAL then integer else unsigned


    STEPPING[iterator: like iterator,
             step    : like step is step] with
    // ------------------------------------------------------------------------
    //   An iterator that can advance by non-unity steps
    // ------------------------------------------------------------------------

        Advance I:iterator, By:step             as iterator
        I:iterator += S:step                    as iterator is Advance I, S

        for I:iterator by S:step loop Body      as nil


    use RESTARTABLE[iterator] when kind.RESTARTABLE
    use BIDIRECTIONAL[iterator] when kind.BIDIRECTIONAL
    use STEPPING[iterator, step] when kind.STEPPING


ITERATOR with
// ----------------------------------------------------------------------------
//   Default iterator is restatbale, bidirectional and stepping
// ----------------------------------------------------------------------------
    use ITERATOR[RESTARTABLE + BIDIRECTIONAL + STEPPING]
