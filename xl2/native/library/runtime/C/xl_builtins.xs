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
// * Revision   : $Revision: 305 $
// * Date       : $Date: 2007-06-20 08:51:09 +0200 (Wed, 20 Jun 2007) $
// ****************************************************************************

module XL_BUILTINS with

    // ========================================================================
    // 
    //   Bytecode types
    // 
    // ========================================================================

    type integer                                                                                is XL.BYTECODE.xlint
    type unsigned                                                                               is XL.BYTECODE.xluint
    type real                                                                                   is XL.BYTECODE.xlreal
    type character                                                                              is XL.BYTECODE.xlchar
    type text                                                                                   is XL.BYTECODE.xltext
    type record                                                                                 is XL.BYTECODE.xlrecord
    type boolean                                                                                is enumeration (false, true) as XL.BYTECODE.xlbool 




    // The following is defined in the compiler for this module
    // type module is XL.BYTECODE.xlmodule



    // ========================================================================
    // 
    //   Signed integers
    // 
    // ========================================================================

    // Integer operations
    function Integer() return integer                                                           is XL.BYTECODE.zero_int
    to Copy(out Tgt : integer; in Src : integer)                written Tgt := Src              is XL.BYTECODE.copy_int
    to Add(in out X : integer; in Y : integer)                  written X+=Y                    is XL.BYTECODE.adds_int
    to Subtract(in out X : integer; in Y : integer)             written X-=Y                    is XL.BYTECODE.subs_int
    to Multiply(in out X : integer; in Y : integer)             written X*=Y                    is XL.BYTECODE.muls_int
    to Divide(in out X : integer; in Y : integer)               written X/=Y                    is XL.BYTECODE.divs_int
    function Negate(X : integer) return integer                 written -X                      is XL.BYTECODE.neg_int
    function Add(X, Y : integer) return integer                 written X+Y                     is XL.BYTECODE.add_int
    function Subtract(X, Y : integer) return integer            written X-Y                     is XL.BYTECODE.sub_int
    function Multiply(X, Y : integer) return integer            written X*Y                     is XL.BYTECODE.mul_int
    function Divide(X, Y : integer) return integer              written X/Y                     is XL.BYTECODE.div_int
    function Modulo(X, Y : integer) return integer              written X mod Y                 is XL.BYTECODE.mod_int
    function Remainder(X, Y : integer) return integer           written X rem Y                 is XL.BYTECODE.rem_int
    function Power(X : integer; Y : unsigned) return integer    written X^Y                     is XL.BYTECODE.power_int
    function Power(X : integer; Y : integer) return integer     written X^Y                     is XL.BYTECODE.power_int
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

    // ========================================================================
    // 
    //   Unsigned integer operations
    // 
    // ========================================================================
    // REVISIT: Would 'cardinal' or 'natural' be a better name than 'unsigned'?

    // Unsigned integer operations
    function Unsigned() return unsigned                                                         is XL.BYTECODE.zero_uint
    to Copy(out Tgt : unsigned; in Src : unsigned)              written Tgt := Src              is XL.BYTECODE.copy_uint
    to Add(in out X : unsigned; in Y : unsigned)                written X+=Y                    is XL.BYTECODE.adds_uint
    to Subtract(in out X : unsigned; in Y : unsigned)           written X-=Y                    is XL.BYTECODE.subs_uint
    to Multiply(in out X : unsigned; in Y : unsigned)           written X*=Y                    is XL.BYTECODE.muls_uint
    to Divide(in out X : unsigned; in Y : unsigned)             written X/=Y                    is XL.BYTECODE.divs_uint
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


    // ========================================================================
    // 
    //    Floating-point operations
    // 
    // ========================================================================

    // Real operations
    function Real() return real                                                                 is XL.BYTECODE.zero_real
    to Copy(out Tgt : real; in Src : real)                      written Tgt := Src              is XL.BYTECODE.copy_real
    to Add(in out X : real; in Y : real)                        written X+=Y                    is XL.BYTECODE.adds_real
    to Subtract(in out X : real; in Y : real)                   written X-=Y                    is XL.BYTECODE.subs_real
    to Multiply(in out X : real; in Y : real)                   written X*=Y                    is XL.BYTECODE.muls_real
    to Divide(in out X : real; in Y : real)                     written X/=Y                    is XL.BYTECODE.divs_real
    function Negate(X : real) return real                       written -X                      is XL.BYTECODE.neg_real
    function Add(X, Y : real) return real                       written X+Y                     is XL.BYTECODE.add_real
    function Subtract(X, Y : real) return real                  written X-Y                     is XL.BYTECODE.sub_real
    function Multiply(X, Y : real) return real                  written X*Y                     is XL.BYTECODE.mul_real
    function Divide(X, Y : real) return real                    written X/Y                     is XL.BYTECODE.div_real
    function Modulo(X, Y : real) return real                    written X mod Y                 is XL.BYTECODE.mod_real
    function Remainder(X, Y : real) return real                 written X rem Y                 is XL.BYTECODE.rem_real
    function Power(X: real; Y : unsigned) return real           written X^Y                     is XL.BYTECODE.power_uint_real
    function Power(X: real; Y : integer) return real            written X^Y                     is XL.BYTECODE.power_int_real
    function Power(X: real; Y : real) return real               written X^Y                     is XL.BYTECODE.power_real_real
    function Equal(X, Y : real) return boolean                  written X=Y                     is XL.BYTECODE.equ_real
    function LessThan(X, Y : real) return boolean               written X<Y                     is XL.BYTECODE.lt_real
    function LessOrEqual(X, Y : real) return boolean            written X<=Y                    is XL.BYTECODE.le_real
    function GreaterThan(X, Y : real) return boolean            written X>Y                     is XL.BYTECODE.gt_real
    function GreaterOrEqual(X, Y : real) return boolean         written X>=Y                    is XL.BYTECODE.ge_real
    function Different(X, Y : real) return boolean              written X<>Y                    is XL.BYTECODE.ne_real



    // ========================================================================
    // 
    //    Fixed-point operations
    // 
    // ========================================================================

    // TO BE DONE


    // ========================================================================
    // 
    //    Boolean operations
    // 
    // ========================================================================

    // Boolean operations
    function Boolean() return boolean                                                           is XL.BYTECODE.zero_bool
    to Copy(out Tgt : boolean; in Src : boolean)                written Tgt := Src              is XL.BYTECODE.copy_bool
    function LogicalNot(X : boolean) return boolean             written not X                   is XL.BYTECODE.not_bool
    function LogicalAnd(X, Y : boolean) return boolean          written X and Y                 is XL.BYTECODE.and_bool
    function LogicalOr(X, Y : boolean) return boolean           written X or Y                  is XL.BYTECODE.or_bool
    function LogicalXor(X, Y : boolean) return boolean          written X xor Y                 is XL.BYTECODE.xor_bool
    function Equal(X, Y : boolean) return boolean               written X=Y                     is XL.BYTECODE.equ_bool
    function LessThan(X, Y : boolean) return boolean            written X<Y                     is XL.BYTECODE.lt_bool
    function LessOrEqual(X, Y : boolean) return boolean         written X<=Y                    is XL.BYTECODE.le_bool
    function GreaterThan(X, Y : boolean) return boolean         written X>Y                     is XL.BYTECODE.gt_bool
    function GreaterOrEqual(X, Y : boolean) return boolean      written X>=Y                    is XL.BYTECODE.ge_bool
    function Different(X, Y : boolean) return boolean           written X<>Y                    is XL.BYTECODE.ne_bool



    // ========================================================================
    // 
    //    Character operations
    // 
    // ========================================================================

    // Character operations
    function character() return character                                                       is XL.BYTECODE.zero_char
    to Copy(out Tgt : character; in Src : character)            written Tgt := Src              is XL.BYTECODE.copy_char
    function Equal(X, Y : character) return boolean             written X=Y                     is XL.BYTECODE.equ_char
    function LessThan(X, Y : character) return boolean          written X<Y                     is XL.BYTECODE.lt_char
    function LessOrEqual(X, Y : character) return boolean       written X<=Y                    is XL.BYTECODE.le_char
    function GreaterThan(X, Y : character) return boolean       written X>Y                     is XL.BYTECODE.gt_char
    function GreaterOrEqual(X, Y : character) return boolean    written X>=Y                    is XL.BYTECODE.ge_char
    function Different(X, Y : character) return boolean         written X<>Y                    is XL.BYTECODE.ne_char



    // ========================================================================
    // 
    //    Text
    // 
    // ========================================================================


    // ========================================================================
    // 
    //    Ordered and min/max
    // 
    // ========================================================================

    generic type ordered where
        X, Y : ordered
        Test : boolean := X < Y

    function Min(X : ordered) return ordered is
        return X
    function Min(X : ordered; ...) return ordered is
        result := Min(...)
        if X < result then
            result := X

    function Max(X : ordered) return ordered is
        return X
    function Max(X : ordered; ...) return ordered is
        result := Min(...)
        if X < result then
            result := X



    // ========================================================================
    // 
    //    Ranges
    // 
    // ========================================================================

    generic [type ordered_type]
    type range written range of ordered_type is record with
        First, Last : ordered_type

    function Range (First, Last : ordered) return range[ordered] written First..Last is
         result.First := First
         result.Last := Last



    // ========================================================================
    // 
    //    Arrays
    // 
    // ========================================================================



    // ========================================================================
    // 
    //    Type conversions
    // 
    // ========================================================================

    // Convert to integer
    function integer(Value : integer) return integer                                            is XL.BYTECODE.int_from_int
    function integer(Value : unsigned) return integer                                           is XL.BYTECODE.int_from_uint
    function integer(Value : character) return integer                                          is XL.BYTECODE.int_from_char
    function integer(Value : boolean) return integer                                            is XL.BYTECODE.int_from_bool
    function integer(Value : real) return integer                                               is XL.BYTECODE.int_from_real

    // Convert to unsigned
    function unsigned(Value : integer) return unsigned written Value U                          is XL.BYTECODE.uint_from_int
    function unsigned(Value : unsigned) return unsigned                                         is XL.BYTECODE.uint_from_uint

    // Convert to real
    function real(Value : integer) return real                                                  is XL.BYTECODE.real_from_int
    function real(Value : unsigned) return real                                                 is XL.BYTECODE.real_from_uint
    function real(Value : real) return real                                                     is XL.BYTECODE.real_from_real

    // Convert to character
    function character(Value : integer) return character                                        is XL.BYTECODE.char_from_int

    // Convert to boolean
    function boolean(Value : integer) return boolean                                            is XL.BYTECODE.bool_from_int
    function boolean(Value : unsigned) return boolean                                           is XL.BYTECODE.bool_from_uint



    // ========================================================================
    // 
    //    Iterators
    // 
    // ========================================================================

    iterator IntegerIterator (
            var out Counter : integer
            Low, High       : integer) written Counter in Low..High is
        Counter := Low
        while Counter <= High loop
            yield
            Counter += 1

    iterator IntegerIncrementingIterator (
            var out Counter : integer
            Low, High, Incr : integer) written Counter in Low..High step Incr is
        Counter := Low
        while Counter <= High loop
            yield
            Counter += Incr

    iterator RealIncrementingIterator (
            var out Counter : real
            Low, High, Incr : real) written Counter in Low..High step Incr is
        Counter := Low
        while Counter <= High loop
            yield
            Counter += Incr

    iterator RangeIterator (
            var out Counter : integer
            interval : range) written Counter in interval is
        Counter := interval.first
        while Counter <= interval.last loop
            yield
            Counter += 1

    iterator RangeIncrementingIterator (
            var out Counter : integer
            interval : range;
            incr : range.ordered_type) written Counter in interval step incr is
        Counter := interval.first
        while Counter <= interval.last loop
            yield
            Counter += incr


    // ========================================================================
    // 
    //    Assert macro
    // 
    // ========================================================================

    procedure xl_assertion_failure(msg : text; file : text; line : integer) is           XL.BYTECODE.xl_assert

    // Evaluate the expression, expand the condition as text
    transform (assert 'cond') into
        xl_assert_implementation(xl.value 'cond', xl.text 'cond', 'cond')

    // Check the constant cases for static asserts
    transform (xl_assert_implementation(true, 'message', 'source')) into
        @nop

    transform (xl_assert_implementation(false, 'message', 'source')) into
        xl.error ("Assertion '$1' is false at compile time", 'source')

    // Otherwise, expand to a runtime test
    transform (xl_assert_implementation('cond', 'msg', 'src')) into
        if not 'cond' then
            xl_assertion_failure 'msg', xl.file 'src', xl.line 'src'

