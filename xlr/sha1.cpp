// ****************************************************************************
//  sha1.cpp                                                        XLR project
// ****************************************************************************
// 
//   File Description:
// 
//     Implementation of the SHA-1 algorithm
//     Adapted from the GnuPG project, http://www.gnupg.org/
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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

/*  Test vectors:
 *
 *  "abc"
 *  A999 3E36 4706 816A BA3E  2571 7850 C26C 9CD0 D89D
 *
 *  "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
 *  8498 3E44 1C3B D26E BAAE  4AA1 F951 29E5 E546 70F1
 */

#include "sha1.h"
#include <string.h>

XL_BEGIN

// ============================================================================
// 
//   Utilities, macros, ...
// 
// ============================================================================

#define wipememory2(_ptr,_set,_len) do {                             \
  volatile char *_vptr=(volatile char *)(_ptr); size_t _vlen=(_len); \
  while(_vlen) { *_vptr=(_set); _vptr++; _vlen--; } } while(0)


#define wipememory(_ptr,_len) wipememory2(_ptr,0,_len)


#if defined(__i386__) || defined(__x86_64__) || defined(__arm__)
#elif defined(__ppc__) || defined(__ppc64__)
#define BIG_ENDIAN_HOST 1
#else
#error Unknown endianness
#endif


static inline uint32 rol(uint32 x, int n)
// ----------------------------------------------------------------------------
//   Rotate a 32-bit integer by n bytes
// ----------------------------------------------------------------------------
{
#if defined(__GNUC__) && defined(__i386__)
	__asm__("roll %%cl,%0"
		:"=r" (x)
		:"0" (x),"c" (n));
	return x;
#else
        return (x << n) | (x >> (32-n));
#endif
}


static void burn_stack (int bytes)
// ----------------------------------------------------------------------------
//   Make sure we don't leave anything on stack
// ----------------------------------------------------------------------------
{
    char buf[128];

    wipememory(buf,sizeof buf);
    bytes -= sizeof buf;
    if (bytes > 0)
        burn_stack (bytes);
}




// ============================================================================
// 
//    Sha1::Computation class
// 
// ============================================================================

Sha1::Computation::Computation()
// ----------------------------------------------------------------------------
//   Set initial values
// ----------------------------------------------------------------------------
{
    reset();
}

void Sha1::Computation::reset()
// ----------------------------------------------------------------------------
//   Reset computation state
// ----------------------------------------------------------------------------
{
    h0 = 0x67452301;
    h1 = 0xefcdab89;
    h2 = 0x98badcfe;
    h3 = 0x10325476;
    h4 = 0xc3d2e1f0;
    nblocks = 0;
    count = 0;
}


Sha1::Computation& Sha1::Computation::transform(byte *data)
// ----------------------------------------------------------------------------
//   Transform a message which consists of 16 32-bit-words
// ----------------------------------------------------------------------------
{
    uint32 a,b,c,d,e,tm;
    uint32 x[16];

    // Get values from the chaining vars
    a = this->h0;
    b = this->h1;
    c = this->h2;
    d = this->h3;
    e = this->h4;
    
#ifdef BIG_ENDIAN_HOST
    memcpy(x, data, sizeof x);
#else
    int i;
    byte *p2;
    for(i=0, p2=(byte*)x; i < 16; i++, p2 += 4)
    {
	p2[3] = *data++;
	p2[2] = *data++;
	p2[1] = *data++;
	p2[0] = *data++;
    }
#endif


#define K1  0x5A827999L
#define K2  0x6ED9EBA1L
#define K3  0x8F1BBCDCL
#define K4  0xCA62C1D6L
#define F1(x,y,z)   (z ^ (x & (y ^ z)))
#define F2(x,y,z)   (x ^ y ^ z)
#define F3(x,y,z)   ((x & y) | (z & (x | y)))
#define F4(x,y,z)   (x ^ y ^ z)


#define M(i) (tm =   x[i&0x0f] ^ x[(i-14)&0x0f] \
                 ^ x[(i-8)&0x0f] ^ x[(i-3)&0x0f] \
                 , (x[i&0x0f] = rol(tm,1)))

