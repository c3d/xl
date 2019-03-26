// *****************************************************************************
// utf8_fileutils.cpp                                                 XL project
// *****************************************************************************
//
// File description:
//
//     MINGW-only wide char file support
//
//     These functions cannot be inlined easily without including
//     windows.h, which is full of short, uppercase typedefs like CONTEXT
//     or ERRROR which conflict with real symbols here.
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 0,
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

#include "config.h"

#ifndef HAVE_STRUCT_STAT
#include "utf8_fileutils.h"

#include <windows.h>
#include <fcntl.h>


std::wstring utf8_decode(const std::string &str)
// ----------------------------------------------------------------------------
//    Convert a UTF-8 std::string into a UTF-16 std::wstring
// ----------------------------------------------------------------------------
{
    int needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(),
                                     NULL, 0);
    std::wstring dest(needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(),
                        &dest[0], needed);
    return dest;
}


utf8_ifstream::utf8_ifstream(const char* s, std::ios::openmode mode)
// ----------------------------------------------------------------------------
//   Configuration when opening a steam
// ----------------------------------------------------------------------------
    : istream_type(&b),
      b(_wopen(utf8_decode(s).c_str(), wflags(mode)), mode)
{}


int utf8_ifstream::wflags(std::ios::openmode)
// ----------------------------------------------------------------------------
//   Need ot patch flags for correct behavior
// ----------------------------------------------------------------------------
{
    int flags = _O_RDONLY;
    // Experience shows that some versions of GCC misbehave on input.unget()
    // if binary mode is not set, see #2682
    flags |= _O_BINARY;
    return flags;
}


static time_t fileTimeToTime_t(FILETIME ft)
// ----------------------------------------------------------------------------
//   Conversion to more portable file time
// ----------------------------------------------------------------------------
{
   ULARGE_INTEGER ull;
   ull.LowPart = ft.dwLowDateTime;
   ull.HighPart = ft.dwHighDateTime;
   return ull.QuadPart / 10000000ULL - 11644473600ULL;
}


int utf8_stat(const char *path, struct _stat *buffer)
// ----------------------------------------------------------------------------
//   Stat function taking a UTF-8 path
// ----------------------------------------------------------------------------
{
    std::wstring wpath = utf8_decode(std::string(path));
    int status = _wstat(wpath.c_str(), buffer);
    if (status < 0)
        return status;

    // Bug#1567: _wstat returns bogus file times when daylight saving
    // time setting changes
    // By using GetFileAttributesEx we are consistent with
    // QFileInfo::lastModified().toTime_t().
    WIN32_FILE_ATTRIBUTE_DATA attr;
    bool ok = GetFileAttributesEx((const wchar_t *)wpath.c_str(),
                                  GetFileExInfoStandard, &attr);
    if (ok)
    {
        buffer->st_ctime = fileTimeToTime_t(attr.ftCreationTime);
        buffer->st_atime = fileTimeToTime_t(attr.ftLastAccessTime);
        buffer->st_mtime = fileTimeToTime_t(attr.ftLastWriteTime);
    }
    return 0;
}


int utf8_access(const char *path, int mode)
// ----------------------------------------------------------------------------
//   Access function taking a UTF-8 path
// ----------------------------------------------------------------------------
{
    std::wstring wpath = utf8_decode(std::string(path));
    return _waccess(wpath.c_str(), mode);
}


#endif // HAVE_STRUCT_STAT
