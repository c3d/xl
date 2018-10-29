// ****************************************************************************
//  xl.unsigned.xs                                 XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for XL unsigned types
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
module XL.UNSIGNED[value:type] with
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
    x:value ^ y:value           as value

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

module XL.UNSIGNED with
    type unsigned
    type unsigned1
    type unsigned8
    type unsigned16
    type unsigned32
    type unsigned64
    type unsigned128

    using XL.UNSIGNED[unsigned]
    using XL.UNSIGNED[unsigned1]
    using XL.UNSIGNED[unsigned8]
    using XL.UNSIGNED[unsigned16]
    using XL.UNSIGNED[unsigned32]
    using XL.UNSIGNED[unsigned64]
    using XL.UNSIGNED[unsigned128]

    bit_size unsigned                           is 64
    bit_size unsigned1                          is 1
    bit_size unsigned8                          is 8
    bit_size unsigned16                         is 16
    bit_size unsigned32                         is 32
    bit_size unsigned64                         is 64
    bit_size unsigned128                        is 128

    bit_align unsigned                          is 64
    bit_align unsigned1                         is 8
    bit_align unsigned8                         is 8
    bit_align unsigned16                        is 16
    bit_align unsigned32                        is 32
    bit_align unsigned64                        is 64
    bit_align unsigned128                       is 128

    bit_size X:unsigned                         is 64
    bit_size X:unsigned1                        is 1
    bit_size X:unsigned8                        is 8
    bit_size X:unsigned16                       is 16
    bit_size X:unsigned32                       is 32
    bit_size X:unsigned64                       is 64
    bit_size X:unsigned128                      is 128

    bit_align X:unsigned                        is 64
    bit_align X:unsigned1                       is 8
    bit_align X:unsigned8                       is 8
    bit_align X:unsigned16                      is 16
    bit_align X:unsigned32                      is 32
    bit_align X:unsigned64                      is 64
    bit_align X:unsigned128                     is 128
