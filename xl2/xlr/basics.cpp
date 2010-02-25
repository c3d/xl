// ****************************************************************************
//  basics.cpp                      (C) 1992-2009 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Basic operations (arithmetic, ...)
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
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include <iostream>
#include <sstream>
#include <ctime>
#include <cstdio>

#include "basics.h"
#include "context.h"
#include "renderer.h"
#include "opcodes.h"
#include "compiler.h"
#include "options.h"
#include "runtime.h"
#include "types.h"

XL_BEGIN

// ============================================================================
// 
//    Some utility functions used in basics.tbl
// 
// ============================================================================

static inline integer_t xl_mod(integer_r xr, integer_r yr)
// ----------------------------------------------------------------------------
//   Compute a mathematical 'mod' from the C99 % operator
// ----------------------------------------------------------------------------
{
    integer_t x = XL_INT(xr);
    integer_t y = XL_INT(yr);
    integer_t tmp = x % y;
    if (tmp && (x^y) < 0)
        tmp += y;
    return tmp;
}


static inline integer_t xl_pow(integer_r xr, integer_r yr)
// ----------------------------------------------------------------------------
//   Compute integer power
// ----------------------------------------------------------------------------
{
    integer_t x = XL_INT(xr);
    integer_t y = XL_INT(yr);
    integer_t tmp = 0;
    if (y >= 0)
    {
        tmp = 1;
        while (y)
        {
            if (y & 1)
                tmp *= x;
            x *= x;
            y >>= 1;
        }
    }
    return tmp;
}


static inline real_t xl_modf(real_r xr, real_r yr)
// ----------------------------------------------------------------------------
//   Compute a mathematical 'mod' from fmod
// ----------------------------------------------------------------------------
{
    real_t x = XL_REAL(xr);
    real_t y = XL_REAL(yr);
    real_t tmp = fmod(x,y);
    if (tmp != 0.0 && x*y < 0.0)
        tmp += y;
    return tmp;
}


static inline real_t xl_powf(real_r xr, integer_r yr)
// ----------------------------------------------------------------------------
//   Compute real power with an integer on the right
// ----------------------------------------------------------------------------
{
    real_t x = XL_REAL(xr);
    integer_t y = XL_INT(yr);
    boolean_t negative = y < 0;
    real_t tmp = 1.0;
    if (negative)
        y = -y;
    while (y)
    {
        if (y & 1)
            tmp *= x;
        x *= x;
        y >>= 1;
    }
    if (negative)
        tmp = 1.0/tmp;
    return tmp;
}


static inline integer_t xl_time()
// ----------------------------------------------------------------------------
//   Return the current system time
// ----------------------------------------------------------------------------
{
    time_t t;
    time(&t);
    return t;
}

#define XL_RTIME(tmfield)                       \
    struct tm tm;                               \
    time_t clock = t;                           \
    localtime_r(&clock, &tm);                   \
    XL_RINT(tmfield)

#define XL_RCTIME(tmfield)                      \
    struct tm tm;                               \
    time_t clock;                               \
    time(&clock);                               \
    localtime_r(&clock, &tm);                   \
    XL_RINT(tmfield)


template<typename number>
static inline number xl_random(number low, number high)
// ----------------------------------------------------------------------------
//    Return a pseudo-random number in the low..high range
// ----------------------------------------------------------------------------
{
#ifndef CONFIG_MINGW
    double base = drand48();
#else
    double base = double(rand()) / RAND_MAX;
#endif // CONFIG_MINGW
    return number(base * (high-low) + low);
}


#ifdef CONFIG_MINGW
static struct tm *localtime_r (const time_t * timep, struct tm * result)
// ----------------------------------------------------------------------------
//   MinGW doesn't have localtime_r, but its localtime is thread-local storage
// ----------------------------------------------------------------------------
{
    *result = *localtime (timep);
    return result;
}
#endif // CONFIG_MINGW




// ============================================================================
// 
//    Top-level operation
// 
// ============================================================================

#include "opcodes_declare.h"
#include "basics.tbl"


void EnterBasics(Context *c)
// ----------------------------------------------------------------------------
//   Enter all the basic operations defined in basics.tbl
// ----------------------------------------------------------------------------
{
    Compiler *compiler = c->compiler;
#include "opcodes_define.h"
#include "basics.tbl"
}

XL_END
