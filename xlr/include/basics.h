#ifndef BASICS_H
// *****************************************************************************
// basics.h                                                           XL project
// *****************************************************************************
//
// File description:
//
//      Basic operations (arithmetic, etc)
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
// (C) 2012, Baptiste Soulisse <baptiste.soulisse@taodyne.com>
// (C) 2009-2011,2014,2019, Christophe de Dinechin <christophe@dinechin.org>
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

#include "tree.h"
#include "context.h"
#include "opcodes.h"
#include "main.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>

XL_BEGIN

// ============================================================================
//
//   Top level entry point
//
// ============================================================================

// Top-level entry point: enter all basic operations in the context
void EnterBasics();

// Top-level entry point: delete all globals related to basic operations
void DeleteBasics();



// ============================================================================
//
//    Some utility functions used in basics.tbl
//
// ============================================================================

inline longlong xl_text2int(text_r t)
// ----------------------------------------------------------------------------
//   Converts text to a numerical value
// ----------------------------------------------------------------------------
{
    std::istringstream stream(t.value);
    longlong result = 0.0;
    stream >> result;
    return result;
}


inline double xl_text2real(text_r t)
// ----------------------------------------------------------------------------
//   Converts text to a numerical value
// ----------------------------------------------------------------------------
{
    std::istringstream stream(t.value);
    double result = 0.0;
    stream >> result;
    return result;
}


inline text xl_int2text(longlong value)
// ----------------------------------------------------------------------------
//   Convert a numerical value to text
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << value;
    return out.str();
}


inline text xl_real2text(double value)
// ----------------------------------------------------------------------------
//   Convert a numerical value to text
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << value;
    return out.str();
}


inline integer_t xl_mod(integer_r xr, integer_r yr)
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


inline integer_t xl_pow(integer_r xr, integer_r yr)
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


inline real_t xl_modf(real_r xr, real_r yr)
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


inline real_t xl_powf(real_r xr, integer_r yr)
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


inline integer_t xl_time(double delay)
// ----------------------------------------------------------------------------
//   Return the current system time
// ----------------------------------------------------------------------------
{
    time_t t;
    time(&t);
    MAIN->Refresh(delay);
    return t;
}


#define XL_RTIME(tmfield)                       \
    struct tm tm;                               \
    time_t clock = t;                           \
    localtime_r(&clock, &tm);                   \
    XL_RINT(tmfield)


#define XL_RCTIME(tmfield, delay)               \
    struct tm tm;                               \
    time_t clock;                               \
    time(&clock);                               \
    localtime_r(&clock, &tm);                   \
    MAIN->Refresh(delay);                       \
    XL_RINT(tmfield)


template<typename number>
inline number xl_random(number low, number high)
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

inline bool xl_random_seed(int seed)
// ----------------------------------------------------------------------------
//    Initialized random number generator using the argument passed as seed
// ----------------------------------------------------------------------------
{
#ifndef CONFIG_MINGW
    srand48(seed);
#else
    srand(seed);
#endif // CONFIG_MINGW

    return true;
}

#ifdef CONFIG_MINGW
#ifndef localtime_r
inline struct tm *localtime_r (const time_t * timep, struct tm * result)
// ----------------------------------------------------------------------------
//   MinGW doesn't have localtime_r, but its localtime is thread-local storage
// ----------------------------------------------------------------------------
{
    *result = *localtime (timep);
    return result;
}
#endif
#endif // CONFIG_MINGW


inline text xl_text_replace(text txt, text before, text after)
// ----------------------------------------------------------------------------
//   Return a copy of txt with all occurrences of before replaced with after
// ----------------------------------------------------------------------------
{
  size_t pos = 0;
  while ((pos = txt.find(before, pos)) != std::string::npos)
  {
     txt.replace(pos, before.length(), after);
     pos += after.length();
  }
  return txt;
}


XL_END

#endif // BASICS_H
