// ****************************************************************************
//  symbol.xs                                       XL - An extensible language
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

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
