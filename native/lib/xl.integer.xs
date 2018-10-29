// ****************************************************************************
//  xl.integer.xs                                   XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for XL integer types
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

use XL.BOOLEAN
use XL.UNSIGNED
module XL.INTEGER[value:type] with
    x:value + y:value           as value
    x:value - y:value           as value
    x:value * y:value           as value
    x:value / y:value           as value
    x:value rem y:value         as value
    x:value mod y:value         as value
    x:value and y:value         as value
    x:value or y:value          as value
    x:value xor y:value         as value
    x:value shl  y:unsigned     as value
    x:value shr  y:unsigned     as value
    x:value ashr y:unsigned     as value
    x:value lshr y:unsigned     as value
    -x:value                    as value
    not x:value                 as value
    x:value ^ y:unsigned        as value

    out x:value := y:value      as value

    in out x:value += y:value   as value
    in out x:value -= y:value   as value
    in out x:value *= y:value   as value
    in out x:value /= y:value   as value

    ++in out x:value            as value
    --in out x:value            as value
    in out x:value++            as value
    in out x:value--            as value

    X:value = Y:value           as boolean
    X:value <> Y:value          as boolean
    X:value > Y:value           as boolean
    X:value >= Y:value          as boolean
    X:value < Y:value           as boolean
    X:value <= Y:value          as boolean

module XL.INTEGER with
    type integer
    type integer1
    type integer8
    type integer16
    type integer32
    type integer64
    type integer128

    using XL.INTEGER[integer]
    using XL.INTEGER[integer1]
    using XL.INTEGER[integer8]
    using XL.INTEGER[integer16]
    using XL.INTEGER[integer32]
    using XL.INTEGER[integer64]
    using XL.INTEGER[integer128]

    bit_size integer                            is 64
    bit_size integer1                           is 1
    bit_size integer8                           is 8
    bit_size integer16                          is 16
    bit_size integer32                          is 32
    bit_size integer64                          is 64
    bit_size integer128                         is 128

    bit_align integer                           is 64
    bit_align integer1                          is 8
    bit_align integer8                          is 8
    bit_align integer16                         is 16
    bit_align integer32                         is 32
    bit_align integer64                         is 64
    bit_align integer128                        is 128

    bit_size X:integer                          is 64
    bit_size X:integer1                         is 1
    bit_size X:integer8                         is 8
    bit_size X:integer16                        is 16
    bit_size X:integer32                        is 32
    bit_size X:integer64                        is 64
    bit_size X:integer128                       is 128

    bit_align X:integer                         is 64
    bit_align X:integer1                        is 8
    bit_align X:integer8                        is 8
    bit_align X:integer16                       is 16
    bit_align X:integer32                       is 32
    bit_align X:integer64                       is 64
    bit_align X:integer128                      is 128
