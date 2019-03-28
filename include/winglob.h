#ifndef WINGLOB_H
#define WINGLOB_H
// *****************************************************************************
// winglob.h                                                          XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010,2015-2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
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

#ifdef HAVE_GLOB

#include <glob.h>

#else // !HAVE_GLOB

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

#endif // !HAVE_GLOB

#endif // WINGLOB_H
