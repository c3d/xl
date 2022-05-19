// *****************************************************************************
// xl_lib.h                                                           XL project
// *****************************************************************************
//
// File description:
//
//      Default support library for XL
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2003-2008,2015,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010, Thomas Hume <aszarsha@gmail.com>
// (C) 2005, Sébastien Brochet <sebbrochet@sourceforge.net>
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

#include <cassert>
#include <cstdio>
#include <string>
#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>
#include <map>
#include <ciso646>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MSC_VER
#if !defined(CONFIG_HPUX)   && !defined(__hpux)      && \
    !defined(CONFIG_CYGWIN) && !defined(CONFIG_MINGW)
#include <sys/dir.h>
#else
#include <dirent.h>
#endif
#endif


// ============================================================================
//
//    Instructions
//
// ============================================================================

#define restart() continue
#define call(x) (void) (x)


// ============================================================================
//
//    Basic types
//
// ============================================================================

typedef std::string text;

inline int length(text &t) { return t.length(); }

template<class T>
inline std::vector<T> &operator += (std::vector<T> & what, const T& last)
{
    what.push_back(last);
    return what;
}


template<class T>
inline std::vector<T> &operator += (std::vector<T> & what,
                                    const std::vector<T>& last)
{
    what.insert(what.end(), last.begin(), last.end());
    return what;
}


template<class T>
inline int size (std::vector<T> & what)
{
    return what.size();
}


template <class T, class U>
inline int count(const std::map<T,U> &m, const T &v)
{
    return m.count(v);
}


template <class T, class U>
inline int erase(std::map<T,U> &m, const T &v)
{
    return m.erase(v);
}


template<class T>
inline T & back(std::vector<T> &v)
{
    if (v.size() <= 0)
    {
        std::cerr << "Ouch, trying to get back of empty vector";
        abort();
    }

    return v.back();
}


inline bool back(std::vector<bool> &v)
{
    if (v.size() <= 0)
    {
        std::cerr << "Ouch, trying to get back of empty vector";
        abort();
    }

    return v.back();
}


template<class T>
inline void clear(std::vector<T> &v)
{
    v.clear();
}


template<class T>
void popback(std::vector<T> &v)
{
    if (v.size() <= 0)
    {
        std::cerr << "Ouch, trying to pop last vector element";
        abort();
    }

    v.pop_back();
}


template <class T>
T *clone(T *input)
{
    if (input)
        return new T(*input);
    return 0;
}


template <class T>
void copy(T *output, T *input)
{
    new(output) T(input);
}



// ============================================================================
//
//    Text I/O
//
// ============================================================================

namespace xl
{
    namespace parser {
        namespace tree {
            struct treenode;
        }
    }

    namespace textio
    {
        typedef std::iostream *file;
        typedef std::ostream *outputfile;
        typedef std::istream *inputfile;
        typedef std::ostringstream *outputstringstream;
        typedef std::ios_base::openmode openmode;
        const openmode readmode = std::ios_base::in;
        const openmode writemode = std::ios_base::out;

        std::ostream *standardoutput = &std::cout;
        std::ostream *standarderror = &std::cerr;
        std::istream *standardinput = &std::cin;

        inline file open(text name, openmode mode = readmode)
        { return new std::fstream(name.c_str(), mode); }
        inline void close(file f) { delete f; }
        inline void putback(file f, char c) { f->putback(c); }
        inline bool valid(file f) { return f->good(); }

        inline outputstringstream openstringstream()
        { return new std::ostringstream; }
        inline text closestringstream(outputstringstream s)
        {
            text result = s->str();
            delete s;
            return result;
        }


        // Since at this point we don't have valid varags
        // (unlike in the Mozart version), we just mimic them...
        // exactly to the extent the bootstrap compiler needs it...
        void write(xl::parser::tree::treenode *);
        template <class A>
        inline void write(const A& a)
        { (*standardoutput) << a; }
        template <class A, class B>
        inline void write(const A& a, const B& b)
        { write(a); write(b); }

        template <class A, class B, class C>
        inline void write(const A& a, const B& b, const C& c)
        { write(a); write(b, c); }

        template <class A, class B, class C, class D>
        inline void write(const A& a, const B& b, const C& c, const D& d)
        { write(a); write(b, c, d); }

        template <class A, class B, class C, class D,
                  class E>
        inline void write(const A& a, const B& b, const C& c, const D& d,
                          const E& e)
        { write(a); write(b, c, d, e); }

