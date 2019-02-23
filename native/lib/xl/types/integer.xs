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

use NUMBER
use SYSTEM
use BITWISE
use LIST



type integer[Low..High] with
// ----------------------------------------------------------------------------
//    Description of a generic integer type
// ----------------------------------------------------------------------------

    // Interfaces that 'integer' implements
    as number
    as bitwise

    // Implement the necessary interface for `type`
    BitSize                     as bit_count
    BitAlign                    as bit_count


with
    // Some local definitions for convenience
    IMin Bits                                   is -(1 << (Bits - 1))
    IMax Bits                                   is  (1 << (Bits - 1)) - 1
    IRange Bits                                 is IMin Bits..IMax Bits
    UMin Bits                                   is  0
    UMax Bits                                   is  (1 << Bits) - 1
    URange Bits                                 is UMin Bits..UMax Bits


// Other notations for integer types
type integer range Low..High                    is integer[Low..High]
type integer bits Bits                          is integer[IRange Bits]
type unsigned[Low..High]        when Low >=0    is another integer[Low..High]
type unsigned range Low..High   when Low >= 0   is unsigned[Low..High]

// Sized types
type integer8                                   is integer bits 8
type integer16                                  is integer bits 16
type integer32                                  is integer bits 32
type integer64                                  is integer bits 64

type unsigned8                                  is unsigned bits 8
type unsigned16                                 is unsigned bits 16
type unsigned32                                 is unsigned bits 32
type unsigned64                                 is unsigned bits 64

// System types are optimized for the natural register size on the machine
type integer                                    is SYSTEM.integer
type unsigned                                   is SYSTEM.unsigned
