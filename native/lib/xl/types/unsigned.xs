// ****************************************************************************
//  unsigned.xs                                     XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for unsigned types
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


module UNSIGNED[number:type, Size, Align] with
// ----------------------------------------------------------------------------
//    A generic interface for unsigned types
// ----------------------------------------------------------------------------
    use NUMBER[number, Size, Align, Kind is UNSIGNED]


module UNSIGNED with
// ----------------------------------------------------------------------------
//   Definition of the basic unsigned types
// ----------------------------------------------------------------------------
    type unsigned8
    type unsigned16
    type unsigned32
    type unsigned64
    type unsigned128

    use UNSIGNED[unsigned8,     8,   8]
    use UNSIGNED[unsigned16,   16,  16]
    use UNSIGNED[unsigned32,   32,  32]
    use UNSIGNED[unsigned64,   64,  64]
    use UNSIGNED[unsigned128, 128, 128]

    unsigned            is SYSTEM.unsigned
    types               is list of type
