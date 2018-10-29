// ****************************************************************************
//  xl.real.xs                                      XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     The basic real number operations
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

module XL.REAL[value:type] is
    x:value + y:value           as value
    x:value - y:value           as value
    x:value * y:value           as value
    x:value / y:value           as value
    x:value rem y:value         as value
    x:value mod y:value         as value
    -x:value                    as value
    x:value ^ y:integer         as value
    x:value ^ y:value           as value

    X:real = Y:real             as boolean
    X:real <> Y:real            as boolean
    X:real > Y:real             as boolean
    X:real >= Y:real            as boolean
    X:real < Y:real             as boolean
    X:real <= Y:real            as boolean

    quiet_NaN                   as real
    signaling_NaN               as real

module XL.REAL is
    type real
    type real16
    type real32
    type real64
    type real80
    type real128

    using XL.REAL[real]
    using XL.REAL[real16]
    using XL.REAL[real32]
    using XL.REAL[real64]
    using XL.REAL[real80]
    using XL.REAL[real128]

    bit_size real                               is 64
    bit_size real16                             is 16
    bit_size real32                             is 32
    bit_size real64                             is 64
    bit_size real80                             is 80
    bit_size real128                            is 128

    bit_align real                              is 64
    bit_align real16                            is 16
    bit_align real32                            is 32
    bit_align real64                            is 64
    bit_align real80                            is 64
    bit_align real128                           is 128

    bit_size X:real                             is 64
    bit_size X:real16                           is 16
    bit_size X:real32                           is 32
    bit_size X:real64                           is 64
    bit_size X:real80                           is 80
    bit_size X:realr128                         is 128

    bit_align X:real                            is 64
    bit_align X:real16                          is 16
    bit_align X:real32                          is 32
    bit_align X:real64                          is 64
    bit_align X:real80                          is 64
    bit_align X:real128                         is 128
