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
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include <iostream>
#include <stdio.h>
#include "sha1.h"

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

#endif // SHA1_OSTREAM_H
