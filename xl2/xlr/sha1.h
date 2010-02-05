#ifndef SHA1_H
#define SHA1_H
// ****************************************************************************
//  sha1.h                                                          XLR project
// ****************************************************************************
// 
//   File Description:
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
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Jerome Forissier <jerome@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

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
