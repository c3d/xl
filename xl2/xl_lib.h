// ****************************************************************************
//  xl_lib.h                        (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
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
// ****************************************************************************
// This document is confidential.
// Do not redistribute without written permission
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <map>



// ============================================================================
// 
//    Instructions
// 
// ============================================================================

#define restart() continue


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
inline int size (std::vector<T> & what)
{
    return what.size();
}


template <class T, class U>
inline int count(const std::map<T,U> &m, const T &v)
{
    return m.count(v);
}


template<class T>
inline T & back(std::vector<T> &v)
{
    return v.back();
}


template<class T>
void popback(std::vector<T> &v)
{
    v.pop_back();
}



// ============================================================================
// 
//    Text I/O
// 
// ============================================================================

inline void write(std::ostream *o, char x) { *o << x; }
inline void write(std::ostream *o, int x) { *o << x; }
inline void write(std::ostream *o, double x) { *o << x; }
inline void write(std::ostream *o, text x) { *o << x; }

inline void read(std::istream *i, char &x) { i->get(x); }
inline void read(std::istream *i, int &x) { *i >> x; }
inline void read(std::istream *i, double &x) { *i >> x; }
inline void read(std::istream *i, text &x) { *i >> x; }

namespace xl
{
    namespace textio
    {
        typedef std::iostream *file;
        typedef std::ostream *outputfile;
        typedef std::istream *inputfile;
        file open(text name) { return new std::fstream(name.c_str(), std::ios_base::in); }
        void close(file f) { delete f; }
        void read(file f, char &c) { f->get(c); }
        void putback(file f, char c) { f->putback(c); }
        bool valid(file f) { return f->good(); }

        namespace encoding
        {
            namespace ascii
            {
                inline char tolower(char c) { return std::tolower(c); }
                inline bool isspace(char c) { return std::isspace(c); }
                inline bool islinebreak(char c) { return c == '\n'; }
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
    text text_range;
    char &value;
};

inline XLTextIterator *XLMakeIterator(char& what, const text &range)
{
    return new XLTextIterator(what, range);
}


inline bool XLDeleteIterator(XLIterator *it) { delete it; return false; }


template<class T>
inline std::pair<T,T> XLMakeRange(const T &first, const T&second)
{
    return std::pair<T,T>(first,second);
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
int main(int argc, char **argv)
{
    for (int arg = 0; arg < argc; arg++)
        xl::ui::console::arguments.push_back(argv[arg]);
    XLMain();
    return 0;
}

