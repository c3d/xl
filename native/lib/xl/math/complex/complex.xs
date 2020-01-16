// *****************************************************************************
// complex.xs                                                         XL project
// *****************************************************************************
//
// File description:
//
//      interface for complex numbers
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

in XL.MATH

COMPLEX[real  : type like number,
        angle : type like number] as module with
// ----------------------------------------------------------------------------
//    A generic module for complex numbers taking a `real` type
// ----------------------------------------------------------------------------

    // A complex can be represented in either cartesian or polar form
    complex as type like number is cartesian or polar
    cartesian                   is type cartesian(Re:real, Im:real)
    polar                       is type polar(Mod:real, Arg:angle)

    cartesian                   like number
    polar                       like number

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
    [[i]]       * Z:cartesian   as cartesian
    [[i]]       * Z:polar       as polar

    // Implicit conversion between the representations
    Z:cartesian                 as polar
    Z:polar                     as cartesian
    X:real                      as cartesian
    X:real                      as polar
    X:real                      as complex
    X:natural                   as cartesian
    X:natural                   as polar
    X:natural                   as complex
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

    // Prefix form for field access
    re          Z:cartesian     as real
    re          Z:polar         as real
    im          Z:cartesian     as real
    im          Z:polar         as real
    mod         Z:cartesian     as real
    mod         Z:polar         as real
    arg         Z:polar         as real
    arg         Z:cartesian     as real

    // Comparisons
    X:cartesian = Y:cartesian   as boolean
    X:cartesian < Y:cartesian   as boolean

    // Some elementaty functions on complex numbers
    abs         Z: complex      as real is mod Z
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

    in XL.TEXT_IO
    write       Z:cartesian     as mayfail
    write       Z:polar         as mayfail


COMPLEX as module with
// ----------------------------------------------------------------------------
//   A non-generic module providing generic types
// ----------------------------------------------------------------------------

    complex[real:type like number] as type like number is
        super.COMPLEX[real].complex with super.COMPLEX[real]

    complex as type like number is complex[real]
