// *****************************************************************************
// xl.math.complex.xs                                                 XL project
// *****************************************************************************
//
// File description:
//
//      Interface for complex numbers
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

use NUMBER = XL.MATH.NUMBER
use ANGLE  = XL.MATH.ANGLE

module COMPLEX[real : like NUMBER.number, angle : like ANGLE.angle] with
// ----------------------------------------------------------------------------
//   A generic module for complex numbers
// ----------------------------------------------------------------------------
//  The reason for separating 'real' and 'angle' arguments is to be able
//  to properly accomodate a non floating-point representation for 'real',
//  and possibly a natural number representation for angles (e.g. degrees)
//
//  The interface does not specify the many special cases and optimizations
//  found in the implementation, e.g. converting imaginary to cartesian, etc.

    // A complex can be represented in several different forms
    type complex                is real or imaginary or cartesian or polar
    type imaginary              matches imaginary(Im:real)
    type cartesian              matches cartesian(Re:real, Im:real)
    type polar                  matches polar(Mod:real, Arg:angle)

    // Imaginary unit
    i                           as imaginary
    j                           is i

    // Shortcut notations, e.g. to write `2 + 3i`
    Z:complex [[i]]             is Z * i
    [[i]] Z:complex             is i * Z

    // Shortcut for polar notation like `2 ∠ 180°`
    Mod:real ∠ Arg:angle        as polar        is polar(Mod, Arg)

    // Implicit conversion between the representations
    X:implicit real             as cartesian    // Convert 2 to 2.0 to cartesian
    X:implicit real             as polar
    X:imaginary                 as cartesian
    X:imaginary                 as polar
    Z:polar                     as cartesian
    Z:cartesian                 as polar

    // Basic arithmetic (the implementation optimizes many special cases)
    X:complex + Y:complex       as complex
    X:complex - Y:complex       as complex
    X:complex * Y:complex       as complex
    X:complex / Y:complex       as complex      when Y ≠ 0
    X:complex / Y:complex       as error        when Y = 0 is
        error "Divide by zero", X, Y
    +Z:complex                  as complex      is Z
    -Z:complex                  as complex
    ~Z:complex                  as complex
    conjugate Z:complex         is ~Z

    // Prefix form for real and imaginary part without whole conversion
    re  Z:complex               as real
    im  Z:complex               as real
    mod Z:complex               as real
    arg Z:complex               as real

    // Equality (from which we derive inequality)
    X:complex = Y:complex       as boolean

    // Total order
    // The < operator is a lexicographic order, required for some containers
    // For consistency, it is only defined on the cartesian form, where it
    // matches the order on real values. Beware of arithmetic and order, though
    X:cartesian < Y:cartesian   as boolean

    // Some elementaty functions on complex numbers
    // Those are defined on the most efficient implementation, but they apply
    // to all complex representations thanks to implicit conversions
    modulus     Z:complex       is mod Z
    argument    Z:complex       is arg Z
    abs         Z: complex      is mod Z
    sqrt        Z:polar         as polar
    exp         Z:cartesian     as polar
    ln          Z:polar         as cartesian    when Z.Arg ≠ 0
    ln          Z:polar         as error        when Z.Arg = 0 is
        error "Logarithm of zero", Z

    Z:cartesian ^ N:natural     as cartesian
    X:polar     ^ Y:cartesian   as polar

    // Circular functions
    sin         Z:cartesian     as polar
    cos         Z:cartesian     as polar
    tan         Z:cartesian     as polar
    arcsin      Z:polar         as cartesian
    arccos      Z:polar         as cartesian
    arctan      Z:polar         as cartesian

    // Hyperbolic functions
    sinh        Z:cartesian     as polar
    cosh        Z:cartesian     as polar
    tanh        Z:cartesian     as polar
    arcsinh     Z:polar         as cartesian
    arccosh     Z:polar         as cartesian
    arctanh     Z:polar         as cartesian

    // Basic I/O operations
    to write    Output, Z:cartesian
    to write    Output, Z:polar
    to read     Input, Z:out cartesian
    to read     Input, Z:out polar



// ============================================================================
//
//  Shortcuts specializing the general case above
//
// ============================================================================

module COMPLEX[real:type] is
// ----------------------------------------------------------------------------
//   A generic module for the case of floating-point numbers
// ----------------------------------------------------------------------------
//   In that case, we can use the same type by default for the angle type
    use ANGLE = XL.MATH.ANGLE[real]
    COMPLEX[real, ANGLE.angle]


module COMPLEX with
// ----------------------------------------------------------------------------
//   A non-generic module offering a generic type
// ----------------------------------------------------------------------------
//   This makes it easier to instantiate the generic modules

    type complex[type real, type angle]         is COMPLEX[real, angle].complex
    type complex[type real]                     is COMPLEX[real].complex
    type complex                                is complex[real]
