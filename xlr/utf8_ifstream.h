#ifndef UTF8_IFSTREAM_H
#define UTF8_IFSTREAM_H
// ****************************************************************************
//  utf8_ifstream.h                                                XLR project
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
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 2011 Jerome Forissier <jerome@taodyne.com>
//  (C) 2011 Taodyne SAS
// ****************************************************************************

#include <fstream>


#ifdef CONFIG_MINGW

#include <ext/stdio_filebuf.h>
#include <windows.h>
#include <fcntl.h>

inline std::wstring utf8_decode(const std::string &str)
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

    explicit
    utf8_ifstream(const char* s)
        : istream_type(&b),
          b(_wopen(utf8_decode(s).c_str(), _O_RDONLY), std::ios_base::in)
    {}

protected:
    filebuf_type b;
};

#else

typedef std::ifstream utf8_ifstream;

#endif

#endif // UTF8_FSTREAM_H
