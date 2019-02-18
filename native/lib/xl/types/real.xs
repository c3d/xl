// ****************************************************************************
//  real.xs                                         XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     The basic real number operations
//
//     Real numbers are actually stored using a floating-point representation
//     XL distinguishes the `real` type, which cannot hold special values
//     like NaN, and consequently is fully ordered, and the `ieee` types
//     which support NaN values
//
//
//
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

import NUMBER
use SET

module REAL[number:type, Size, Align, Kind] with
// ----------------------------------------------------------------------------
//    A generic interface for real types
// ----------------------------------------------------------------------------
    use NUMBER[number, Size, Align, Kind]

    number has
    // ------------------------------------------------------------------------
    //    Add interfaces specific to floating-point types
    // ------------------------------------------------------------------------
        MantissaBits        as bit_count
        ExponentBits        as bit_count


    module CONSTANTS with
    // ------------------------------------------------------------------------
    //    Export common constants for the type
    // ------------------------------------------------------------------------
        pi                              as value
        if number.UNORDERED then
            quiet_NaN                   as value
            signaling_NaN               as value
            infinity                    as value

    use CONSTANTS


module REAL with
// ----------------------------------------------------------------------------
//   Definition of the basic real types
// ----------------------------------------------------------------------------

    type real16
    type real32
    type real64
    type real128

    use REAL[real16,   16,  16, FLOAT]
    use REAL[real32,   32,  32, FLOAT]
    use REAL[real64,   64,  64, FLOAT]
    use REAL[real128, 128, 128, FLOAT]

    module IEEE with
    // ------------------------------------------------------------------------
    //    The IEEE-754 standard supports NaN values, but is not ordered
    // ------------------------------------------------------------------------
        type float16
        type float32
        type float64
        type float128

        use REAL[float16,   16,  16, FLOAT + UNORDERED]
        use REAL[float32,   32,  32, FLOAT + UNORDERED]
        use REAL[float64,   64,  64, FLOAT + UNORDERED]
        use REAL[float128, 128, 128, FLOAT + UNORDERED]

    real                is SYSTEM.real
    types               is list of types
