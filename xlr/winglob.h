#ifndef WINGLOB_H
#define WINGLOB_H
// ****************************************************************************
//  winglob.h                                                       XLR project
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
// This document is released under the GNU General Public License.
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

