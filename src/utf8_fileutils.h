#ifndef UTF8_FILEUTILS_H
#define UTF8_FILEUTILS_H
// ****************************************************************************
//  utf8_fileutils.h                                              XL project
// ****************************************************************************
//
//   File Description:
//
//    A class similar to ifstream, with support for UTF-8 encoded filenames.
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
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 2011 Jerome Forissier <jerome@taodyne.com>
//  (C) 2011 Taodyne SAS
// ****************************************************************************

#include "config.h"
#include <fstream>
#include <sys/stat.h>


#ifndef HAVE_STRUCT_STAT

#include <ext/stdio_filebuf.h>

typedef struct _stat utf8_filestat_t;

std::wstring utf8_decode(const std::string &str);

class utf8_ifstream : public std::istream
// ----------------------------------------------------------------------------
//    Quick-and-dirty ifstream with support for UTF-8 encoded file names
// ----------------------------------------------------------------------------
{

public:
    typedef std::istream                   istream_type;
    typedef __gnu_cxx::stdio_filebuf<char> filebuf_type;

public:
    utf8_ifstream()
        : istream_type()
    {}
    explicit utf8_ifstream(const char* s,
                           std::ios::openmode mode = std::ios::in);

protected:
    int wflags(std::ios::openmode);

protected:
    filebuf_type b;
};


extern int      utf8_stat(const char *path, struct _stat *buffer);
extern int      utf8_access(const char *path, int mode);


inline bool isDirectorySeparator(int c)
// ----------------------------------------------------------------------------
//   For Losedows, can be / or \ (and I need this to avoid multiline comment)
// ----------------------------------------------------------------------------
{
    return c == '/' || c == '\\';
}

#else // HAVE_STRUCT_STAT

// Real systems
typedef struct stat   utf8_filestat_t;
typedef std::ifstream utf8_ifstream;
#define utf8_stat(path, buffer) stat(path, buffer)
#define utf8_access(path, mode) access(path, mode)

inline bool isDirectorySeparator(int c)
// ----------------------------------------------------------------------------
//   For real systems, the directory separator is /
// ----------------------------------------------------------------------------
{
    return c == '/';
}

#endif // HAVE_STRUCT_STAT

#endif // UTF8_FILEUTILS_H
