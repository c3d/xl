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
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Jerome Forissier <jerome@taodyne.com>
//  (C) 2010 Taodyne SAS
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
