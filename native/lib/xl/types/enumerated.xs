// ****************************************************************************
//  enumerated.xs                                   XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     A type covering all enumerated values
//
//     Like in Ada, enumerated values include integer types (signed or not)
//     as well as enumerations. They can be used among other things to
//     build discrete ranges, to index arrays, or for iterations
//
//
//
//
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

type enumerated with
// ----------------------------------------------------------------------------
//    Interface for enumerated types
// ----------------------------------------------------------------------------

    as COPY.copiable
    as MOVE.movable
    as CLONE.clonable
    as DELETE.deletable
    as COMPARISON.equatable
    as COMPARISION.ordered
    as MEMORY.sized
    as MEMORY.aligned

    First                               as enumerated
    Last                                as enumerated
    Successor   Value:enumerated        as enumerated
    Predecessor Value:enumerated        as enumerated

    // The underlying representation type
    representation                      as type like unsigned
