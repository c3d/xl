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



// ============================================================================
// 
//    Basic types
// 
// ============================================================================

typedef std::string text;

template<class T>
inline std::vector<T> &operator += (std::vector<T> & what, const T& last)
{
    what.push_back(last);
    return what;
}



// ============================================================================
// 
//    Text I/O
// 
// ============================================================================

inline void XLWrite(std::ostream *o, char x) { *o << x; }
inline void XLWrite(std::ostream *o, int x) { *o << x; }
inline void XLWrite(std::ostream *o, double x) { *o << x; }
inline void XLWrite(std::ostream *o, text x) { *o << x; }

inline void XLRead(std::istream *i, char &x) { i->get(x); }
inline void XLRead(std::istream *i, int &x) { *i >> x; }
inline void XLRead(std::istream *i, double &x) { *i >> x; }
inline void XLRead(std::istream *i, text &x) { *i >> x; }

namespace xl { namespace textio {
    typedef std::iostream *file;
    file open(text name) { return new std::fstream(name.c_str()); }
    void close(file f) { delete f; }
} }


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
    virtual bool more() { return value < range.second; }
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

template<class T>struct XLDefaultInit { static T value() { return T(); } };
template<class T>struct XLDefaultInit<T*> { static T*value(){return new T();}};



// ============================================================================
// 
//    Pointer automatic dereferencing
// 
// ============================================================================

template <class T> inline T& XLDeref(T& x) { return x; }
template <class T> inline T& XLDeref(T * &x) { return *x; }



// ============================================================================
// 
//    Main entry point...
// 
// ============================================================================

extern void XLMain();
int main(int argc, char **argv) { XLMain(); return 0; }

