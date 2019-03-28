#ifndef UTF8_H
#define UTF8_H
// *****************************************************************************
// utf8.h                                                             XL project
// *****************************************************************************
//
// File description:
//
//    Simple utilities to deal with UTF-8 encoding
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
// (C) 2010,2014-2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
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

#include "base.h"
#include <ctype.h>

XL_BEGIN

#define IS_UTF8_FIRST(x)        (uchar(x) >= 0xC0 && uchar(x) <= 0xFD)
#define IS_UTF8_NEXT(x)         (uchar(x) >= 0x80 && uchar(x) <= 0xBF)

inline bool IS_UTF8_OR_ALPHA(char c)
// ----------------------------------------------------------------------------
//   When splitting words, we arbitrarily take any UTF8 as being "alpha"...
// ----------------------------------------------------------------------------
{
    return isalpha(c) || IS_UTF8_FIRST(c) || IS_UTF8_NEXT(c);
}


inline uint Utf8Previous(const text &t, uint position)
// ----------------------------------------------------------------------------
//   Finds the previous position in the text, assumed to be UTF-8
// ----------------------------------------------------------------------------
{
    if (position > 0)
    {
        kstring d = t.data();
        position--;
        while (position > 0 && IS_UTF8_NEXT(d[position]))
            position--;
    }
    return position;
}


inline uint Utf8Next(const text &t, uint position)
// ----------------------------------------------------------------------------
//   Find the next position in the text, assumed to be UTF-8
// ----------------------------------------------------------------------------
{
    uint tlen = t.length();
    if (position < tlen)
    {
        position++;
        kstring d = t.data();
        while (position < tlen && IS_UTF8_NEXT(d[position]))
            position++;
    }
    return position;
}


inline uint Utf8Code(const text &t, uint position)
// ----------------------------------------------------------------------------
//   Return the Unicode value for the character at the given position
// ----------------------------------------------------------------------------
{
    uint tlen = t.length();
    uint code = 0;
    if (position < tlen)
    {
        kstring d = t.data();
        code = d[position];
        if (code & 0x80)
        {
            // Reference: Wikipedia UTF-8 description
            if ((code & 0xE0) == 0xC0 && position+1 < tlen)
                code = ((code & 0x1F)          << 6)
                    |  (d[position+1] & 0x3F);
            else if ((code & 0xF0) == 0xE0 && position + 2 < tlen)
                code = ((code & 0xF)           << 12)
                    |  ((d[position+1] & 0x3F) << 6)
                    |   (d[position+2] & 0x3F);
            else if ((code & 0xF8) == 0xF0 && position + 3 < tlen)
                code = ((code & 0xF)           << 18)
                    |  ((d[position+1] & 0x3F) << 12)
                    |  ((d[position+2] & 0x3F) << 6)
                    |   (d[position+3] & 0x3F);
        }
    }
    return code;
}


inline text Utf8WordsAfter(text value, uint pos, uint count =3, uint skip =0)
// ----------------------------------------------------------------------------
//    Return a few words after the given position
// ----------------------------------------------------------------------------
{
    text result = "";
    uint len = value.length();
    for (uint word = 0; pos < len && word < count; word++)
    {
        while (pos < len && !IS_UTF8_OR_ALPHA(value[pos]))
        {
            if (!skip)
                result += value[pos];
            pos++;
        }
        while (pos < len && IS_UTF8_OR_ALPHA(value[pos]))
        {
            if (!skip)
                result += value[pos];
            pos++;
        }
        if (skip)
            skip--;
    }
    return result;
}


inline text Utf8WordsBefore(text value, uint pos, uint count =3, uint skip =0)
// ----------------------------------------------------------------------------
//    Return a few words before the given position
// ----------------------------------------------------------------------------
{
    text result = "";
    for (uint word = 0; (int) pos >= 0 && word < count; word++)
    {
        while ((int) pos >= 0 && !IS_UTF8_OR_ALPHA(value[pos]))
        {
            if (!skip)
                result.insert(0, 1, value[pos]);
            pos--;
        }
        while ((int) pos >= 0 && IS_UTF8_OR_ALPHA(value[pos]))
        {
            if(!skip)
                result.insert(0, 1, value[pos]);
            pos--;
        }
        if (skip)
            skip--;
    }
    return result;
}


inline uint Utf8Length(text value)
// ----------------------------------------------------------------------------
//    Return the length of the text in characters
// ----------------------------------------------------------------------------
{
    uint result = 0;
    uint max = value.length();
    for (uint pos = 0; pos < max; pos++)
        result += !IS_UTF8_NEXT(value[pos]);
    return result;
}

XL_END

#endif // UTF8_H
