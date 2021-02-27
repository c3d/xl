// *****************************************************************************
// xl.math.xs                                                         XL project
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
// This software is licensed under the GNU General Public License v3+
// (C) 2019-2020, Christophe de Dinechin <christophe@dinechin.org>
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

with
    X:real
    Y:real
    Z:real
    N:natural
    I:integer
    Base:real

// Functions on integer values
abs I           as integer
sign I          as integer

// Functions on real values
abs X           as real
sign X          as real
ceil X          as real
floor X         as real
sqrt X          as real
exp X           as real
exp2 X          as real
log X           as real
ln X            as real
log10 X         as real
log2 X          as real
logb X          as integer
log(Base,X)     as real
hypot(X,Y)      as real
hypothenuse(X,Y)as real is hypot(X,Y)
cbrt X          as real
CubeRoot X      as real is cbrt X
erf X           as real
ErrorFunction X as real is erf X
lgamma X        as real
LogGamma X      as real is lgamma X
fma X,Y,Z       as real
X*Y+Z           as real is fma X,Y,Z
Z+X*Y           as real is fma X,Y,Z

// Circular trigonometry
sin X           as real
cos X           as real
tan X           as real
asin X          as real
acos X          as real
atan X          as real
atan Y/X        as real

// Hyperbolic trigonometry
sinh X          as real
cosh X          as real
tanh X          as real
asinh X         as real
acosh X         as real
atanh X         as real

// Bessel functions
j0 X            as real
j1 X            as real
jn I, X         as real
J 0, X          as real is j0 X
J 1, X          as real is j1 X
J I, X          as real is jn I,X
y0 X            as real
y1 X            as real
yn I, X         as real
Y 0, X          as real is y0 X
Y 1, X          as real is y1 X
Y I, X          as real is yn I, X

// Optimized forms
(exp X) - 1.0   as real
log (1.0+X)     as real
log (X+1.0)     as real
2.0^X           as real

// Constants
pi
e

// Optimized expressions
// constant (log2  [[e]])
// constant (log10 [[e]])
// constant (log   2.0)
// constant (pi/2.0)
// constant (pi/4.0)
// constant (pi*0.5)
// constant (pi*0.25)
// constant (1.0/[[pi]])
// constant (2.0/[[pi]])
// constant (2.0/sqrt [[pi]])
// constant (1.0/[[pi]])
// constant (2.0/[[pi]])
// constant (2.0/sqrt [[pi]])
// constant (sqrt 2.0)
// constant (1.0/sqrt 2.0)
// constant (sqrt 2.0/2.0)
