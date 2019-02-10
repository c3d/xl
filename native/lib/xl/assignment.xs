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

interface XL.ASSIGNMENT[value:type] is
// ----------------------------------------------------------------------------
//    Assignment operations (copy and move)
// ----------------------------------------------------------------------------

    out x:value := y:value              as value // Copy
    out x:value <- in out y:value       as value // Move

    in out x:value += y:value           as value // Add to target
    in out x:value -= y:value           as value // Subtract from target
    in out x:value *= y:value           as value // Multiply with target
    in out x:value /= y:value           as value // Divide with target
