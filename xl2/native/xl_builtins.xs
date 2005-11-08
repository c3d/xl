// ****************************************************************************
//  xl_builtins.xs                  (C) 1992-2004 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//     This defines the built-in operations in XL
//
//     The module XL_BUILTINS is 'use'd at the beginning of every
//     translation unit. Since it is not a direct ancestor but
//     in a 'using' map, it is possible to redefine types named 'integer'
//
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

module XL_BUILTINS with

    // Bytecode type definitions
    // The following is defined in the compiler for this module
    // type module is XL.BYTECODE.xlmodule
    type integer                                                                                is XL.BYTECODE.xlint
    type integer8                                                                               is XL.BYTECODE.xlint8
    type integer16                                                                              is XL.BYTECODE.xlint16
    type integer32                                                                              is XL.BYTECODE.xlint32
    type integer64                                                                              is XL.BYTECODE.xlint64
    type unsigned                                                                               is XL.BYTECODE.xluint
    type unsigned8                                                                              is XL.BYTECODE.xluint8
    type unsigned16                                                                             is XL.BYTECODE.xluint16
    type unsigned32                                                                             is XL.BYTECODE.xluint32
    type unsigned64                                                                             is XL.BYTECODE.xluint64
    type real                                                                                   is XL.BYTECODE.xlreal
    type real32                                                                                 is XL.BYTECODE.xlreal32
    type real64                                                                                 is XL.BYTECODE.xlreal64
    type boolean                                                                                is XL.BYTECODE.xlbool
    type character                                                                              is XL.BYTECODE.xlchar
    type text                                                                                   is XL.BYTECODE.xltext
    type record                                                                                 is XL.BYTECODE.xlrecord


    // Integer operations
    to Copy(out Tgt : integer; in Src : integer)                written Tgt := Src              is XL.BYTECODE.copy_int
    function Negate(X : integer) return integer                 written -X                      is XL.BYTECODE.neg_int
    function Add(X, Y : integer) return integer                 written X+Y                     is XL.BYTECODE.add_int
    function Subtract(X, Y : integer) return integer            written X-Y                     is XL.BYTECODE.sub_int
    function Multiply(X, Y : integer) return integer            written X*Y                     is XL.BYTECODE.mul_int
    function Divide(X, Y : integer) return integer              written X/Y                     is XL.BYTECODE.div_int
    function Modulo(X, Y : integer) return integer              written X mod Y                 is XL.BYTECODE.mod_int
    function Remainder(X, Y : integer) return integer           written X rem Y                 is XL.BYTECODE.rem_int
    function Power(X : integer; Y : unsigned) return integer    written X^Y                     is XL.BYTECODE.power_int
    function Equal(X, Y : integer) return boolean               written X=Y                     is XL.BYTECODE.equ_int
    function LessThan(X, Y : integer) return boolean            written X<Y                     is XL.BYTECODE.lt_int
    function LessOrEqual(X, Y : integer) return boolean         written X<=Y                    is XL.BYTECODE.le_int
    function GreaterThan(X, Y : integer) return boolean         written X>Y                     is XL.BYTECODE.gt_int
    function GreaterOrEqual(X, Y : integer) return boolean      written X>=Y                    is XL.BYTECODE.ge_int
    function Different(X, Y : integer) return boolean           written X<>Y                    is XL.BYTECODE.ne_int
    function BitwiseAnd(X, Y : integer) return integer          written X and Y                 is XL.BYTECODE.and_int
    function BitwiseOr(X, Y : integer) return integer           written X or Y                  is XL.BYTECODE.or_int
    function BitwiseXor(X, Y : integer) return integer          written X xor Y                 is XL.BYTECODE.xor_int
    function BitwiseNot(X : integer) return integer             written not X                   is XL.BYTECODE.not_int

    // Sized integer operations
    to Copy(out Tgt : integer8; in Src : integer8)              written Tgt := Src              is XL.BYTECODE.copy_int8
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

    to Copy(out Tgt : integer16; in Src : integer16)            written Tgt := Src              is XL.BYTECODE.copy_int16
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

    to Copy(out Tgt : integer32; in Src : integer32)            written Tgt := Src              is XL.BYTECODE.copy_int32
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

    to Copy(out Tgt : integer64; in Src : integer64)            written Tgt := Src              is XL.BYTECODE.copy_int64
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


    // Unsigned integer operations
    to Copy(out Tgt : unsigned; in Src : unsigned)              written Tgt := Src              is XL.BYTECODE.copy_uint
    function Add(X, Y : unsigned) return unsigned               written X+Y                     is XL.BYTECODE.add_uint
    function Subtract(X, Y : unsigned) return unsigned          written X-Y                     is XL.BYTECODE.sub_uint
    function Multiply(X, Y : unsigned) return unsigned          written X*Y                     is XL.BYTECODE.mul_uint
    function Divide(X, Y : unsigned) return unsigned            written X/Y                     is XL.BYTECODE.div_uint
    function Modulo(X, Y : unsigned) return unsigned            written X mod Y                 is XL.BYTECODE.mod_uint
    function Remainder(X, Y : unsigned) return unsigned         written X rem Y                 is XL.BYTECODE.rem_uint
    function Power(X, Y : unsigned) return unsigned             written X^Y                     is XL.BYTECODE.power_uint
    function Equal(X, Y : unsigned) return boolean              written X=Y                     is XL.BYTECODE.equ_uint
    function LessThan(X, Y : unsigned) return boolean           written X<Y                     is XL.BYTECODE.lt_uint
    function LessOrEqual(X, Y : unsigned) return boolean        written X<=Y                    is XL.BYTECODE.le_uint
    function GreaterThan(X, Y : unsigned) return boolean        written X>Y                     is XL.BYTECODE.gt_uint
    function GreaterOrEqual(X, Y : unsigned) return boolean     written X>=Y                    is XL.BYTECODE.ge_uint
    function Different(X, Y : unsigned) return boolean          written X<>Y                    is XL.BYTECODE.ne_uint
    function BitwiseAnd(X, Y : unsigned) return unsigned        written X and Y                 is XL.BYTECODE.and_uint
    function BitwiseOr(X, Y : unsigned) return unsigned         written X or Y                  is XL.BYTECODE.or_uint
    function BitwiseXor(X, Y : unsigned) return unsigned        written X xor Y                 is XL.BYTECODE.xor_uint
    function BitwiseNot(X : unsigned) return unsigned           written not X                   is XL.BYTECODE.not_uint

    // Sized unsigned integer operations
    to Copy(out Tgt : unsigned8; in Src : unsigned8)            written Tgt := Src              is XL.BYTECODE.copy_uint8
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

    to Copy(out Tgt : unsigned16; in Src : unsigned16)          written Tgt := Src              is XL.BYTECODE.copy_uint16
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

    to Copy(out Tgt : unsigned32; in Src : unsigned32)          written Tgt := Src              is XL.BYTECODE.copy_uint32
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

    to Copy(out Tgt : unsigned64; in Src : unsigned64)          written Tgt := Src              is XL.BYTECODE.copy_uint64
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


    // Real operations
    to Copy(out Tgt : real; in Src : real)                      written Tgt := Src              is XL.BYTECODE.copy_real
    function Negate(X : real) return real                       written -X                      is XL.BYTECODE.neg_real
    function Add(X, Y : real) return real                       written X+Y                     is XL.BYTECODE.add_real
    function Subtract(X, Y : real) return real                  written X-Y                     is XL.BYTECODE.sub_real
    function Multiply(X, Y : real) return real                  written X*Y                     is XL.BYTECODE.mul_real
    function Divide(X, Y : real) return real                    written X/Y                     is XL.BYTECODE.div_real
    function Modulo(X, Y : real) return real                    written X mod Y                 is XL.BYTECODE.mod_real
    function Remainder(X, Y : real) return real                 written X rem Y                 is XL.BYTECODE.rem_real
    function Power(X: real; Y : unsigned) return real           written X^Y                     is XL.BYTECODE.power_real_uint
    function Power(X: real; Y : integer) return real            written X^Y                     is XL.BYTECODE.power_real_int
    function Power(X: real; Y : real) return real               written X^Y                     is XL.BYTECODE.power_real_real
    function Equal(X, Y : real) return boolean                  written X=Y                     is XL.BYTECODE.equ_real
    function LessThan(X, Y : real) return boolean               written X<Y                     is XL.BYTECODE.lt_real
    function LessOrEqual(X, Y : real) return boolean            written X<=Y                    is XL.BYTECODE.le_real
    function GreaterThan(X, Y : real) return boolean            written X>Y                     is XL.BYTECODE.gt_real
    function GreaterOrEqual(X, Y : real) return boolean         written X>=Y                    is XL.BYTECODE.ge_real
    function Different(X, Y : real) return boolean              written X<>Y                    is XL.BYTECODE.ne_real

    // Sized real operations
    to Copy(out Tgt : real32; in Src : real32)                  written Tgt := Src              is XL.BYTECODE.copy_real32
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

    to Copy(out Tgt : real64; in Src : real64)                  written Tgt := Src              is XL.BYTECODE.copy_real64
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


    // Boolean operations
    to Copy(out Tgt : boolean; in Src : boolean)                written Tgt := Src              is XL.BYTECODE.copy_bool
    function LogicalNot(X : boolean) return boolean             written not X                   is XL.BYTECODE.not_bool
    function LogicalAnd(X, Y : boolean) return boolean          written X and Y                 is XL.BYTECODE.and_bool
    function LogicalOr(X, Y : boolean) return boolean           written X or Y                  is XL.BYTECODE.or_bool
    function LogicalXor(X, Y : boolean) return boolean          written X xor Y                 is XL.BYTECODE.xor_bool


    // Character operations


    // Text operations


    // Records
