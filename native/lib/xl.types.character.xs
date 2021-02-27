// *****************************************************************************
// xl.types.character.xs                                              XL project
// *****************************************************************************
//
// File description:
//
//     Interface for character data types
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

use LIST


module CHARACTER[value:type, Size, Align] with
// ----------------------------------------------------------------------------
//    A generic interface for character types
// ----------------------------------------------------------------------------

    use COMPARISON[value]
    use ASSIGN[value]
    use COPY[value]
    use MEMORY.SIZE[Size]
    use MEMORY.ALIGN[Align]



module CHARACTER with
// ----------------------------------------------------------------------------
//   Definition of the basic character types
// ----------------------------------------------------------------------------

    character                   is UTF32

    ASCII                       as type
    UTF8                        as type
    UTF16                       as type
    UTF32                       as type
    EBCDIC                      as type

    use CHARACTER[ASCII,        8,   8]
    use CHARACTER[UTF8,         8,   8]
    use CHARACTER[UTF16,       16,  16]
    use CHARACTER[UTF32,       32,  32]
    use CHARACTER[EBCDIC,       8,   8]

    types                       as list of type



// *****************************************************************************
// character.xs                                                       XL project
// *****************************************************************************
//
// File description:
//
//     Interface for character data types
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

type character[Bits] with
// ----------------------------------------------------------------------------
//    Generic interface for character types
// ----------------------------------------------------------------------------

    features ENUMERTATED.enumerated
    features COPY.copiable
    features MOVE.movable
    features CLONE.clonable
    features DELETE.deletable
    features COMPARISON.equatable
    features COMPARISON.ordered
    features MEMORY.sized
    features MEMORY.aligned


type ASCII              is character[7]
type UTF8               is character[8]
type UTF16              is character[16]
type UTF32              is character[32]
type character          is UTF32
