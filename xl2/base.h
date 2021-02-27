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
// This software is licensed under the GNU General Public License v3+
// (C) 2003,2009-2010,2014-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
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
#include <stddef.h>
#include <string>

/* Include the configuration file */
#include "configuration.h"


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
#if CONFIG_HAS_UCHAR
typedef unsigned char   uchar;
#endif

#if CONFIG_HAS_USHORT
typedef unsigned short  ushort;
#endif

#if CONFIG_HAS_UINT
typedef unsigned int    uint;
#endif

#if CONFIG_HAS_ULONG
typedef unsigned long   ulong;
#endif

/* The largest available integer type */
#if CONFIG_HAS_LONGLONG
typedef long long          longlong;
typedef unsigned long long ulonglong;
#elif CONFIG_HAS_INT64
typedef __int64          longlong;
typedef unsigned __int64 ulonglong;
#else
typedef long            longlong;
typedef unsigned long   ulonglong;
#endif

/* Sized integers (dependent on the compiler...) - Only when needed */
typedef char            int8;
typedef short           int16;
typedef long            int32;
typedef long long       int64;

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;
typedef unsigned long long uint64;

/* A type that can be used to cast a pointer without data loss */
typedef long            ptrint;


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

#if CONFIG_HAS_NAMESPACE
#define MZ_BEGIN                namespace Mozart {
#define MZ_END                  }
#else   /* !CONFIG_HAS_NAMESPACE */
#define MZ_BEGIN
#define MZ_END
#endif  /* ?CONFIG_HAS_NAMESPACE */


#endif /* ELEMENTALS_H */
