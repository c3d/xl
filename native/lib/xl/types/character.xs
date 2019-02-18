// ****************************************************************************
//  character.xs                                    XL - An extensible language
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

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
