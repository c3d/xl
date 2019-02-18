// ****************************************************************************
//  contract.xs                                     XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//      Interface for software contracts
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

use SOURCE
use COMPILER
use RUNTIME
use MODULE


module CONTRACT with
// ----------------------------------------------------------------------------
//    Contracts between entities
// ----------------------------------------------------------------------------

    // Require that a module be present (typically used in generic code)
    require Module:source[module]               as COMPILER.error or nil

    // Precondition for a function
    assume Condition:source[boolean]            as source[nil]

    // Postcondition for a function
    ensure Condition:source[boolean]            as source[nil]

    // General assertion for all other conditions
    assert Condition:source[boolean]            as source[nil]
