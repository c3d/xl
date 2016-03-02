#ifndef WINGLOB_H
#define WINGLOB_H
// ****************************************************************************
//  winglob.h                                                     ELFE project
// ****************************************************************************
//
//   File Description:
//
//    Minimalist replacement of glob() functions for Windows
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
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#ifndef CONFIG_MINGW

#include <glob.h>

#else

#include <vector>
#include <string>

typedef std::vector<std::string> globpaths;

struct glob_t
// ----------------------------------------------------------------------------
//    Minimalist replacement for struct glob
// ----------------------------------------------------------------------------
{
    size_t      gl_pathc;     // Count of total paths so far.
    globpaths   gl_pathv;     // List of paths matching pattern.
};


extern int      glob(const char *pattern,
                     int flags,
                     int (*errfunc)(const char *epath, int errno),
                     glob_t *pglob);

extern void     globfree(glob_t *pglob);


/* Only define those we need */
#define	GLOB_MARK	0x0008

#endif // CONFIG_MINGW

#endif // WINGLOB_H

