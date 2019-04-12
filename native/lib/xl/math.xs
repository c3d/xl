// *****************************************************************************
// math.xs                                                            XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2019, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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

module ARBITRARY_PRECISION

module COMPLEX
module QUATERNION
module VECTOR
module MATRIX
module STATISTICS


// Using the `MATH` module by default uses `real` functions and constants
use REAL
use MATH[real].FUNCTIONS
use MATH[real].CONSTANTS


module MATH[type number] with
// ----------------------------------------------------------------------------
//  Interface of the math module for a given number type
// ----------------------------------------------------------------------------

    module FUNCTIONS with
    // ------------------------------------------------------------------------
    //    Interface for basic math functions
    // ------------------------------------------------------------------------

        Abs     X:number                as number
        Sign    X:number                as integer
        Sqrt    X:number                as number

        Sin     X:number                as number
        Cos     X:number                as number
        Tan     X:number                as number
        ArcSin  X:number                as number
        ArcCos  X:number                as number
        ArcTan  X:number                as number
        ArcTan  Y:number, X:number      as number

        SinH    X:number                as number
        CosH    X:number                as number
        TanH    X:number                as number
        ArcSinH X:number                as number
        ArcCosH X:number                as number
        ArcTanH X:number                as number

        Exp     X:number                as number
        Exp     X:number - 1            as number
        Log     X:number                as number
        Log10   X:number                as number
        Log2    X:number                as number
        Log2i   X:number                as integer
        Log     (1 + X:number)          as number


    module CONSTANTS is
    // ------------------------------------------------------------------------
    //    Interface for math constants
    // ------------------------------------------------------------------------
        ZERO                            is number 0
        ONE                             is number 1
        TWO                             is number 2
        PI                              is number 3.1415926535897932384626433
