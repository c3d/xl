// ****************************************************************************
//  complex.xs                                      XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//      Complex numbers
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

COMPLEX[real:type] has
// ----------------------------------------------------------------------------
//   Interface of the complex number module
// ----------------------------------------------------------------------------

    // A complex number can be represented in cartesian or polar form
    complex                     is cartesian or polar
    cartesian                   is type cartesian(re:real, im:real)
    polar                       is type polar(rho:real, theta:real)

    // Complex numbers have arithmetic and equality operators
    use ARITHMETIC[complex]
    use COMPARISON[complex].EQUALITY

    // Special operations with real numbers as one operand
    X:complex + Y:real          as complex
    X:complex - Y:real          as complex
    X:complex * Y:real          as complex
    X:complex / Y:real          as complex

    // Getting real, imaginary, modulus and argument
    real        Z:complex       as real
    imaginary   Z:complex       as real
    modulus     Z:complex       as real
    argument    Z:complex       as real

    // Converting to polar and cartesian form
    cartesian   Z:complex       as cartesian
    polar       Z:complex       as polar

    // Unit imaginary real
    i                           is cartesian(real 0, real 1)
    j                           is i

    // Shortcut notation for things like `2 + 3i`
    syntax
        postfix 400, "i"
    Re:real + Im:real i         is cartesian(Re, Im)
    Im:real i                   is cartesian(real 0, Im)
