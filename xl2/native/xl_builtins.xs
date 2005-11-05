module XL_BUILTINS with

    to Copy(out Tgt : integer; in Src : integer) written Tgt := Src is
        XL.BYTECODE.copy_int

    function Add(X, Y : integer) return integer written X+Y is
        XL.BYTECODE.add_int
    function Sub(X, Y : integer) return integer written X-Y is
        XL.BYTECODE.sub_int
    function Mul(X, Y : integer) return integer written X*Y is
        XL.BYTECODE.mul_int
    function Div(X, Y : integer) return integer written X/Y is
        XL.BYTECODE.div_int
    function CmpEq(X, Y : integer) return boolean written X=Y is
        XL.BYTECODE.equ_int
    function CmpLt(X, Y : integer) return boolean written X<Y is
        XL.BYTECODE.lt_int
    function CmpLe(X, Y : integer) return boolean written X<=Y is
        XL.BYTECODE.le_int
    function CmpGt(X, Y : integer) return boolean written X>Y is
        XL.BYTECODE.gt_int
    function CmpGe(X, Y : integer) return boolean written X>=Y is
        XL.BYTECODE.ge_int
    function CmpNe(X, Y : integer) return boolean written X<>Y is
        XL.BYTECODE.ne_int


    to Copy(out Tgt : real; in Src : real) written Tgt := Src is
        XL.BYTECODE.copy_real

    function Add(X, Y : real) return real written X+Y is
        XL.BYTECODE.add_real
    function Sub(X, Y : real) return real written X-Y is
        XL.BYTECODE.sub_real
    function Mul(X, Y : real) return real written X*Y is
        XL.BYTECODE.mul_real
    function Div(X, Y : real) return real written X/Y is
        XL.BYTECODE.div_real
    function CmpEq(X, Y : real) return boolean written X=Y is
        XL.BYTECODE.equ_real
    function CmpLt(X, Y : real) return boolean written X<Y is
        XL.BYTECODE.lt_real
    function CmpLe(X, Y : real) return boolean written X<=Y is
        XL.BYTECODE.le_real
    function CmpGt(X, Y : real) return boolean written X>Y is
        XL.BYTECODE.gt_real
    function CmpGe(X, Y : real) return boolean written X>=Y is
        XL.BYTECODE.ge_real
    function CmpNe(X, Y : real) return boolean written X<>Y is
        XL.BYTECODE.ne_real


    function BooleanNot(value : boolean) return boolean is
        XL.BYTECODE.not_bool
    function BooleanAnd(X, Y : boolean) return boolean written X and Y is
        XL.BYTECODE.and_bool
    function BooleanOr(X, Y : boolean) return boolean written X or Y is
        XL.BYTECODE.or_bool
    function BooleanXor(X, Y : boolean) return boolean written X xor Y is
        XL.BYTECODE.xor_bool
