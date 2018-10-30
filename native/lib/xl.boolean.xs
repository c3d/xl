// ****************************************************************************
//  xl.boolean.xs                                   XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     The basic boolean operations
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

module XL.BOOLEAN with
    type boolean
    false : boolean
    true  : boolean

    X:boolean = Y:boolean       as boolean
    X:boolean <> Y:boolean      as boolean
    X:boolean > Y:boolean       as boolean
    X:boolean >= Y:boolean      as boolean
    X:boolean < Y:boolean       as boolean
    X:boolean <= Y:boolean      as boolean
    X:boolean and Y:boolean     as boolean
    X:boolean or  Y:boolean     as boolean
    X:boolean xor Y:boolean     as boolean
    not X:boolean               as boolean