        template <class A, class B, class C, class D,
                  class E, class F>
        inline void write(const A& a, const B& b, const C& c, const D& d,
                          const E& e, const F& f)
        { write(a); write(b, c, d, e, f); }

        template <class A, class B, class C, class D,
                  class E, class F, class G>
        inline void write(const A& a, const B& b, const C& c, const D& d,
                          const E& e, const F& f, const G& g)
        { write(a); write(b, c, d, e, f, g); }

        template <class A, class B, class C, class D,
                  class E, class F, class G, class H>
        inline void write(const A& a, const B& b, const C& c, const D& d,
                          const E& e, const F& f, const G& g, const H& h)
        { write(a); write(b, c, d, e, f, g, h); }

        inline void writeln() { (*standardoutput) << '\n'; }

        template <class A>
        inline void writeln(const A& a)
        { write(a); writeln(); }

        template <class A, class B>
        inline void writeln(const A& a, const B& b)
        { write(a, b); writeln(); }

        template <class A, class B, class C>
        inline void writeln(const A& a, const B& b, const C& c)
        { write(a, b, c); writeln(); }

        template <class A, class B, class C, class D>
        inline void writeln(const A& a, const B& b, const C& c, const D& d)
        { write(a, b, c, d); writeln(); }

        template <class A, class B, class C, class D,
                  class E>
        inline void writeln(const A& a, const B& b, const C& c, const D& d,
                          const E& e)
        { write(a, b, c, d, e); writeln(); }

        template <class A, class B, class C, class D,
                  class E, class F>
        inline void writeln(const A& a, const B& b, const C& c, const D& d,
                          const E& e, const F& f)
        { write(a, b, c, d, e, f); writeln(); }

        template <class A, class B, class C, class D,
                  class E, class F, class G>
        inline void writeln(const A& a, const B& b, const C& c, const D& d,
                          const E& e, const F& f, const G& g)
        { write(a, b, c, d, e, f, g); writeln(); }

        template <class A, class B, class C, class D,
                  class E, class F, class G, class H>
        inline void writeln(const A& a, const B& b, const C& c, const D& d,
                          const E& e, const F& f, const G& g, const H& h)
        { write(a, b, c, d, e, f, g, h); writeln(); }

        inline void read(file f, char &c) { f->get(c); }
        inline void read(std::istream *i, char &x) { i->get(x); }
        inline void read(std::istream *i, int &x) { *i >> x; }
        inline void read(std::istream *i, double &x) { *i >> x; }
        inline void read(std::istream *i, text &x) { *i >> x; }

        namespace encoding
        {
            namespace ascii
            {
                inline char tolower(char c) { return std::tolower(c); }
                inline bool isspace(char c) { return std::isspace(c); }
                inline bool islinebreak(char c) { return c == '\n'; }
                inline bool islinebreak(::text c) { return c == "\n"; }
                inline bool isdigit(char c) { return std::isdigit(c); }
                inline bool ispunctuation(char c) { return std::ispunct(c); }
                inline bool isletter(char c) { return std::isalpha(c); }
                inline bool isletterordigit(char c) { return std::isalnum(c); }
                inline bool isquote(char c) { return c == '"' || c == '\''; }
                inline bool isnul(char c) { return c == 0; }
                const text cr = "\n";
                const text tab = "\t";
            }
        }
    }

    namespace ui
    {
        namespace console
        {
            extern std::vector< ::text > arguments;
        }
    }

#ifdef _MSC_VER
    namespace files
    {
        inline std::vector<std::string> directory(std::string where)
        {
        	  assert(0);
            std::vector<std::string> result;
            return result;
        }
    }
}

#else

    namespace files
    {
        inline std::vector<std::string> directory(std::string where)
        {
            std::vector<std::string> result;
            DIR *dirp = opendir(where.c_str());
            if (dirp)
            {
                while(struct dirent *dp = readdir(dirp))
                {
#if defined(linux) || defined(CONFIG_CYGWIN)
                    int len = strlen(dp->d_name);
#else
                    int len = dp->d_namlen;
#endif // CONFIG_LINUX
                    std::string entry(dp->d_name, len);
                    result.push_back(entry);
                }
            }
            closedir(dirp);
            return result;
        }
    }
}

#endif


template <class T>
::text XLtext(const T& x) {
    std::string result;
    std::ostream *old = xl::textio::standardoutput;
    std::ostringstream out;
    xl::textio::standardoutput = &out;
    xl::textio::write(x);
    result = out.str();
    xl::textio::standardoutput = old;
    return result;
}


