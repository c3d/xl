#ifndef SHA1_H
#define SHA1_H
// *****************************************************************************
// sha1.h                                                             XL project
// *****************************************************************************
//
// File description:
//
//     Function prototypes for the SHA-1 algorithm.
//
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
// (C) 2010,2019, Christophe de Dinechin <christophe@dinechin.org>
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

#include "base.h"
#include <stddef.h>
#include <string.h>
#include <iostream>


XL_BEGIN

struct Sha1
// ----------------------------------------------------------------------------
//    Encapsulate SHA-1 computations and result
// ----------------------------------------------------------------------------
//    We need a class to store SHA-1 results as valid keys for std::map
{
    enum { SIZE = 160 / 8 }; // 160 bits = 20 bytes

    struct Computation
    // ------------------------------------------------------------------------
    //   An inner class used to compute the given hash
    // ------------------------------------------------------------------------
    {
        Computation();
        Computation &   operator() (const void *inbuf, size_t inlen);
        byte *          Result() { return finalize().buf; }
        void            reset();

    private:
        Computation &   transform(byte *data);
        Computation &   finalize();

    private:
        uint32 h0,h1,h2,h3,h4;
        uint32 nblocks;
        byte   buf[64];
        int    count;
    };

public:
    Sha1() { memset(hash, 0, SIZE); }
    Sha1(Computation &c) { memcpy(hash, c.Result(), SIZE); }
    Sha1(const Sha1&o) { memcpy(hash, o.hash, SIZE); }
    ~Sha1() {}

    Sha1 &operator= (const Sha1 &o) { memcpy(hash,o.hash,SIZE); return *this; }

    bool operator<  (const Sha1 &o) { return memcmp(hash, o.hash, SIZE) < 0; }
    bool operator>  (const Sha1 &o) { return memcmp(hash, o.hash, SIZE) > 0; }
    bool operator== (const Sha1 &o) { return memcmp(hash, o.hash, SIZE) == 0; }
    bool operator<= (const Sha1 &o) { return memcmp(hash, o.hash, SIZE) <= 0; }
    bool operator>= (const Sha1 &o) { return memcmp(hash, o.hash, SIZE) >= 0; }
    bool operator!= (const Sha1 &o) { return memcmp(hash, o.hash, SIZE) != 0; }

    byte hash[SIZE];
};


XL_END

#endif // SHA1_H
