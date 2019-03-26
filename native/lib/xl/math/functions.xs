// ****************************************************************************
//  functions.xs                                    XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Signature for common mathematical functions
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
