#ifndef SHA1_OSTREAM_H
#define SHA1_OSTREAM_H
// ****************************************************************************
//  sha1_ostream.h                                                  XLR project
// ****************************************************************************
// 
//   File Description:
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
//  (C) 2010 Taodyne SAS
// ****************************************************************************

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
