// *****************************************************************************
// winglob.cpp                                                        XL project
// *****************************************************************************
//
// File description:
//
//
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

#include "config.h"

#ifndef HAVE_GLOB

#include "base.h"
#include "winglob.h"
#include <dirent.h>
#include <string>
#include <windows.h>


static void globinternal(text dir, text pattern, globpaths &paths)
// ----------------------------------------------------------------------------
//   Internal variant of glob, where dir is a directory to look into
// ----------------------------------------------------------------------------
{
    // Check if we are scanning a directory
    text subpat = "";
    bool dirmode = false;

    // Transform the regexp into a file-style regexp
    size_t pos = pattern.find_first_of("/\\");
    if (pos != pattern.npos)
    {
        subpat = pattern.substr(pos + 1, pattern.npos);
        pattern = pattern.substr(0, pos-1);
        dirmode = true;
    }

    // Open directory
    WIN32_FIND_DATAA fdata;
    HANDLE fhandle = FindFirstFileA(dir.c_str(), &fdata);
    bool more = fhandle != INVALID_HANDLE_VALUE;
    if (more)
    {
        do
        {
            std::string entry(fdata.cFileName);
            if (dirmode)
                globinternal(dir + "\\" + entry, subpat, paths);
            else
                paths.push_back(entry);
            more = FindNextFileA(fhandle, &fdata);
        } while (more);
        FindClose(fhandle);
    }
}


int glob(const char *pattern,
         int flags,
         int (*errfunc)(const char *epath, int errno),
         glob_t *pglob)
// ----------------------------------------------------------------------------
//   Simulation of the glob() function on Windows
// ----------------------------------------------------------------------------
{
    (void)flags;
    (void)errfunc;
    globinternal("", pattern, pglob->gl_pathv);
    pglob->gl_pathc = pglob->gl_pathv.size();
    return 0;
}


void globfree(glob_t *pglob)
// ----------------------------------------------------------------------------
//   Simulation of the globfree() function on Windows
// ----------------------------------------------------------------------------
{
    pglob->gl_pathv.clear();
}

#endif // !HAVE_GLOB