// ============================================================================
//
//   Iterators (for loops)
//
// ============================================================================

struct XLIterator
{
    virtual ~XLIterator() {}
    virtual void first() {}
    virtual bool more() { return false; }
    virtual void next() {}
};


template <class T> struct XLRangeIterator : XLIterator
{
    XLRangeIterator(T &v, const std::pair<T,T> &r)
        : value(v), range(r) {}
    virtual void first() { value = range.first; }
    virtual bool more() { return value <= range.second; }
    virtual void next() { value++; }
    T & value;
    std::pair<T,T> range;
};

template<class T>
inline XLRangeIterator<T> *XLMakeIterator(T& what, const std::pair<T,T> &range)
{
    return new XLRangeIterator<T> (what, range);
}


struct XLTextIterator : XLIterator
{
    XLTextIterator (char &c, const text &t)
        : value(c), text_range(t) {}
    virtual void first() { index = 0; max = text_range.length(); }
    virtual bool more() {
        if (index < max) value = text_range[index];
        return index < max;
    }
    virtual void next() { index++; }
    int index, max;
    char &value;
    text text_range;
};

inline XLTextIterator *XLMakeIterator(char& what, const text &range)
{
    return new XLTextIterator(what, range);
}


template <class Value, class Map>
struct XLMapIterator : XLIterator
{
    XLMapIterator (Value &v, Map &m)
        : value(v), range(m) {}
    virtual void first() { it = range.begin(); }
    virtual bool more() {
        bool has_more = it != range.end();
        if (has_more) value = it->first;
        return has_more;
    }
    virtual void next() { it++; }
    Value &value;
    Map &range;
    typename Map::iterator it;
};


template <class Index, class Value>
inline XLMapIterator<Index, std::map<Index, Value> > *
XLMakeIterator(Index& w, std::map<Index, Value> &r)
{
    return new XLMapIterator<Index, std::map<Index, Value> > (w, r);
}


template <class Value, class Vector>
struct XLVectorIterator : XLIterator
{
    XLVectorIterator (Value &v, Vector &m)
        : value(v), range(m) {}
    virtual void first() { it = range.begin(); }
    virtual bool more() {
        bool has_more = it != range.end();
        if (has_more) value = *it;
        return has_more;
    }
    virtual void next() { it++; }
    Value &value;
    Vector &range;
    typename Vector::iterator it;
};


template <class Value>
inline XLVectorIterator<Value, std::vector<Value> > *
XLMakeIterator(Value& w, std::vector<Value> &r)
{
    return new XLVectorIterator<Value, std::vector<Value> > (w, r);
}


inline bool XLDeleteIterator(XLIterator *it) { delete it; return false; }


template<class T>
inline std::pair<T,T> XLMakeRange(const T &first, const T&second)
{
    return std::pair<T,T>(first,second);
}


inline text range (text &from, std::pair<int,int> range)
{
    return text(from, range.first, range.second-range.first+1);
}



// ============================================================================
//
//   Default initialization
//
// ============================================================================

template<class T>struct XLDefaultInit
{
    static T value() { return T(); }
};
template<class T>struct XLDefaultInit<T*>
{
    static T*value(){ return 0; }
};



// ============================================================================
//
//    Pointer automatic dereferencing
//
// ============================================================================

template <class T> inline T& XLDeref(T& x)
{
    return x;
}
template <class T> inline T& XLDeref(T * &x)
{
    if (!x)
        x = new T();
    return *x;
}



// ============================================================================
//
//    Main entry point...
//
// ============================================================================

extern void XLMain();
extern void XLInit();
extern void XLTerm();
extern void XLTraceInit(int Argc, char **Argv);
extern char XLTrace[];

int XLTraceIndent = 0;
void XLTraceName(text trace) {
    std::printf("%-10s:", trace.c_str());
    for (int i = 0; i < XLTraceIndent; i++)
        std::cout << ' ';
}

int main(int argc, char **argv)
{
    for (int arg = 0; arg < argc; arg++)
    {
        if (!(argv[arg][0] == '-' && argv[arg][1] == 't'))
            xl::ui::console::arguments.push_back(argv[arg]);
    }

#ifdef XLINIT
    XLTraceInit(argc, argv);
    XLInit();
#endif
    XLMain();
#ifdef XLINIT
    XLTerm();
#endif
    return 0;
}

