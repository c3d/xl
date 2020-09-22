// *****************************************************************************
// integer.xs                                                         XL project
// *****************************************************************************
//
// File description:
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

use NUMBER
use SYSTEM
use BITWISE
use BITSHIFT
use LIST


type integer[Low..High] with
// ----------------------------------------------------------------------------
//    Description of a generic integer type
// ----------------------------------------------------------------------------

    // Interfaces that 'integer' implements
    as number
    as bitwise
    as bitshift
    as enumerated

    // Implement the necessary interface for `type`
    BitSize                     as bit_count
    BitAlign                    as bit_count

    // Indicates if the type is signed
    Signed                      as boolean


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
type natural[Low..High]        when Low >= 0   is another integer[Low..High]
type natural range Low..High   when Low >= 0   is natural[Low..High]

// Sized types
type integer8                                   is integer bits 8
type integer16                                  is integer bits 16
type integer32                                  is integer bits 32
type integer64                                  is integer bits 64

type natural8                                  is natural bits 8
type natural16                                 is natural bits 16
type natural32                                 is natural bits 32
type natural64                                 is natural bits 64
