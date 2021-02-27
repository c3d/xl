// *****************************************************************************
// xl.program.contract.xs                                             XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2018-2020, Christophe de Dinechin <christophe@dinechin.org>
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
