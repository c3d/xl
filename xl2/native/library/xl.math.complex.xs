// *****************************************************************************
// xl.math.complex.xs                                                 XL project
// *****************************************************************************
//
// File description:
//
//     Implementation of a generic complex type
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
// (C) 2003-2004,2006,2008,2015,2020, Christophe de Dinechin <christophe@dinechin.org>
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

module XL.MATH.COMPLEX with

    generic [type value is real]
    type complex is record with
        re : value
        im : value

    function Complex (re, im : complex.value) return complex written re + im i
    function Complex (re : complex.value) return complex
    function Imaginary (im : complex.value) return complex written im i
    to Copy(out Target : complex; Source : complex) written Target := Source

    function Add (X, Y : complex) return complex written X+Y
    function Subtract (X, Y : complex) return complex written X-Y
    function Multiply (X, Y : complex) return complex written X*Y
    function Divide (X, Y : complex) return complex written X/Y

    // Optimized notations with a number and a complex
    function Copy(out Target : complex;
                  Source     : complex.value) written Target := Source
    function Multiply (X : complex;
                       Y : complex.value) return complex written X*Y written Y*X
    function Divide (X : complex;
                     Y : complex.value) return complex written X/Y
