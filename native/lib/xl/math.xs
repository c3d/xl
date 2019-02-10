// ****************************************************************************
//  xl.math.xs                                      XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for basic math functions
//
//
//
//
//
//
//
//
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

use INTEGER

MATH[number:type] has
// ----------------------------------------------------------------------------
//  Interface of the math module for a given number type
// ----------------------------------------------------------------------------

    FUNCTIONS has
    // ------------------------------------------------------------------------
    //    Interface for basic math functions
    // ------------------------------------------------------------------------

        Abs   X:number                  as number
        Sign  X:number                  as integer
        Sqrt  X:number                  as number

        Sin   X:number                  as number
        Cos   X:number                  as number
        Tan   X:number                  as number
        Asin  X:number                  as number
        Acos  X:number                  as number
        Atan  X:number                  as number
        Atan  Y:number, X:number        as number

        Sinh  X:number                  as number
        Cosh  X:number                  as number
        Tanh  X:number                  as number
        Asinh X:number                  as number
        Acosh X:number                  as number
        Atanh X:number                  as number

        Exp   X:number                  as number
        Expm1 X:number                  as number
        Log   X:number                  as number
        Log10 X:number                  as number
        Log2  X:number                  as number
        Log2i X:number                  as integer
        Log1p X:number                  as number


    CONSTANTS has
    // ------------------------------------------------------------------------
    //    Interface for math constants
    // ------------------------------------------------------------------------
        ZERO                            is number 0
        ONE                             is number 1
        TWO                             is number 2
        PI                              is number 3.1415926535897932384626433


MATH has
// ----------------------------------------------------------------------------
//   Interface for the basic math module
// ----------------------------------------------------------------------------

    // Using the `MATH` module by default uses `real` functions and constants
    use MATH[real].FUNCTIONS
    use MATH[real].CONSTANTS

    // Advanced mathematical topics
    module BIG_INTEGER          // Big integer
    module FIXED_POINT          // Fixed-point computations
    module FLOATING_POINT       // Generic floating-point operations

    module COMPLEX              // Complex numbers
    module QUATERNION           // Quaternions
    module VECTOR               // Numerical vectors
    module MATRIX               // Numerical matrices
    module STATISTICS           // Statistics