#define R(a,b,c,d,e,f,k,m)  do { e += rol(a, 5)     \
                                      + f(b, c, d)  \
                                      + k             \
                                      + m;            \
                                 b = rol(b, 30);    \
                               } while(0)
    R(a, b, c, d, e, F1, K1, x[ 0]);
    R(e, a, b, c, d, F1, K1, x[ 1]);
    R(d, e, a, b, c, F1, K1, x[ 2]);
    R(c, d, e, a, b, F1, K1, x[ 3]);
    R(b, c, d, e, a, F1, K1, x[ 4]);
    R(a, b, c, d, e, F1, K1, x[ 5]);
    R(e, a, b, c, d, F1, K1, x[ 6]);
    R(d, e, a, b, c, F1, K1, x[ 7]);
    R(c, d, e, a, b, F1, K1, x[ 8]);
    R(b, c, d, e, a, F1, K1, x[ 9]);
    R(a, b, c, d, e, F1, K1, x[10]);
    R(e, a, b, c, d, F1, K1, x[11]);
    R(d, e, a, b, c, F1, K1, x[12]);
    R(c, d, e, a, b, F1, K1, x[13]);
    R(b, c, d, e, a, F1, K1, x[14]);
    R(a, b, c, d, e, F1, K1, x[15]);
    R(e, a, b, c, d, F1, K1, M(16));
    R(d, e, a, b, c, F1, K1, M(17));
    R(c, d, e, a, b, F1, K1, M(18));
    R(b, c, d, e, a, F1, K1, M(19));
    R(a, b, c, d, e, F2, K2, M(20));
    R(e, a, b, c, d, F2, K2, M(21));
    R(d, e, a, b, c, F2, K2, M(22));
    R(c, d, e, a, b, F2, K2, M(23));
    R(b, c, d, e, a, F2, K2, M(24));
    R(a, b, c, d, e, F2, K2, M(25));
    R(e, a, b, c, d, F2, K2, M(26));
    R(d, e, a, b, c, F2, K2, M(27));
    R(c, d, e, a, b, F2, K2, M(28));
    R(b, c, d, e, a, F2, K2, M(29));
    R(a, b, c, d, e, F2, K2, M(30));
    R(e, a, b, c, d, F2, K2, M(31));
    R(d, e, a, b, c, F2, K2, M(32));
    R(c, d, e, a, b, F2, K2, M(33));
    R(b, c, d, e, a, F2, K2, M(34));
    R(a, b, c, d, e, F2, K2, M(35));
    R(e, a, b, c, d, F2, K2, M(36));
    R(d, e, a, b, c, F2, K2, M(37));
    R(c, d, e, a, b, F2, K2, M(38));
    R(b, c, d, e, a, F2, K2, M(39));
    R(a, b, c, d, e, F3, K3, M(40));
    R(e, a, b, c, d, F3, K3, M(41));
    R(d, e, a, b, c, F3, K3, M(42));
    R(c, d, e, a, b, F3, K3, M(43));
    R(b, c, d, e, a, F3, K3, M(44));
    R(a, b, c, d, e, F3, K3, M(45));
    R(e, a, b, c, d, F3, K3, M(46));
    R(d, e, a, b, c, F3, K3, M(47));
    R(c, d, e, a, b, F3, K3, M(48));
    R(b, c, d, e, a, F3, K3, M(49));
    R(a, b, c, d, e, F3, K3, M(50));
    R(e, a, b, c, d, F3, K3, M(51));
    R(d, e, a, b, c, F3, K3, M(52));
    R(c, d, e, a, b, F3, K3, M(53));
    R(b, c, d, e, a, F3, K3, M(54));
    R(a, b, c, d, e, F3, K3, M(55));
    R(e, a, b, c, d, F3, K3, M(56));
    R(d, e, a, b, c, F3, K3, M(57));
    R(c, d, e, a, b, F3, K3, M(58));
    R(b, c, d, e, a, F3, K3, M(59));
    R(a, b, c, d, e, F4, K4, M(60));
    R(e, a, b, c, d, F4, K4, M(61));
    R(d, e, a, b, c, F4, K4, M(62));
    R(c, d, e, a, b, F4, K4, M(63));
    R(b, c, d, e, a, F4, K4, M(64));
    R(a, b, c, d, e, F4, K4, M(65));
    R(e, a, b, c, d, F4, K4, M(66));
    R(d, e, a, b, c, F4, K4, M(67));
    R(c, d, e, a, b, F4, K4, M(68));
    R(b, c, d, e, a, F4, K4, M(69));
    R(a, b, c, d, e, F4, K4, M(70));
    R(e, a, b, c, d, F4, K4, M(71));
    R(d, e, a, b, c, F4, K4, M(72));
    R(c, d, e, a, b, F4, K4, M(73));
    R(b, c, d, e, a, F4, K4, M(74));
    R(a, b, c, d, e, F4, K4, M(75));
    R(e, a, b, c, d, F4, K4, M(76));
    R(d, e, a, b, c, F4, K4, M(77));
    R(c, d, e, a, b, F4, K4, M(78));
    R(b, c, d, e, a, F4, K4, M(79));

    /* update chainig vars */
    this->h0 += a;
    this->h1 += b;
    this->h2 += c;
    this->h3 += d;
    this->h4 += e;

    return *this;
}


