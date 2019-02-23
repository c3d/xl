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

use NUMBER
use SYS = XL.SYSTEM.TYPES


type unsigned with
// ----------------------------------------------------------------------------
//   The most basic unsigned type uses the system default size
// ----------------------------------------------------------------------------
    use SYS.unsigned
    use number


type unsigned[Low..High] with
// ----------------------------------------------------------------------------
//    Define a general unsigned type in the given range
// ----------------------------------------------------------------------------
    Range               as range of SYS.unsigned
    MinValue            as unsigned
    MaxValue            as unsigned


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
