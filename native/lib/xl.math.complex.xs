// *****************************************************************************
// complex.xs                                                         XL project
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

module COMPLEX[real:type, angle:type] with
// ----------------------------------------------------------------------------
//   A generic module for complex numbers
// ----------------------------------------------------------------------------
//  The reason for separating 'real' and 'angle' arguments is to be able
//  to properly accomodate a non floating-point representation for 'real',
//  and possibly a natural number representation for angles (e.g. degrees)

    // A complex can be represented in either cartesian or polar form
    type complex                is cartesian or polar
    type cartesian              is matching cartesian(Re:real, Im:real)
    type polar                  is matching polar(Mod:real, Arg:angle)

    // Imaginary unit
    i                           as cartesian
    i                           as polar
    j                           as cartesian    is i
    j                           as polar        is i

    // Shortcut notation for cartesian notation like `2 + 3i`
    syntax { postfix 400 i j }
    Re:real + Im:real i         as cartesian    is cartesian(Re, Im)
    Im:real i                   as cartesian    is cartesian(0, Im)
    Re:real + Im:real j         as cartesian    is cartesian(Re, Im)
    Im:real j                   as cartesian    is cartesian(0, Im)

    // Shortcut for multiplication by i (e.g. to accelerate exp(Z*i))
    Z:cartesian i               as cartesian    is Z * i
    Z:cartesian * [[i]]         as cartesian
    Z:polar     * [[i]]         as polar
    [[i]]       * Z:cartesian   as cartesian    is Z * i
    [[i]]       * Z:polar       as polar        is Z * i

    // Implicit conversion between the representations
    Z:cartesian                 as polar
    Z:polar                     as cartesian

    // Implicit conversions from the base type
    X:real                      as cartesian    is cartesian(X, 0)
    X:real                      as polar        is polar(X, 0)

    // Implicit conversion from anything that implicitly converts to [real]
    // This is necessary for [Z:=2] if [real] is floating point
    X:implicit[real]            as cartesian    is cartesian(real(X), 0)
    X:implicit[real]            as polar        is polar(real(X), 0)
`
    // Special operations that can be optimized with one real operand
    Z:cartesian + X:real        as cartesian
    Z:cartesian - X:real        as cartesian
    Z:cartesian * X:real        as cartesian
    Z:cartesian / X:real        as cartesian    when X ≠ 0
    Z:cartesian / X:real        as error        when X = 0
    X:real      + Z:cartesian   as cartesian
    X:real      - Z:cartesian   as cartesian
    X:real      * Z:cartesian   as cartesian
    X:real      / Z:cartesian   as cartesian    when Z ≠ 0
    X:real      / Z:cartesian   as error        when Z = 0

    // In polar form, only multiplication and division are faster
    Z:polar     * X:real        as polar
    Z:polar     / X:real        as polar        when X ≠ 0
    Z:polar     / X:real        as error        when X = 0
    X:real      * Z:polar       as polar
    X:real      / Z:polar       as polar        when X ≠ 0
    X:real      / Z:polar       as error        when X = 0

    // Basic arithmetic in cartesian form
    X:cartesian + Y:cartesian   as cartesian
    X:cartesian - Y:cartesian   as cartesian
    X:cartesian * Y:cartesian   as cartesian
    X:cartesian / Y:cartesian   as cartesian    when X ≠ 0
    X:cartesian / Y:cartesian   as error        when X = 0
    +X:cartesian                as cartesian
    -X:cartesian                as cartesian
    ~X:cartesian                as cartesian    // Conjugate
    conjugate X:cartesian       as cartesian    is ~X

    // Basic arithmetic in polar form
    X:polar     * Y:polar       as polar
    X:polar     / Y:polar       as polar        when Y.Arg ≠ 0
    X:polar     / Y:polar       as error        when Y.Arg = 0
    +X:polar                    as polar
    -X:polar                    as polar
    ~X:polar                    as polar        // Conjugate
    conjugate X:polar           as polar        is ~X

    // Prefix form for real and imaginary part without whole conversion
    re          Z:cartesian     as real         is Z.Re
    re          Z:polar         as real
    im          Z:cartesian     as real         is Z.Im
    im          Z:polar         as real
    mod         Z:cartesian     as real
    mod         Z:polar         as real         is Z.Mod
    arg         Z:cartesian     as real
    arg         Z:polar         as real         is Z.Arg

    // Equality
    X:cartesian = Y:cartesian   as boolean
    X:polar     = Y:polar       as boolean

    // Total order
    // The < operator is a lexicographic order, required for some containers
    // For consistency, it is only defined on the cartesian form, where it
    // matches the order on real values
    X:cartesian < Y:cartesian   as boolean

    // Some elementaty functions on complex numbers
    abs         Z: complex      as real         is mod Z
    sqrt        Z:polar         as polar
    exp         Z:cartesian     as polar
    ln          Z:polar         as cartesian when Z.Arg ≠ 0

    Z:cartesian ^ N:natural     as cartesian
    X:polar     ^ Y:cartesian   as polar

    sin         Z:cartesian     as polar
    cos         Z:cartesian     as polar
    tan         Z:cartesian     as polar
    arcsin      Z:polar         as cartesian
    arccos      Z:polar         as cartesian
    arctan      Z:polar         as cartesian

    sinh        Z:cartesian     as polar
    cosh        Z:cartesian     as polar
    tanh        Z:cartesian     as polar
    arcsinh     Z:polar         as cartesian
    arccosh     Z:polar         as cartesian
    arctanh     Z:polar         as cartesian

    // Basic I/O operations
    to write    Z:cartesian     as ok
    to write    Z:polar         as ok
    to read     Z:out cartesian as ok
    to read     Z:out polar     as ok


module COMPLEX[real:type] is COMPLEX[real, real]
// ----------------------------------------------------------------------------
//   A generic module for the case of floating-point numbers
// ----------------------------------------------------------------------------
//   In that case, we can use the same type by default for the angle type


module COMPLEX with
// ----------------------------------------------------------------------------
//   A non-generic module offering a generic type
// ----------------------------------------------------------------------------
//   This makes it easier to instantiate the generic modules

    type complex[real:type, angle:type] is COMPLEX[real,angle].complex
    type complex[real:type] is complex[real,real]
    type complex is complex[real]
