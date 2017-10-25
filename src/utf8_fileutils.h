#ifndef UTF8_IFSTREAM_H
#define UTF8_IFSTREAM_H
// ****************************************************************************
//  utf8_ifstream.h                                              XL project
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
//  (C) 2011 Jerome Forissier <jerome@taodyne.com>
//  (C) 2011 Taodyne SAS
// ****************************************************************************

#include <fstream>
#include <sys/stat.h>


#ifdef CONFIG_MINGW

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

#else

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

#endif

#endif // UTF8_FSTREAM_H
