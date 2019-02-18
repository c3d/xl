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
//
// Comparisions usually represent mathematical equality or ordering.
// However, computer scientists sometimes chose to diverge from mathematics.
//
// Notably, IEEE-754, the most common standard for floating point, has
// very special cases for NaN values (Not a Number).
// If X is a NaN, no comparison returns true. So X = X is false, meaning
// that equality is not reflexive. But then X <> X is also false...
//
// IEEE-754 has the same problem with order relationships and NaN values
//
// This has two consequences for the design of XL libraries:
// 1) The properties of comparisons are described on a case-by-case basis
// 2) XL splits the `real` and `ieee` types (see REAL module)


use BOOLEAN, FLAGS

module COMPARISON[value:type] with
// ----------------------------------------------------------------------------
//    Specification for comparisons between values
// ----------------------------------------------------------------------------
//    Comparisons relate values of the same type.

    module EQUALITY with
    // ------------------------------------------------------------------------
    //   Equality operators
    // ------------------------------------------------------------------------

        // Base operator
        X:value = Y:value       as boolean

        // Derived operator
        X:value <> Y:value      as boolean

        // Indicates if equality is symmetric, transitive and reflexive
        SYMMETRIC               as boolean // X = Y  => Y = X
        REFLEXIVE               as boolean // X = X is true
        TRANSITIVE              as boolean // X = Y and Y = Z => X = Z
        EXCLUSIVE               as boolean // (X=Y) and (X<>Y) are not both true
        TOTAL                   as boolean // (X = Y) xor (X <> Y) is true


    module IDENTITY with
    // ------------------------------------------------------------------------
    //   Identity operators (same entity for the computer)
    // ------------------------------------------------------------------------

        // Base operator
        X:value == Y:value      as boolean

        // Derived operator if not overriden
        X:value != Y:value      as boolean


    module ORDER with
    // ------------------------------------------------------------------------
    //   Total ordering between elements
    // ------------------------------------------------------------------------

        // Base operator
        X:value <= Y:value      as boolean

        // Derived operator
        X:value >= Y:value      as boolean
        X:value < Y:value       as boolean
        X:value > Y:value       as boolean

        // Min and Max values
        Max Args:list[value]    as value
        Min Args:list[value]    as value

        // Indicate what kind of order we are dealing with
        ANTISYMMETRIC           as boolean // X<=Y and Y<=X => X=Y
        TRANSITIVE              as boolean // X<=Y and Y<=Z => X<=Z
        CONNEX                  as boolean // X<=Y or Y<=X is true


    // If you use XL.COMPARISON, you get all modules by default
    use EQUALITY, IDENTITY, ORDER
