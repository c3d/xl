#ifndef BASE_H
#define BASE_H
/* ************************************************************************* */
/*   base.h                     (C) 1992-2000 Christophe de Dinechin (ddd)   */
/*                                                          XL2 project     */
/* ************************************************************************* */
/*                                                                           */
/*   File Description:                                                       */
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
/* ************************************************************************* */
/* This document is released under the GNU General Public License.           */
/* See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details    */
/*  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>            */
/*  (C) 2010 Taodyne SAS                                                     */
/* ***************************************************************************/

/* Include the places where conflicting versions of some types may exist */
#include <sys/types.h>
#include <string>
#include <cstddef>
#include <stdint.h>

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
   XL_ASSERT checks for some condition at runtime.
   XL_CASSERT checks for a condition at compile time
*/


#if !defined(XL_DEBUG) && (defined(DEBUG) || defined(_DEBUG))
#define XL_DEBUG        1
#endif

#ifdef XL_DEBUG
#define XL_ASSERT(x)   { if (!(x)) xl_assert_failed(#x, __FILE__, __LINE__); }
#define XL_CASSERT(x)  struct __dummy { char foo[((int) (x))*2-1]; }
externc void xl_assert_failed(kstring msg, kstring file, uint line);

#else
#define XL_ASSERT(x)
#define XL_CASSERT(x)
#endif


// ============================================================================
// 
//   Tracing information
// 
// ============================================================================

#ifdef XL_DEBUG
#  include "traces.h"
#  ifdef XL_TRACE_INSTNAME
#    define IFTRACE(x)    if XLTRACE(x)
#    define XLTRACE(x)    (XL_TRACE_INSTNAME && XL_TRACE_INSTNAME->x)
#    define IFTRACE2(x,y) if XLTRACE2(x,y)
#    define XLTRACE2(x,y) (XL_TRACE_INSTNAME && (XL_TRACE_INSTNAME->x || XL_TRACE_INSTNAME->y))
#  else
#    define IFTRACE(x)    if(0)
#    define XLTRACE(x)    0
#    define IFTRACE2(x,y) if(0)
#    define XLTRACE2(x,y) 0
#  endif
#else
#  define IFTRACE(x)    if(0)
#  define XLTRACE(x)    0
#  define IFTRACE2(x,y) if(0)
#  define XLTRACE2(x,y) 0
#endif



/* ========================================================================= */
/*                                                                           */
/*   Namespace support                                                       */
/*                                                                           */
/* ========================================================================= */

#if CONFIG_HAS_NAMESPACE
#define XL_BEGIN                namespace XL {
#define XL_END                  }
#else   /* !CONFIG_HAS_NAMESPACE */
#define XL_BEGIN
#define XL_END
#endif  /* ?CONFIG_HAS_NAMESPACE */


#endif /* ELEMENTALS_H */
