#ifndef SHA1_H
#define SHA1_H

// ****************************************************************************
//  sha1.h                          (C) XXX
//                                                            XL2 project
// ****************************************************************************
//
//   File Description:
//
//     Function prototypes for the SHA-1 algorithm.
//
// Usage example:
//
//   char msg[] = "Message to hash";
//   SHA1_CONTEXT ctx;
//   byte *hash;
//
//   sha1_init(&ctx);
//   sha1_write(&ctx, (byte *)msg, sizeof(msg)-1);
//   sha1_final(&ctx);
//   hash = sha1_read(&ctx);
//   printf("SHA-1 hash of '%s' is ", msg);
//   for (int i = 0; i < SHA1_SIZE; i++)
//     printf("%02x", hash[i]);
//   printf("\n");
//
//
// ****************************************************************************
// This program is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "base.h"

XL_BEGIN

typedef struct {
    uint32 h0,h1,h2,h3,h4;
    uint32 nblocks;
    byte   buf[64];
    int    count;
} SHA1_CONTEXT;

#define SHA1_SIZE 20  /* 20 bytes = 160 bits */

void sha1_init(SHA1_CONTEXT *hd);
void sha1_write(SHA1_CONTEXT *hd, byte *inbuf, size_t inlen);
void sha1_final(SHA1_CONTEXT *hd);
byte *sha1_read(SHA1_CONTEXT *hd);

XL_END

#endif // SHA1_H
