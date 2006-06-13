// ****************************************************************************
//  xl.sized_integers.xs            (C) 1992-2006 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
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
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

module XL.SIZED_INTEGERS with

    type integer8                                                                               is XL.BYTECODE.xlint8
    type integer16                                                                              is XL.BYTECODE.xlint16
    type integer32                                                                              is XL.BYTECODE.xlint32
    type integer64                                                                              is XL.BYTECODE.xlint64
    type unsigned8                                                                              is XL.BYTECODE.xluint8
    type unsigned16                                                                             is XL.BYTECODE.xluint16
    type unsigned32                                                                             is XL.BYTECODE.xluint32
    type unsigned64                                                                             is XL.BYTECODE.xluint64

    // Convert to integer
    function integer(Value : integer8) return integer                                           is XL.BYTECODE.int8_to_int
    function integer(Value : integer16) return integer                                          is XL.BYTECODE.int16_to_int
    function integer(Value : integer32) return integer                                          is XL.BYTECODE.int32_to_int
    function integer(Value : integer64) return integer                                          is XL.BYTECODE.int64_to_int
    function integer(Value : unsigned8) return integer                                          is XL.BYTECODE.uint8_to_int
    function integer(Value : unsigned16) return integer                                         is XL.BYTECODE.uint16_to_int
    function integer(Value : unsigned32) return integer                                         is XL.BYTECODE.uint32_to_int
    function integer(Value : unsigned64) return integer                                         is XL.BYTECODE.uint64_to_int

    // Convert to integer8
    function integer8(Value : integer) return integer8                                          is XL.BYTECODE.int_to_int8
    function integer8(Value : integer8) return integer8                                         is XL.BYTECODE.int8_to_int8
    function integer8(Value : integer16) return integer8                                        is XL.BYTECODE.int16_to_int8
    function integer8(Value : integer32) return integer8                                        is XL.BYTECODE.int32_to_int8
    function integer8(Value : integer64) return integer8                                        is XL.BYTECODE.int64_to_int8
    function integer8(Value : unsigned) return integer8                                         is XL.BYTECODE.uint_to_int8
    function integer8(Value : unsigned8) return integer8                                        is XL.BYTECODE.uint8_to_int8
    function integer8(Value : unsigned16) return integer8                                       is XL.BYTECODE.uint16_to_int8
    function integer8(Value : unsigned32) return integer8                                       is XL.BYTECODE.uint32_to_int8
    function integer8(Value : unsigned64) return integer8                                       is XL.BYTECODE.uint64_to_int8
    function integer8(Value : character) return integer8                                        is XL.BYTECODE.char_to_int8
    function integer8(Value : boolean) return integer8                                          is XL.BYTECODE.bool_to_int8
    function integer8(Value : real) return integer8                                             is XL.BYTECODE.real_to_int8

    // Convert to integer16
    function integer16(Value : integer) return integer16                                        is XL.BYTECODE.int_to_int16
    function integer16(Value : integer8) return integer16                                       is XL.BYTECODE.int8_to_int16
    function integer16(Value : integer16) return integer16                                      is XL.BYTECODE.int16_to_int16
    function integer16(Value : integer32) return integer16                                      is XL.BYTECODE.int32_to_int16
    function integer16(Value : integer64) return integer16                                      is XL.BYTECODE.int64_to_int16
    function integer16(Value : unsigned) return integer16                                       is XL.BYTECODE.uint_to_int16
    function integer16(Value : unsigned8) return integer16                                      is XL.BYTECODE.uint8_to_int16
    function integer16(Value : unsigned16) return integer16                                     is XL.BYTECODE.uint16_to_int16
    function integer16(Value : unsigned32) return integer16                                     is XL.BYTECODE.uint32_to_int16
    function integer16(Value : unsigned64) return integer16                                     is XL.BYTECODE.uint64_to_int16
    function integer16(Value : character) return integer16                                      is XL.BYTECODE.char_to_int16
    function integer16(Value : boolean) return integer16                                        is XL.BYTECODE.bool_to_int16
    function integer16(Value : real) return integer16                                           is XL.BYTECODE.real_to_int16

    // Convert to integer32
    function integer32(Value : integer) return integer32                                        is XL.BYTECODE.int_to_int32
    function integer32(Value : integer8) return integer32                                       is XL.BYTECODE.int8_to_int32
    function integer32(Value : integer16) return integer32                                      is XL.BYTECODE.int16_to_int32
    function integer32(Value : integer32) return integer32                                      is XL.BYTECODE.int32_to_int32
    function integer32(Value : integer64) return integer32                                      is XL.BYTECODE.int64_to_int32
    function integer32(Value : unsigned) return integer32                                       is XL.BYTECODE.uint_to_int32
    function integer32(Value : unsigned8) return integer32                                      is XL.BYTECODE.uint8_to_int32
    function integer32(Value : unsigned16) return integer32                                     is XL.BYTECODE.uint16_to_int32
    function integer32(Value : unsigned32) return integer32                                     is XL.BYTECODE.uint32_to_int32
    function integer32(Value : unsigned64) return integer32                                     is XL.BYTECODE.uint64_to_int32
    function integer32(Value : character) return integer32                                      is XL.BYTECODE.char_to_int32
    function integer32(Value : boolean) return integer32                                        is XL.BYTECODE.bool_to_int32
    function integer32(Value : real) return integer32                                           is XL.BYTECODE.real_to_int32

    // Convert to integer64
    function integer64(Value : integer) return integer64                                        is XL.BYTECODE.int_to_int64
    function integer64(Value : integer8) return integer64                                       is XL.BYTECODE.int8_to_int64
    function integer64(Value : integer16) return integer64                                      is XL.BYTECODE.int16_to_int64
    function integer64(Value : integer32) return integer64                                      is XL.BYTECODE.int32_to_int64
    function integer64(Value : integer64) return integer64                                      is XL.BYTECODE.int64_to_int64
    function integer64(Value : unsigned) return integer64                                       is XL.BYTECODE.uint_to_int64
    function integer64(Value : unsigned8) return integer64                                      is XL.BYTECODE.uint8_to_int64
    function integer64(Value : unsigned16) return integer64                                     is XL.BYTECODE.uint16_to_int64
    function integer64(Value : unsigned32) return integer64                                     is XL.BYTECODE.uint32_to_int64
    function integer64(Value : unsigned64) return integer64                                     is XL.BYTECODE.uint64_to_int64
    function integer64(Value : character) return integer64                                      is XL.BYTECODE.char_to_int64
    function integer64(Value : boolean) return integer64                                        is XL.BYTECODE.bool_to_int64
    function integer64(Value : real) return integer64                                           is XL.BYTECODE.real_to_int64

    function unsigned(Value : integer8) return unsigned                                         is XL.BYTECODE.int8_to_uint
    function unsigned(Value : integer16) return unsigned                                        is XL.BYTECODE.int16_to_uint
    function unsigned(Value : integer32) return unsigned                                        is XL.BYTECODE.int32_to_uint
    function unsigned(Value : integer64) return unsigned                                        is XL.BYTECODE.int64_to_uint
    function unsigned(Value : unsigned8) return unsigned                                        is XL.BYTECODE.uint8_to_uint
    function unsigned(Value : unsigned16) return unsigned                                       is XL.BYTECODE.uint16_to_uint
    function unsigned(Value : unsigned32) return unsigned                                       is XL.BYTECODE.uint32_to_uint
    function unsigned(Value : unsigned64) return unsigned                                       is XL.BYTECODE.uint64_to_uint
    function unsigned(Value : character) return unsigned                                        is XL.BYTECODE.char_to_uint
    function unsigned(Value : boolean) return unsigned                                          is XL.BYTECODE.bool_to_uint
    function unsigned(Value : real) return unsigned                                             is XL.BYTECODE.real_to_uint


    // Convert to unsigned8
    function unsigned8(Value : integer) return unsigned8                                        is XL.BYTECODE.int_to_uint8
    function unsigned8(Value : integer8) return unsigned8                                       is XL.BYTECODE.int8_to_uint8
    function unsigned8(Value : integer16) return unsigned8                                      is XL.BYTECODE.int16_to_uint8
    function unsigned8(Value : integer32) return unsigned8                                      is XL.BYTECODE.int32_to_uint8
    function unsigned8(Value : integer64) return unsigned8                                      is XL.BYTECODE.int64_to_uint8
    function unsigned8(Value : unsigned) return unsigned8                                       is XL.BYTECODE.uint_to_uint8
    function unsigned8(Value : unsigned8) return unsigned8                                      is XL.BYTECODE.uint8_to_uint8
    function unsigned8(Value : unsigned16) return unsigned8                                     is XL.BYTECODE.uint16_to_uint8
    function unsigned8(Value : unsigned32) return unsigned8                                     is XL.BYTECODE.uint32_to_uint8
    function unsigned8(Value : unsigned64) return unsigned8                                     is XL.BYTECODE.uint64_to_uint8
    function unsigned8(Value : character) return unsigned8                                      is XL.BYTECODE.char_to_uint8
    function unsigned8(Value : boolean) return unsigned8                                        is XL.BYTECODE.bool_to_uint8
    function unsigned8(Value : real) return unsigned8                                           is XL.BYTECODE.real_to_uint8

    // Convert to unsigned16
    function unsigned16(Value : integer) return unsigned16                                      is XL.BYTECODE.int_to_uint16
    function unsigned16(Value : integer8) return unsigned16                                     is XL.BYTECODE.int8_to_uint16
    function unsigned16(Value : integer16) return unsigned16                                    is XL.BYTECODE.int16_to_uint16
    function unsigned16(Value : integer32) return unsigned16                                    is XL.BYTECODE.int32_to_uint16
    function unsigned16(Value : integer64) return unsigned16                                    is XL.BYTECODE.int64_to_uint16
    function unsigned16(Value : unsigned) return unsigned16                                     is XL.BYTECODE.uint_to_uint16
    function unsigned16(Value : unsigned8) return unsigned16                                    is XL.BYTECODE.uint8_to_uint16
    function unsigned16(Value : unsigned16) return unsigned16                                   is XL.BYTECODE.uint16_to_uint16
    function unsigned16(Value : unsigned32) return unsigned16                                   is XL.BYTECODE.uint32_to_uint16
    function unsigned16(Value : unsigned64) return unsigned16                                   is XL.BYTECODE.uint64_to_uint16
    function unsigned16(Value : character) return unsigned16                                    is XL.BYTECODE.char_to_uint16
    function unsigned16(Value : boolean) return unsigned16                                      is XL.BYTECODE.bool_to_uint16
    function unsigned16(Value : real) return unsigned16                                         is XL.BYTECODE.real_to_uint16

    // Convert to unsigned32
    function unsigned32(Value : integer) return unsigned32                                      is XL.BYTECODE.int_to_uint32
    function unsigned32(Value : integer8) return unsigned32                                     is XL.BYTECODE.int8_to_uint32
    function unsigned32(Value : integer16) return unsigned32                                    is XL.BYTECODE.int16_to_uint32
    function unsigned32(Value : integer32) return unsigned32                                    is XL.BYTECODE.int32_to_uint32
    function unsigned32(Value : integer64) return unsigned32                                    is XL.BYTECODE.int64_to_uint32
    function unsigned32(Value : unsigned) return unsigned32                                     is XL.BYTECODE.uint_to_uint32
    function unsigned32(Value : unsigned8) return unsigned32                                    is XL.BYTECODE.uint8_to_uint32
    function unsigned32(Value : unsigned16) return unsigned32                                   is XL.BYTECODE.uint16_to_uint32
    function unsigned32(Value : unsigned32) return unsigned32                                   is XL.BYTECODE.uint32_to_uint32
    function unsigned32(Value : unsigned64) return unsigned32                                   is XL.BYTECODE.uint64_to_uint32
    function unsigned32(Value : character) return unsigned32                                    is XL.BYTECODE.char_to_uint32
    function unsigned32(Value : boolean) return unsigned32                                      is XL.BYTECODE.bool_to_uint32
    function unsigned32(Value : real) return unsigned32                                         is XL.BYTECODE.real_to_uint32

    // Convert to unsigned64
    function unsigned64(Value : integer) return unsigned64                                      is XL.BYTECODE.int_to_uint64
    function unsigned64(Value : integer8) return unsigned64                                     is XL.BYTECODE.int8_to_uint64
    function unsigned64(Value : integer16) return unsigned64                                    is XL.BYTECODE.int16_to_uint64
    function unsigned64(Value : integer32) return unsigned64                                    is XL.BYTECODE.int32_to_uint64
    function unsigned64(Value : integer64) return unsigned64                                    is XL.BYTECODE.int64_to_uint64
    function unsigned64(Value : unsigned) return unsigned64                                     is XL.BYTECODE.uint_to_uint64
    function unsigned64(Value : unsigned8) return unsigned64                                    is XL.BYTECODE.uint8_to_uint64
    function unsigned64(Value : unsigned16) return unsigned64                                   is XL.BYTECODE.uint16_to_uint64
    function unsigned64(Value : unsigned32) return unsigned64                                   is XL.BYTECODE.uint32_to_uint64
    function unsigned64(Value : unsigned64) return unsigned64                                   is XL.BYTECODE.uint64_to_uint64
    function unsigned64(Value : character) return unsigned64                                    is XL.BYTECODE.char_to_uint64
    function unsigned64(Value : boolean) return unsigned64                                      is XL.BYTECODE.bool_to_uint64
    function unsigned64(Value : real) return unsigned64                                         is XL.BYTECODE.real_to_uint64

    function real(Value : integer8) return real                                                 is XL.BYTECODE.int8_to_real
    function real(Value : integer16) return real                                                is XL.BYTECODE.int16_to_real
    function real(Value : integer32) return real                                                is XL.BYTECODE.int32_to_real
    function real(Value : integer64) return real                                                is XL.BYTECODE.int64_to_real
    function real(Value : unsigned8) return real                                                is XL.BYTECODE.uint8_to_real
    function real(Value : unsigned16) return real                                               is XL.BYTECODE.uint16_to_real
    function real(Value : unsigned32) return real                                               is XL.BYTECODE.uint32_to_real
    function real(Value : unsigned64) return real                                               is XL.BYTECODE.uint64_to_real

    // Sized integer operations
    function Integer8() return integer8                                                         is XL.BYTECODE.zero_int8
    to Copy(out Tgt : integer8; in Src : integer8)              written Tgt := Src              is XL.BYTECODE.copy_int8
    to Add(in out X : integer8; in Y : integer8)                written X+=Y                    is XL.BYTECODE.adds_int8
    to Subtract(in out X : integer8; in Y : integer8)           written X-=Y                    is XL.BYTECODE.subs_int8
    to Multiply(in out X : integer8; in Y : integer8)           written X*=Y                    is XL.BYTECODE.muls_int8
    to Divide(in out X : integer8; in Y : integer8)             written X/=Y                    is XL.BYTECODE.divs_int8
    function Negate(X : integer8) return integer8               written -X                      is XL.BYTECODE.neg_int8
    function Add(X, Y : integer8) return integer8               written X+Y                     is XL.BYTECODE.add_int8
    function Subtract(X, Y : integer8) return integer8          written X-Y                     is XL.BYTECODE.sub_int8
    function Multiply(X, Y : integer8) return integer8          written X*Y                     is XL.BYTECODE.mul_int8
    function Divide(X, Y : integer8) return integer8            written X/Y                     is XL.BYTECODE.div_int8
    function Modulo(X, Y : integer8) return integer8            written X mod Y                 is XL.BYTECODE.mod_int8
    function Remainder(X, Y : integer8) return integer8         written X rem Y                 is XL.BYTECODE.rem_int8
    function Power(X : integer8; Y : unsigned) return integer8  written X^Y                     is XL.BYTECODE.power_int8
    function Equal(X, Y : integer8) return boolean              written X=Y                     is XL.BYTECODE.equ_int8
    function LessThan(X, Y : integer8) return boolean           written X<Y                     is XL.BYTECODE.lt_int8
    function LessOrEqual(X, Y : integer8) return boolean        written X<=Y                    is XL.BYTECODE.le_int8
    function GreaterThan(X, Y : integer8) return boolean        written X>Y                     is XL.BYTECODE.gt_int8
    function GreaterOrEqual(X, Y : integer8) return boolean     written X>=Y                    is XL.BYTECODE.ge_int8
    function Different(X, Y : integer8) return boolean          written X<>Y                    is XL.BYTECODE.ne_int8
    function BitwiseAnd(X, Y : integer8) return integer8        written X and Y                 is XL.BYTECODE.and_int8
    function BitwiseOr(X, Y : integer8) return integer8         written X or Y                  is XL.BYTECODE.or_int8
    function BitwiseXor(X, Y : integer8) return integer8        written X xor Y                 is XL.BYTECODE.xor_int8
    function BitwiseNot(X : integer8) return integer8           written not X                   is XL.BYTECODE.not_int8

    function Integer16() return integer16                                                       is XL.BYTECODE.zero_int16
    to Copy(out Tgt : integer16; in Src : integer16)            written Tgt := Src              is XL.BYTECODE.copy_int16
    to Add(in out X : integer16; in Y : integer16)              written X+=Y                    is XL.BYTECODE.adds_int16
    to Subtract(in out X : integer16; in Y : integer16)         written X-=Y                    is XL.BYTECODE.subs_int16
    to Multiply(in out X : integer16; in Y : integer16)         written X*=Y                    is XL.BYTECODE.muls_int16
    to Divide(in out X : integer16; in Y : integer16)           written X/=Y                    is XL.BYTECODE.divs_int16
    function Negate(X : integer16) return integer16             written -X                      is XL.BYTECODE.neg_int16
    function Add(X, Y : integer16) return integer16             written X+Y                     is XL.BYTECODE.add_int16
    function Subtract(X, Y : integer16) return integer16        written X-Y                     is XL.BYTECODE.sub_int16
    function Multiply(X, Y : integer16) return integer16        written X*Y                     is XL.BYTECODE.mul_int16
    function Divide(X, Y : integer16) return integer16          written X/Y                     is XL.BYTECODE.div_int16
    function Modulo(X, Y : integer16) return integer16          written X mod Y                 is XL.BYTECODE.mod_int16
    function Remainder(X, Y : integer16) return integer16       written X rem Y                 is XL.BYTECODE.rem_int16
    function Power(X: integer16; Y: unsigned) return integer16  written X^Y                     is XL.BYTECODE.power_int16
    function Equal(X, Y : integer16) return boolean             written X=Y                     is XL.BYTECODE.equ_int16
    function LessThan(X, Y : integer16) return boolean          written X<Y                     is XL.BYTECODE.lt_int16
    function LessOrEqual(X, Y : integer16) return boolean       written X<=Y                    is XL.BYTECODE.le_int16
    function GreaterThan(X, Y : integer16) return boolean       written X>Y                     is XL.BYTECODE.gt_int16
    function GreaterOrEqual(X, Y : integer16) return boolean    written X>=Y                    is XL.BYTECODE.ge_int16
    function Different(X, Y : integer16) return boolean         written X<>Y                    is XL.BYTECODE.ne_int16
    function BitwiseAnd(X, Y : integer16) return integer16      written X and Y                 is XL.BYTECODE.and_int16
    function BitwiseOr(X, Y : integer16) return integer16       written X or Y                  is XL.BYTECODE.or_int16
    function BitwiseXor(X, Y : integer16) return integer16      written X xor Y                 is XL.BYTECODE.xor_int16
    function BitwiseNot(X : integer16) return integer16         written not X                   is XL.BYTECODE.not_int16

    function Integer32() return integer32                                                       is XL.BYTECODE.zero_int32
    to Copy(out Tgt : integer32; in Src : integer32)            written Tgt := Src              is XL.BYTECODE.copy_int32
    to Add(in out X : integer32; in Y : integer32)              written X+=Y                    is XL.BYTECODE.adds_int32
    to Subtract(in out X : integer32; in Y : integer32)         written X-=Y                    is XL.BYTECODE.subs_int32
    to Multiply(in out X : integer32; in Y : integer32)         written X*=Y                    is XL.BYTECODE.muls_int32
    to Divide(in out X : integer32; in Y : integer32)           written X/=Y                    is XL.BYTECODE.divs_int32
    function Negate(X : integer32) return integer32             written -X                      is XL.BYTECODE.neg_int32
    function Add(X, Y : integer32) return integer32             written X+Y                     is XL.BYTECODE.add_int32
    function Subtract(X, Y : integer32) return integer32        written X-Y                     is XL.BYTECODE.sub_int32
    function Multiply(X, Y : integer32) return integer32        written X*Y                     is XL.BYTECODE.mul_int32
    function Divide(X, Y : integer32) return integer32          written X/Y                     is XL.BYTECODE.div_int32
    function Modulo(X, Y : integer32) return integer32          written X mod Y                 is XL.BYTECODE.mod_int32
    function Remainder(X, Y : integer32) return integer32       written X rem Y                 is XL.BYTECODE.rem_int32
    function Power(X: integer32; Y: unsigned) return integer32  written X^Y                     is XL.BYTECODE.power_int32
    function Equal(X, Y : integer32) return boolean             written X=Y                     is XL.BYTECODE.equ_int32
    function LessThan(X, Y : integer32) return boolean          written X<Y                     is XL.BYTECODE.lt_int32
    function LessOrEqual(X, Y : integer32) return boolean       written X<=Y                    is XL.BYTECODE.le_int32
    function GreaterThan(X, Y : integer32) return boolean       written X>Y                     is XL.BYTECODE.gt_int32
    function GreaterOrEqual(X, Y : integer32) return boolean    written X>=Y                    is XL.BYTECODE.ge_int32
    function Different(X, Y : integer32) return boolean         written X<>Y                    is XL.BYTECODE.ne_int32
    function BitwiseAnd(X, Y : integer32) return integer32      written X and Y                 is XL.BYTECODE.and_int32
    function BitwiseOr(X, Y : integer32) return integer32       written X or Y                  is XL.BYTECODE.or_int32
    function BitwiseXor(X, Y : integer32) return integer32      written X xor Y                 is XL.BYTECODE.xor_int32
    function BitwiseNot(X : integer32) return integer32         written not X                   is XL.BYTECODE.not_int32

    function Integer64() return integer64                                                       is XL.BYTECODE.zero_int64
    to Copy(out Tgt : integer64; in Src : integer64)            written Tgt := Src              is XL.BYTECODE.copy_int64
    to Add(in out X : integer64; in Y : integer64)              written X+=Y                    is XL.BYTECODE.adds_int64
    to Subtract(in out X : integer64; in Y : integer64)         written X-=Y                    is XL.BYTECODE.subs_int64
    to Multiply(in out X : integer64; in Y : integer64)         written X*=Y                    is XL.BYTECODE.muls_int64
    to Divide(in out X : integer64; in Y : integer64)           written X/=Y                    is XL.BYTECODE.divs_int64
    function Negate(X : integer64) return integer64             written -X                      is XL.BYTECODE.neg_int64
    function Add(X, Y : integer64) return integer64             written X+Y                     is XL.BYTECODE.add_int64
    function Subtract(X, Y : integer64) return integer64        written X-Y                     is XL.BYTECODE.sub_int64
    function Multiply(X, Y : integer64) return integer64        written X*Y                     is XL.BYTECODE.mul_int64
    function Divide(X, Y : integer64) return integer64          written X/Y                     is XL.BYTECODE.div_int64
    function Modulo(X, Y : integer64) return integer64          written X mod Y                 is XL.BYTECODE.mod_int64
    function Remainder(X, Y : integer64) return integer64       written X rem Y                 is XL.BYTECODE.rem_int64
    function Power(X: integer64; Y: unsigned) return integer64  written X^Y                     is XL.BYTECODE.power_int64
    function Equal(X, Y : integer64) return boolean             written X=Y                     is XL.BYTECODE.equ_int64
    function LessThan(X, Y : integer64) return boolean          written X<Y                     is XL.BYTECODE.lt_int64
    function LessOrEqual(X, Y : integer64) return boolean       written X<=Y                    is XL.BYTECODE.le_int64
    function GreaterThan(X, Y : integer64) return boolean       written X>Y                     is XL.BYTECODE.gt_int64
    function GreaterOrEqual(X, Y : integer64) return boolean    written X>=Y                    is XL.BYTECODE.ge_int64
    function Different(X, Y : integer64) return boolean         written X<>Y                    is XL.BYTECODE.ne_int64
    function BitwiseAnd(X, Y : integer64) return integer64      written X and Y                 is XL.BYTECODE.and_int64
    function BitwiseOr(X, Y : integer64) return integer64       written X or Y                  is XL.BYTECODE.or_int64
    function BitwiseXor(X, Y : integer64) return integer64      written X xor Y                 is XL.BYTECODE.xor_int64
    function BitwiseNot(X : integer64) return integer64         written not X                   is XL.BYTECODE.not_int64


    // Sized unsigned integer operations
    function Unsigned8() return unsigned8                                                       is XL.BYTECODE.zero_uint8
    to Copy(out Tgt : unsigned8; in Src : unsigned8)            written Tgt := Src              is XL.BYTECODE.copy_uint8
    to Add(in out X : unsigned8; in Y : unsigned8)              written X+=Y                    is XL.BYTECODE.adds_uint8
    to Subtract(in out X : unsigned8; in Y : unsigned8)         written X-=Y                    is XL.BYTECODE.subs_uint8
    to Multiply(in out X : unsigned8; in Y : unsigned8)         written X*=Y                    is XL.BYTECODE.muls_uint8
    to Divide(in out X : unsigned8; in Y : unsigned8)           written X/=Y                    is XL.BYTECODE.divs_uint8
    function Add(X, Y : unsigned8) return unsigned8             written X+Y                     is XL.BYTECODE.add_uint8
    function Subtract(X, Y : unsigned8) return unsigned8        written X-Y                     is XL.BYTECODE.sub_uint8
    function Multiply(X, Y : unsigned8) return unsigned8        written X*Y                     is XL.BYTECODE.mul_uint8
    function Divide(X, Y : unsigned8) return unsigned8          written X/Y                     is XL.BYTECODE.div_uint8
    function Modulo(X, Y : unsigned8) return unsigned8          written X mod Y                 is XL.BYTECODE.mod_uint8
    function Remainder(X, Y : unsigned8) return unsigned8       written X rem Y                 is XL.BYTECODE.rem_uint8
    function Power(X: unsigned8; Y: unsigned) return unsigned8  written X^Y                     is XL.BYTECODE.power_uint8
    function Equal(X, Y : unsigned8) return boolean             written X=Y                     is XL.BYTECODE.equ_uint8
    function LessThan(X, Y : unsigned8) return boolean          written X<Y                     is XL.BYTECODE.lt_uint8
    function LessOrEqual(X, Y : unsigned8) return boolean       written X<=Y                    is XL.BYTECODE.le_uint8
    function GreaterThan(X, Y : unsigned8) return boolean       written X>Y                     is XL.BYTECODE.gt_uint8
    function GreaterOrEqual(X, Y : unsigned8) return boolean    written X>=Y                    is XL.BYTECODE.ge_uint8
    function Different(X, Y : unsigned8) return boolean         written X<>Y                    is XL.BYTECODE.ne_uint8
    function BitwiseAnd(X, Y : unsigned8) return unsigned8      written X and Y                 is XL.BYTECODE.and_uint8
    function BitwiseOr(X, Y : unsigned8) return unsigned8       written X or Y                  is XL.BYTECODE.or_uint8
    function BitwiseXor(X, Y : unsigned8) return unsigned8      written X xor Y                 is XL.BYTECODE.xor_uint8
    function BitwiseNot(X : unsigned8) return unsigned8         written not X                   is XL.BYTECODE.not_uint8

    function Unsigned16() return unsigned16                                                     is XL.BYTECODE.zero_uint16
    to Copy(out Tgt : unsigned16; in Src : unsigned16)          written Tgt := Src              is XL.BYTECODE.copy_uint16
    to Add(in out X : unsigned16; in Y : unsigned16)            written X+=Y                    is XL.BYTECODE.adds_uint16
    to Subtract(in out X : unsigned16; in Y : unsigned16)       written X-=Y                    is XL.BYTECODE.subs_uint16
    to Multiply(in out X : unsigned16; in Y : unsigned16)       written X*=Y                    is XL.BYTECODE.muls_uint16
    to Divide(in out X : unsigned16; in Y : unsigned16)         written X/=Y                    is XL.BYTECODE.divs_uint16
    function Add(X, Y : unsigned16) return unsigned16           written X+Y                     is XL.BYTECODE.add_uint16
    function Subtract(X, Y : unsigned16) return unsigned16      written X-Y                     is XL.BYTECODE.sub_uint16
    function Multiply(X, Y : unsigned16) return unsigned16      written X*Y                     is XL.BYTECODE.mul_uint16
    function Divide(X, Y : unsigned16) return unsigned16        written X/Y                     is XL.BYTECODE.div_uint16
    function Modulo(X, Y : unsigned16) return unsigned16        written X mod Y                 is XL.BYTECODE.mod_uint16
    function Remainder(X, Y : unsigned16) return unsigned16     written X rem Y                 is XL.BYTECODE.rem_uint16
    function Power(X: unsigned16;Y:unsigned) return unsigned16  written X^Y                     is XL.BYTECODE.power_uint16
    function Equal(X, Y : unsigned16) return boolean            written X=Y                     is XL.BYTECODE.equ_uint16
    function LessThan(X, Y : unsigned16) return boolean         written X<Y                     is XL.BYTECODE.lt_uint16
    function LessOrEqual(X, Y : unsigned16) return boolean      written X<=Y                    is XL.BYTECODE.le_uint16
    function GreaterThan(X, Y : unsigned16) return boolean      written X>Y                     is XL.BYTECODE.gt_uint16
    function GreaterOrEqual(X, Y : unsigned16) return boolean   written X>=Y                    is XL.BYTECODE.ge_uint16
    function Different(X, Y : unsigned16) return boolean        written X<>Y                    is XL.BYTECODE.ne_uint16
    function BitwiseAnd(X, Y : unsigned16) return unsigned16    written X and Y                 is XL.BYTECODE.and_uint16
    function BitwiseOr(X, Y : unsigned16) return unsigned16     written X or Y                  is XL.BYTECODE.or_uint16
    function BitwiseXor(X, Y : unsigned16) return unsigned16    written X xor Y                 is XL.BYTECODE.xor_uint16
    function BitwiseNot(X : unsigned16) return unsigned16       written not X                   is XL.BYTECODE.not_uint16

    function Unsigned32() return unsigned32                                                     is XL.BYTECODE.zero_uint32
    to Copy(out Tgt : unsigned32; in Src : unsigned32)          written Tgt := Src              is XL.BYTECODE.copy_uint32
    to Add(in out X : unsigned32; in Y : unsigned32)            written X+=Y                    is XL.BYTECODE.adds_uint32
    to Subtract(in out X : unsigned32; in Y : unsigned32)       written X-=Y                    is XL.BYTECODE.subs_uint32
    to Multiply(in out X : unsigned32; in Y : unsigned32)       written X*=Y                    is XL.BYTECODE.muls_uint32
    to Divide(in out X : unsigned32; in Y : unsigned32)         written X/=Y                    is XL.BYTECODE.divs_uint32
    function Add(X, Y : unsigned32) return unsigned32           written X+Y                     is XL.BYTECODE.add_uint32
    function Subtract(X, Y : unsigned32) return unsigned32      written X-Y                     is XL.BYTECODE.sub_uint32
    function Multiply(X, Y : unsigned32) return unsigned32      written X*Y                     is XL.BYTECODE.mul_uint32
    function Divide(X, Y : unsigned32) return unsigned32        written X/Y                     is XL.BYTECODE.div_uint32
    function Modulo(X, Y : unsigned32) return unsigned32        written X mod Y                 is XL.BYTECODE.mod_uint32
    function Remainder(X, Y : unsigned32) return unsigned32     written X rem Y                 is XL.BYTECODE.rem_uint32
    function Power(X: unsigned32;Y:unsigned) return unsigned32  written X^Y                     is XL.BYTECODE.power_uint32
    function Equal(X, Y : unsigned32) return boolean            written X=Y                     is XL.BYTECODE.equ_uint32
    function LessThan(X, Y : unsigned32) return boolean         written X<Y                     is XL.BYTECODE.lt_uint32
    function LessOrEqual(X, Y : unsigned32) return boolean      written X<=Y                    is XL.BYTECODE.le_uint32
    function GreaterThan(X, Y : unsigned32) return boolean      written X>Y                     is XL.BYTECODE.gt_uint32
    function GreaterOrEqual(X, Y : unsigned32) return boolean   written X>=Y                    is XL.BYTECODE.ge_uint32
    function Different(X, Y : unsigned32) return boolean        written X<>Y                    is XL.BYTECODE.ne_uint32
    function BitwiseAnd(X, Y : unsigned32) return unsigned32    written X and Y                 is XL.BYTECODE.and_uint32
    function BitwiseOr(X, Y : unsigned32) return unsigned32     written X or Y                  is XL.BYTECODE.or_uint32
    function BitwiseXor(X, Y : unsigned32) return unsigned32    written X xor Y                 is XL.BYTECODE.xor_uint32
    function BitwiseNot(X : unsigned32) return unsigned32       written not X                   is XL.BYTECODE.not_uint32

    function Unsigned64() return unsigned64                                                     is XL.BYTECODE.zero_uint64
    to Copy(out Tgt : unsigned64; in Src : unsigned64)          written Tgt := Src              is XL.BYTECODE.copy_uint64
    to Add(in out X : unsigned64; in Y : unsigned64)            written X+=Y                    is XL.BYTECODE.adds_uint64
    to Subtract(in out X : unsigned64; in Y : unsigned64)       written X-=Y                    is XL.BYTECODE.subs_uint64
    to Multiply(in out X : unsigned64; in Y : unsigned64)       written X*=Y                    is XL.BYTECODE.muls_uint64
    to Divide(in out X : unsigned64; in Y : unsigned64)         written X/=Y                    is XL.BYTECODE.divs_uint64
    function Add(X, Y : unsigned64) return unsigned64           written X+Y                     is XL.BYTECODE.add_uint64
    function Subtract(X, Y : unsigned64) return unsigned64      written X-Y                     is XL.BYTECODE.sub_uint64
    function Multiply(X, Y : unsigned64) return unsigned64      written X*Y                     is XL.BYTECODE.mul_uint64
    function Divide(X, Y : unsigned64) return unsigned64        written X/Y                     is XL.BYTECODE.div_uint64
    function Modulo(X, Y : unsigned64) return unsigned64        written X mod Y                 is XL.BYTECODE.mod_uint64
    function Remainder(X, Y : unsigned64) return unsigned64     written X rem Y                 is XL.BYTECODE.rem_uint64
    function Power(X: unsigned64;Y:unsigned) return unsigned64  written X^Y                     is XL.BYTECODE.power_uint64
    function Equal(X, Y : unsigned64) return boolean            written X=Y                     is XL.BYTECODE.equ_uint64
    function LessThan(X, Y : unsigned64) return boolean         written X<Y                     is XL.BYTECODE.lt_uint64
    function LessOrEqual(X, Y : unsigned64) return boolean      written X<=Y                    is XL.BYTECODE.le_uint64
    function GreaterThan(X, Y : unsigned64) return boolean      written X>Y                     is XL.BYTECODE.gt_uint64
    function GreaterOrEqual(X, Y : unsigned64) return boolean   written X>=Y                    is XL.BYTECODE.ge_uint64
    function Different(X, Y : unsigned64) return boolean        written X<>Y                    is XL.BYTECODE.ne_uint64
    function BitwiseAnd(X, Y : unsigned64) return unsigned64    written X and Y                 is XL.BYTECODE.and_uint64
    function BitwiseOr(X, Y : unsigned64) return unsigned64     written X or Y                  is XL.BYTECODE.or_uint64
    function BitwiseXor(X, Y : unsigned64) return unsigned64    written X xor Y                 is XL.BYTECODE.xor_uint64
    function BitwiseNot(X : unsigned64) return unsigned64       written not X                   is XL.BYTECODE.not_uint64


    function character(Value : integer8) return character                                       is XL.BYTECODE.int8_to_char
    function character(Value : integer16) return character                                      is XL.BYTECODE.int16_to_char
    function character(Value : integer32) return character                                      is XL.BYTECODE.int32_to_char
    function character(Value : integer64) return character                                      is XL.BYTECODE.int64_to_char
    function character(Value : unsigned8) return character                                      is XL.BYTECODE.uint8_to_char
    function character(Value : unsigned16) return character                                     is XL.BYTECODE.uint16_to_char
    function character(Value : unsigned32) return character                                     is XL.BYTECODE.uint32_to_char
    function character(Value : unsigned64) return character                                     is XL.BYTECODE.uint64_to_char

    function boolean(Value : integer8) return boolean                                           is XL.BYTECODE.int8_to_bool
    function boolean(Value : integer16) return boolean                                          is XL.BYTECODE.int16_to_bool
    function boolean(Value : integer32) return boolean                                          is XL.BYTECODE.int32_to_bool
    function boolean(Value : integer64) return boolean                                          is XL.BYTECODE.int64_to_bool
    function boolean(Value : unsigned8) return boolean                                          is XL.BYTECODE.uint8_to_bool
    function boolean(Value : unsigned16) return boolean                                         is XL.BYTECODE.uint16_to_bool
    function boolean(Value : unsigned32) return boolean                                         is XL.BYTECODE.uint32_to_bool
    function boolean(Value : unsigned64) return boolean                                         is XL.BYTECODE.uint64_to_bool
