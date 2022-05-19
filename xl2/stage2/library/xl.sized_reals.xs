// *****************************************************************************
// xl.sized_reals.xs                                                  XL project
// *****************************************************************************
//
// File description:
//
//
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
// (C) 2006,2008,2015, Christophe de Dinechin <christophe@dinechin.org>
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

module XL.SIZED_REALS with

    type real32                                                                                 is XL.BYTECODE.xlreal32
    type real64                                                                                 is XL.BYTECODE.xlreal64
    type real80                                                                                 is XL.BYTECODE.xlreal80

    // Integral types as used in conversions (avoid importing XL.SIZED_INTEGERS)
    type integer8                                                                               is XL.BYTECODE.xlint8
    type integer16                                                                              is XL.BYTECODE.xlint16
    type integer32                                                                              is XL.BYTECODE.xlint32
    type integer64                                                                              is XL.BYTECODE.xlint64
    type unsigned8                                                                              is XL.BYTECODE.xluint8
    type unsigned16                                                                             is XL.BYTECODE.xluint16
    type unsigned32                                                                             is XL.BYTECODE.xluint32
    type unsigned64                                                                             is XL.BYTECODE.xluint64


    function real(Value : real32) return real                                                   is XL.BYTECODE.real_from_real32
    function real(Value : real64) return real                                                   is XL.BYTECODE.real_from_real64
    function real(Value : real80) return real                                                   is XL.BYTECODE.real_from_real80

    function integer(Value : real32) return integer                                             is XL.BYTECODE.int_from_real32
    function integer(Value : real64) return integer                                             is XL.BYTECODE.int_from_real64
    function integer(Value : real80) return integer                                             is XL.BYTECODE.int_from_real80
    function integer8(Value : real32) return integer8                                           is XL.BYTECODE.int8_from_real32
    function integer8(Value : real64) return integer8                                           is XL.BYTECODE.int8_from_real64
    function integer8(Value : real80) return integer8                                           is XL.BYTECODE.int8_from_real80
    function integer16(Value : real32) return integer16                                         is XL.BYTECODE.int16_from_real32
    function integer16(Value : real64) return integer16                                         is XL.BYTECODE.int16_from_real64
    function integer16(Value : real80) return integer16                                         is XL.BYTECODE.int16_from_real80
    function integer32(Value : real32) return integer32                                         is XL.BYTECODE.int32_from_real32
    function integer32(Value : real64) return integer32                                         is XL.BYTECODE.int32_from_real64
    function integer32(Value : real80) return integer32                                         is XL.BYTECODE.int32_from_real80
    function integer64(Value : real32) return integer64                                         is XL.BYTECODE.int64_from_real32
    function integer64(Value : real64) return integer64                                         is XL.BYTECODE.int64_from_real64
    function integer64(Value : real80) return integer64                                         is XL.BYTECODE.int64_from_real80

    function unsigned(Value : real32) return unsigned                                           is XL.BYTECODE.uint_from_real32
    function unsigned(Value : real64) return unsigned                                           is XL.BYTECODE.uint_from_real64
    function unsigned(Value : real80) return unsigned                                           is XL.BYTECODE.uint_from_real80
    function unsigned8(Value : real32) return unsigned8                                         is XL.BYTECODE.uint8_from_real32
    function unsigned8(Value : real64) return unsigned8                                         is XL.BYTECODE.uint8_from_real64
    function unsigned8(Value : real80) return unsigned8                                         is XL.BYTECODE.uint8_from_real80
    function unsigned16(Value : real32) return unsigned16                                       is XL.BYTECODE.uint16_from_real32
    function unsigned16(Value : real64) return unsigned16                                       is XL.BYTECODE.uint16_from_real64
    function unsigned16(Value : real80) return unsigned16                                       is XL.BYTECODE.uint16_from_real80
    function unsigned32(Value : real32) return unsigned32                                       is XL.BYTECODE.uint32_from_real32
    function unsigned32(Value : real64) return unsigned32                                       is XL.BYTECODE.uint32_from_real64
    function unsigned32(Value : real80) return unsigned32                                       is XL.BYTECODE.uint32_from_real80
    function unsigned64(Value : real32) return unsigned64                                       is XL.BYTECODE.uint64_from_real32
    function unsigned64(Value : real64) return unsigned64                                       is XL.BYTECODE.uint64_from_real64
    function unsigned64(Value : real80) return unsigned64                                       is XL.BYTECODE.uint64_from_real80


    // Convert to real32
    function real32(Value : integer) return real32                                              is XL.BYTECODE.real32_from_int
    function real32(Value : integer8) return real32                                             is XL.BYTECODE.real32_from_int8
    function real32(Value : integer16) return real32                                            is XL.BYTECODE.real32_from_int16
    function real32(Value : integer32) return real32                                            is XL.BYTECODE.real32_from_int32
    function real32(Value : integer64) return real32                                            is XL.BYTECODE.real32_from_int64
    function real32(Value : unsigned8) return real32                                            is XL.BYTECODE.real32_from_uint8
    function real32(Value : unsigned16) return real32                                           is XL.BYTECODE.real32_from_uint16
    function real32(Value : unsigned32) return real32                                           is XL.BYTECODE.real32_from_uint32
    function real32(Value : unsigned64) return real32                                           is XL.BYTECODE.real32_from_uint64
    function real32(Value : real) return real32                                                 is XL.BYTECODE.real32_from_real
    function real32(Value : real32) return real32                                               is XL.BYTECODE.real32_from_real32
    function real32(Value : real64) return real32                                               is XL.BYTECODE.real32_from_real64
    function real32(Value : real80) return real32                                               is XL.BYTECODE.real32_from_real80

    // Convert to real64
    function real64(Value : integer) return real64                                              is XL.BYTECODE.real64_from_int
    function real64(Value : integer8) return real64                                             is XL.BYTECODE.real64_from_int8
    function real64(Value : integer16) return real64                                            is XL.BYTECODE.real64_from_int16
    function real64(Value : integer32) return real64                                            is XL.BYTECODE.real64_from_int32
    function real64(Value : integer64) return real64                                            is XL.BYTECODE.real64_from_int64
    function real64(Value : unsigned8) return real64                                            is XL.BYTECODE.real64_from_uint8
    function real64(Value : unsigned16) return real64                                           is XL.BYTECODE.real64_from_uint16
    function real64(Value : unsigned32) return real64                                           is XL.BYTECODE.real64_from_uint32
    function real64(Value : unsigned64) return real64                                           is XL.BYTECODE.real64_from_uint64
    function real64(Value : real) return real64                                                 is XL.BYTECODE.real64_from_real
    function real64(Value : real32) return real64                                               is XL.BYTECODE.real64_from_real32
    function real64(Value : real64) return real64                                               is XL.BYTECODE.real64_from_real64
    function real64(Value : real80) return real64                                               is XL.BYTECODE.real64_from_real80

    // Convert to real80
    function real80(Value : integer) return real80                                              is XL.BYTECODE.real80_from_int
    function real80(Value : integer8) return real80                                             is XL.BYTECODE.real80_from_int8
    function real80(Value : integer16) return real80                                            is XL.BYTECODE.real80_from_int16
    function real80(Value : integer32) return real80                                            is XL.BYTECODE.real80_from_int32
    function real80(Value : integer64) return real80                                            is XL.BYTECODE.real80_from_int64
    function real80(Value : unsigned8) return real80                                            is XL.BYTECODE.real80_from_uint8
    function real80(Value : unsigned16) return real80                                           is XL.BYTECODE.real80_from_uint16
    function real80(Value : unsigned32) return real80                                           is XL.BYTECODE.real80_from_uint32
    function real80(Value : unsigned64) return real80                                           is XL.BYTECODE.real80_from_uint64
    function real80(Value : real) return real80                                                 is XL.BYTECODE.real80_from_real
    function real80(Value : real32) return real80                                               is XL.BYTECODE.real80_from_real32
    function real80(Value : real64) return real80                                               is XL.BYTECODE.real80_from_real64
    function real80(Value : real80) return real80                                               is XL.BYTECODE.real80_from_real80


    // Sized real operations
    function Real32() return real32                                                             is XL.BYTECODE.zero_real32
    to Copy(out Tgt : real32; in Src : real32)                  written Tgt := Src              is XL.BYTECODE.copy_real32
    to Add(in out X : real32; in Y : real32)                    written X+=Y                    is XL.BYTECODE.adds_real32
    to Subtract(in out X : real32; in Y : real32)               written X-=Y                    is XL.BYTECODE.subs_real32
    to Multiply(in out X : real32; in Y : real32)               written X*=Y                    is XL.BYTECODE.muls_real32
    to Divide(in out X : real32; in Y : real32)                 written X/=Y                    is XL.BYTECODE.divs_real32
    function Negate(X : real32) return real32                   written -X                      is XL.BYTECODE.neg_real32
    function Add(X, Y : real32) return real32                   written X+Y                     is XL.BYTECODE.add_real32
    function Subtract(X, Y : real32) return real32              written X-Y                     is XL.BYTECODE.sub_real32
    function Multiply(X, Y : real32) return real32              written X*Y                     is XL.BYTECODE.mul_real32
    function Divide(X, Y : real32) return real32                written X/Y                     is XL.BYTECODE.div_real32
    function Modulo(X, Y : real32) return real32                written X mod Y                 is XL.BYTECODE.mod_real32
    function Remainder(X, Y : real32) return real32             written X rem Y                 is XL.BYTECODE.rem_real32
    function Power(X: real32; Y : unsigned) return real32       written X^Y                     is XL.BYTECODE.power_real32_uint
    function Power(X: real32; Y : integer) return real32        written X^Y                     is XL.BYTECODE.power_real32_int
    function Power(X: real32; Y : real32) return real32         written X^Y                     is XL.BYTECODE.power_real32_real32
    function Equal(X, Y : real32) return boolean                written X=Y                     is XL.BYTECODE.equ_real32
    function LessThan(X, Y : real32) return boolean             written X<Y                     is XL.BYTECODE.lt_real32
    function LessOrEqual(X, Y : real32) return boolean          written X<=Y                    is XL.BYTECODE.le_real32
    function GreaterThan(X, Y : real32) return boolean          written X>Y                     is XL.BYTECODE.gt_real32
    function GreaterOrEqual(X, Y : real32) return boolean       written X>=Y                    is XL.BYTECODE.ge_real32
    function Different(X, Y : real32) return boolean            written X<>Y                    is XL.BYTECODE.ne_real32

    function Real64() return real64                                                             is XL.BYTECODE.zero_real64
    to Copy(out Tgt : real64; in Src : real64)                  written Tgt := Src              is XL.BYTECODE.copy_real64
    to Add(in out X : real64; in Y : real64)                    written X+=Y                    is XL.BYTECODE.adds_real64
    to Subtract(in out X : real64; in Y : real64)               written X-=Y                    is XL.BYTECODE.subs_real64
    to Multiply(in out X : real64; in Y : real64)               written X*=Y                    is XL.BYTECODE.muls_real64
    to Divide(in out X : real64; in Y : real64)                 written X/=Y                    is XL.BYTECODE.divs_real64
    function Negate(X : real64) return real64                   written -X                      is XL.BYTECODE.neg_real64
    function Add(X, Y : real64) return real64                   written X+Y                     is XL.BYTECODE.add_real64
    function Subtract(X, Y : real64) return real64              written X-Y                     is XL.BYTECODE.sub_real64
    function Multiply(X, Y : real64) return real64              written X*Y                     is XL.BYTECODE.mul_real64
    function Divide(X, Y : real64) return real64                written X/Y                     is XL.BYTECODE.div_real64
    function Modulo(X, Y : real64) return real64                written X mod Y                 is XL.BYTECODE.mod_real64
    function Remainder(X, Y : real64) return real64             written X rem Y                 is XL.BYTECODE.rem_real64
    function Power(X: real64; Y : unsigned) return real64       written X^Y                     is XL.BYTECODE.power_real64_uint
    function Power(X: real64; Y : integer) return real64        written X^Y                     is XL.BYTECODE.power_real64_int
    function Power(X: real64; Y : real64) return real64         written X^Y                     is XL.BYTECODE.power_real64_real64
    function Equal(X, Y : real64) return boolean                written X=Y                     is XL.BYTECODE.equ_real64
    function LessThan(X, Y : real64) return boolean             written X<Y                     is XL.BYTECODE.lt_real64
    function LessOrEqual(X, Y : real64) return boolean          written X<=Y                    is XL.BYTECODE.le_real64
    function GreaterThan(X, Y : real64) return boolean          written X>Y                     is XL.BYTECODE.gt_real64
    function GreaterOrEqual(X, Y : real64) return boolean       written X>=Y                    is XL.BYTECODE.ge_real64
    function Different(X, Y : real64) return boolean            written X<>Y                    is XL.BYTECODE.ne_real64

    function Real80() return real80                                                             is XL.BYTECODE.zero_real80
    to Copy(out Tgt : real80; in Src : real80)                  written Tgt := Src              is XL.BYTECODE.copy_real80
    to Add(in out X : real80; in Y : real80)                    written X+=Y                    is XL.BYTECODE.adds_real80
    to Subtract(in out X : real80; in Y : real80)               written X-=Y                    is XL.BYTECODE.subs_real80
    to Multiply(in out X : real80; in Y : real80)               written X*=Y                    is XL.BYTECODE.muls_real80
    to Divide(in out X : real80; in Y : real80)                 written X/=Y                    is XL.BYTECODE.divs_real80
    function Negate(X : real80) return real80                   written -X                      is XL.BYTECODE.neg_real80
    function Add(X, Y : real80) return real80                   written X+Y                     is XL.BYTECODE.add_real80
    function Subtract(X, Y : real80) return real80              written X-Y                     is XL.BYTECODE.sub_real80
    function Multiply(X, Y : real80) return real80              written X*Y                     is XL.BYTECODE.mul_real80
    function Divide(X, Y : real80) return real80                written X/Y                     is XL.BYTECODE.div_real80
    function Modulo(X, Y : real80) return real80                written X mod Y                 is XL.BYTECODE.mod_real80
    function Remainder(X, Y : real80) return real80             written X rem Y                 is XL.BYTECODE.rem_real80
    function Power(X: real80; Y : unsigned) return real80       written X^Y                     is XL.BYTECODE.power_real80_uint
    function Power(X: real80; Y : integer) return real80        written X^Y                     is XL.BYTECODE.power_real80_int
    function Power(X: real80; Y : real80) return real80         written X^Y                     is XL.BYTECODE.power_real80_real80
    function Equal(X, Y : real80) return boolean                written X=Y                     is XL.BYTECODE.equ_real80
    function LessThan(X, Y : real80) return boolean             written X<Y                     is XL.BYTECODE.lt_real80
    function LessOrEqual(X, Y : real80) return boolean          written X<=Y                    is XL.BYTECODE.le_real80
    function GreaterThan(X, Y : real80) return boolean          written X>Y                     is XL.BYTECODE.gt_real80
    function GreaterOrEqual(X, Y : real80) return boolean       written X>=Y                    is XL.BYTECODE.ge_real80
    function Different(X, Y : real80) return boolean            written X<>Y                    is XL.BYTECODE.ne_real80

