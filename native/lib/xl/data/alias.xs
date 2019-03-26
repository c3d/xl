// ****************************************************************************
//  xl.alias.xs                                     XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     An alias can be used to access another entity
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

module XL.ALIAS[value:type] has
// ----------------------------------------------------------------------------
//    Interface for the module implementing aliases
// ----------------------------------------------------------------------------

    alias:type

    X:alias := Y:value          as value        is builtin Store
    X:alias                     as value        is builtin Load
