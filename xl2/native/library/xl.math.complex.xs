// ****************************************************************************
//  xl.math.complex.xs              (C) 1992-2006 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
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
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision: 259 $
// * Date       : $Date: 2006-06-13 08:33:20 +0200 (Tue, 13 Jun 2006) $
// ****************************************************************************

module XL.MATH.COMPLEX with

    generic [type value is real]
    type complex is record with
        re : value
        im : value

    function Complex (re, im : complex.value) return complex written re + im i
    function Complex (re : complex.value) return complex written re
    function Imaginary (im : complex.value) return complex written im i
    to Copy(out Target : complex; Source : complex) written Target := Source

    function Add (X, Y : complex) return complex written X+Y
    function Subtract (X, Y : complex) return complex written X-Y
    function Multiply (X, Y : complex) return complex written X*Y
    function Divide (X, Y : complex) return complex written X/Y

