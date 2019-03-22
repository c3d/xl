#ifndef SHA1_OSTREAM_H
#define SHA1_OSTREAM_H
// *****************************************************************************
// sha1_ostream.h                                                     XL project
// *****************************************************************************
//
// File description:
//
//     Ostream operations on a SHA-1 class
//
//     The only reasons they are not in sha1.h is because putthing them
//     at that spot apparently 'disables' another operator<< (os&, Tree *)
//     I have not spent enough time to know if it's a GCC bug or a C++ feature
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2009-2010,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can r redistribute it and/or modify
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

#include <iostream>
#include <stdio.h>
#include "sha1.h"

XL_BEGIN

inline std::ostream &operator <<(std::ostream &out, const XL::Sha1 &sha)
// ----------------------------------------------------------------------------
//    Display a hex digest for the given input
// ----------------------------------------------------------------------------
{
    char buffer[sha.SIZE * 2 + 1];
    for (uint i = 0; i < sha.SIZE; i++)
        sprintf(buffer + 2 * i, "%02x", sha.hash[i]);
    buffer[sha.SIZE*2] = 0;
    out << buffer;
    return out;
}

XL_END

#endif // SHA1_OSTREAM_H
