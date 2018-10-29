// ****************************************************************************
//  xl.complex.xs                                   XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//      Basic complex arithmetic
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

module XL.COMPLEX[type value] with

    type complex is complex(re:value, im:value)

    X:complex + Y:complex       as complex
    X:complex - Y:complex       as complex
    X:complex * Y:complex       as complex
    X:complex / Y:complex       as complex

    X:complex + Y:value         as complex
    X:complex - Y:value         as complex
    X:complex * Y:value         as complex
    X:complex / Y:value         as complex

    length Z:complex            as value
    angle  Z:complex            as value
