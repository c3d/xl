// ****************************************************************************
//  integer.xs                                      XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     XL integer types and related arithmetic
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

import NUMBER
import SYSTEM
use LIST


module INTEGER[number:type, Size, Align] with
// ----------------------------------------------------------------------------
//    A generic interface for integer types
// ----------------------------------------------------------------------------
    use NUMBER[number, Size, Align, Kind is INTEGER + SIGNED]


module INTEGER with
// ----------------------------------------------------------------------------
//   Definition of the basic integer types
// ----------------------------------------------------------------------------
    type integer8
    type integer16
    type integer32
    type integer64
    type integer128

    use INTEGER[integer8,     8,   8]
    use INTEGER[integer16,   16,  16]
    use INTEGER[integer32,   32,  32]
    use INTEGER[integer64,   64,  64]
    use INTEGER[integer128, 128, 128]

    integer             is SYSTEM.integer
    types               is list of type
