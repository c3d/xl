#ifndef BASE_H
#define BASE_H
// *****************************************************************************
// base.h                                                             XL project
// *****************************************************************************
//
// File description:
/*                                                                           */
/*     Most basic things in the Mozart system:                               */
/*     - Basic types                                                         */
/*     - Debugging macros                                                    */
/*     - Derived configuration information                                   */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2003,2019, Christophe de Dinechin <christophe@dinechin.org>
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

/* Include the places where conflicting versions of some types may exist */
#include <sys/types.h>
#include <string>
#include <cstddef>
#include <stdint.h>

/* Auto-generated configuration (see libxlr/config.h; Makefile writes build dir copy) */
#include "../../config.h"


/* ========================================================================= */
/*                                                                           */
/*    C/C++ differences that matter for include files                        */
/*                                                                           */
/* ========================================================================= */


#ifndef NULL
#  ifdef __cplusplus
#    define NULL        0
#  else
#    define NULL        ((void *) 0)
#  endif
#endif


#ifndef externc
#  ifdef __cplusplus
#    define externc     extern "C"
#  else
#    define externc     extern
#endif
#endif


#ifndef inline
#  ifndef __cplusplus
#    if defined(__GNUC__)
#      define inline      static __inline__
#    else
#      define inline      static
#    endif
#  endif
#endif


#ifndef true
#  ifndef __cplusplus
#    define true        1
#    define false       0
#    define bool        char
#  endif
#endif



/*===========================================================================*/
/*                                                                           */
/*  Common types                                                             */
/*                                                                           */
/*===========================================================================*/

/* Used for some byte manipulation, where it is more explicit that uchar */
typedef unsigned char   byte;


/* Shortcuts for unsigned numbers - Quite uneasy to figure out in general */
#ifndef HAVE_UCHAR
typedef unsigned char   uchar;
#endif

#ifndef HAVE_USHORT
typedef unsigned short  ushort;
#endif

#ifndef HAVE_UINT
typedef unsigned int    uint;
#endif

#ifndef HAVE_ULONG
typedef unsigned long   ulong;
#endif

/* The largest available integer type */
#ifdef HAVE_LONGLONG
typedef long long          longlong;
typedef unsigned long long ulonglong;
#elif HAVE_INT64
typedef __int64          longlong;
typedef unsigned __int64 ulonglong;
#else
typedef long            longlong;
typedef unsigned long   ulonglong;
#endif /* LONGLONG or INT64 */

/* Sized integers (dependent on the compiler...) - Only when needed */
typedef int8_t          int8;
typedef int16_t         int16;
typedef int32_t         int32;
typedef int64_t         int64;

typedef uint8_t         uint8;
typedef uint16_t        uint16;
typedef uint32_t        uint32;
typedef uint64_t        uint64;

/* A type that can be used to cast a pointer without data loss */
typedef intptr_t        ptrint;


/* Constant and non constant C-style string and void pointer */
typedef char *          cstring;
typedef const char *    kstring;
typedef void *          void_p;
typedef std::string     text;

/* Unicode character */
typedef wchar_t         wchar;
typedef wchar *         wcstring;
typedef const wchar *   wkstring;



/* ========================================================================= */
/*                                                                           */
/*   Debug information                                                       */
/*                                                                           */
/* ========================================================================= */
/*
   MZ_ASSERT checks for some condition at runtime.
   MZ_CASSERT checks for a condition at compile time
*/


#if !defined(MZ_DEBUG) && (defined(DEBUG) || defined(_DEBUG))
#define MZ_DEBUG        1
#endif

#ifdef MZ_DEBUG
#define MZ_ASSERT(x)   { if (!(x)) mz_assert_failed(#x, __FILE__, __LINE__); }
#define MZ_CASSERT(x)  char __dummy[((int) (x))*2-1]
externc void mz_assert_failed(kstring msg, kstring file, uint line);

#else
#define MZ_ASSERT(x)
#define MZ_CASSERT(x)
#endif


// ============================================================================
//
//   Tracing information
//
// ============================================================================

#ifdef MZ_DEBUG
extern ulong mz_traces;
#  define IFTRACE(x)    if (mz_traces & (1 << MZ_TRACE_##x))
#else
#  define IFTRACE(x)    if(0)
#endif



/* ========================================================================= */
/*                                                                           */
/*   Namespace support                                                       */
/*                                                                           */
/* ========================================================================= */

#ifdef __cplusplus
#define MZ_BEGIN                namespace Mozart {
#define MZ_END                  }
#else
#define MZ_BEGIN
#define MZ_END
#endif


#endif /* ELEMENTALS_H */
