// ****************************************************************************
//  comparison.xs                                   XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Specification for comparisons between types
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

use BOOLEAN

COMPARISON[value:type] has
// ----------------------------------------------------------------------------
//    Specification for comparisons between values
// ----------------------------------------------------------------------------
//    Comparisons relate values of the same type.

    EQUALITY has
    // ------------------------------------------------------------------------
    //   Equality operators
    // ------------------------------------------------------------------------

        // Base operator
        X:value = Y:value       as boolean

        // Derived operator
        X:value <> Y:value      as boolean      is not X = Y


    IDENTITY has
    // ------------------------------------------------------------------------
    //   Identity operators (same object)
    // ------------------------------------------------------------------------

        // Base operator
        X:value == Y:value      as boolean

        // Derived operator
        X:value != Y:value      as boolean      is not X == Y


    ORDER has
    // ------------------------------------------------------------------------
    //   Total ordering between elements
    // ------------------------------------------------------------------------

        // Base operator
        X:value < Y:value       as boolean

        // Derived operator
        X:value > Y:value       as boolean      is     Y < X
        X:value >= Y:value      as boolean      is not Y < X
        X:value <= Y:value      as boolean      is not Y > X

        // Min and Max values
        Max X:value             as value        is X
        Max X:value, Rest       as value        is
            RestMax is Max Rest
            if RestMax < X then X else RestMax
        Min X:value             as value        is X
        Min X:value, Rest       as value        is
            RestMin is Min Rest
            if X < RestMin then X else RestMin


    // If you use XL.COMPARISON, you get both EQUALITY and ORDER by default
    use EQUALITY
    use ORDER
