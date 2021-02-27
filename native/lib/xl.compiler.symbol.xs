// *****************************************************************************
// xl.compiler.symbol.xs                                              XL project
// *****************************************************************************
//
// File description:
//
//      Interface for a symbol table
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

SYMBOL with
// ----------------------------------------------------------------------------
//   Interface for the symbol table
// ----------------------------------------------------------------------------

    use ACCESS

    // Symbol table storing
    symbols                     : type with
        scope   : access[symbols]
        visible : string[symbols]

    // The current compilation context
    CONTEXT                     : symbols


    // Return true if Expr is defined in current context
    defined Expr                as boolean