Sha1::Computation& Sha1::Computation::operator()(const void *inptr,
                                                 size_t inlen)
// ----------------------------------------------------------------------------
// Update the message digest with the contents of inptr with inlen bytes
// ----------------------------------------------------------------------------
{
    byte *inbuf = (byte *) inptr;

    if(this->count == 64)
    {
        // Flush the buffer
	transform(this->buf);
        burn_stack (88+4*sizeof(void*));
	this->count = 0;
	this->nblocks++;
    }
    if(!inbuf)
	return *this;
    if(this->count)
    {
	for(; inlen && this->count < 64; inlen--)
	    this->buf[this->count++] = *inbuf++;
	(*this)(NULL, 0);
	if(!inlen)
	    return *this;
    }

    while(inlen >= 64)
    {
	transform(inbuf);
	this->count = 0;
	this->nblocks++;
	inlen -= 64;
	inbuf += 64;
    }
    burn_stack (88+4*sizeof(void*));
    for(; inlen && this->count < 64; inlen--)
	this->buf[this->count++] = *inbuf++;

    return *this;
}


Sha1::Computation& Sha1::Computation::finalize()
// ----------------------------------------------------------------------------
//   Terminate the computation and return the digest
// ----------------------------------------------------------------------------
//  The handle is prepared for a new cycle, but adding bytes to the
//  handle will the destroy the returned buffer.

{
    uint32 t, msb, lsb;
    byte *p;

    (*this) (NULL, 0); // Flush

    t = this->nblocks;

    // Multiply by 64 to make a byte count
    lsb = t << 6;
    msb = t >> 26;

    // Add the count
    t = lsb;
    if((lsb += this->count) < t)
	msb++;

    // Multiply by 8 to make a bit count
    t = lsb;
    lsb <<= 3;
    msb <<= 3;
    msb |= t >> 29;

    if (this->count < 56)
    {
        // Enough room
	this->buf[this->count++] = 0x80;        // Pad
	while(this->count < 56)
	    this->buf[this->count++] = 0;       // Pad
    }
    else
    {
        // Need one extra block
	this->buf[this->count++] = 0x80;        // Pad character
	while(this->count < 64)
	    this->buf[this->count++] = 0;
        (*this) (NULL, 0);                      // Flush
	memset(this->buf, 0, 56);               // Fill next block with zeroes
    }
    
    // Append the 64 bit count
    this->buf[56] = msb >> 24;
    this->buf[57] = msb >> 16;
    this->buf[58] = msb >>  8;
    this->buf[59] = msb	   ;
    this->buf[60] = lsb >> 24;
    this->buf[61] = lsb >> 16;
    this->buf[62] = lsb >>  8;
    this->buf[63] = lsb	   ;
    transform(this->buf);
    burn_stack (88+4*sizeof(void*));

    p = this->buf;
#ifdef BIG_ENDIAN_HOST
#define X(a) do { *(uint32*)p = this->h##a ; p += 4; } while(0)
#else /* little endian */
#define X(a) do { *p++ = this->h##a >> 24; *p++ = this->h##a >> 16;	 \
                  *p++ = this->h##a >> 8; *p++ = this->h##a; } while(0)
#endif
    X(0);
    X(1);
    X(2);
    X(3);
    X(4);
#undef X

    return *this;
}

XL_END


#ifdef UNIT_TEST

#include <cassert>
#include <sstream>
#include "sha1_ostream.h"


int main (int argc, char **argv)
// ----------------------------------------------------------------------------
//   Apply the test on a known input
// ----------------------------------------------------------------------------

{
  char msg[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
  char expected[] = "84983e441c3bd26ebaae4aa1f95129e5e54670f1";

  XL::Sha1::Computation cmp;
  cmp(msg, sizeof msg - 1);
  XL::Sha1 sha1 (cmp);

  std::cout << "SHA1=" << sha1 << '\n';

  std::ostringstream os;
  os << sha1;
  if (os.str() != expected)
      std::cerr << "SHA-1 implementation does not work!\n";
}

#endif

