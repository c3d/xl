// *****************************************************************************
// posix.cpp                                                          XL project
// *****************************************************************************
//
// File description:
//
//     Expose a number of useful POSIX functions
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
// (C) 2020-2021, Christophe de Dinechin <christophe@dinechin.org>
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

#include "native.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

XL_BEGIN

NATIVE(getpid);
NATIVE(putchar);

// Need to eliminate the noreturn attribute which makes instantiation fail
void exit(int rc) { ::exit(rc); }
NATIVE(exit);

// <math.h> functions are often overloaded in C++ - Create local copy
#define MATH(fn)                                \
namespace                                       \
{                                               \
double fn(double x)                             \
{                                               \
    return ::fn(x);                             \
}                                               \
NATIVE(fn);                                     \
}

#define MATH2(fn)                               \
namespace                                       \
{                                               \
double fn(double x, double y)                   \
{                                               \
    return ::fn(x, y);                          \
}                                               \
NATIVE(fn);                                     \
}

#define MATH2I(fn)                              \
namespace                                       \
{                                               \
double fn(int x, double y)                      \
{                                               \
    return ::fn(x, y);                          \
}                                               \
NATIVE(fn);                                     \
}

#define MATH3(fn)                               \
namespace                                       \
{                                               \
double fn(double x, double y, double z)         \
{                                               \
    return ::fn(x, y, z);                       \
}                                               \
NATIVE(fn);                                     \
}

MATH(ceil);
MATH(floor);
MATH(sqrt);
MATH(exp);
MATH(exp2);
MATH(expm1);
MATH(log);
MATH(log2);
MATH(log10);
MATH(log1p);
MATH(logb);
MATH2(hypot);
MATH(cbrt);
MATH(erf);
MATH(lgamma);
MATH3(fma);
MATH2(pow);

MATH(sin);
MATH(cos);
MATH(tan);
MATH(asin);
MATH(acos);
MATH(atan);
MATH2(atan2);
MATH(sinh);
MATH(cosh);
MATH(tanh);
MATH(asinh);
MATH(acosh);
MATH(atanh);
MATH(j0);
MATH(j1);
MATH2I(jn);
MATH(y0);
MATH(y1);
MATH2I(yn);

XL_END
