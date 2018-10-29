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
//  (C) 2018 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

module XL.ALIAS[type value] with

    type alias
    X:alias := Y:value          as value        is builtin Store
    X:alias                     as value        is builtin Load
