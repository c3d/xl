#ifndef ATOMIC_H
#define ATOMIC_H
// ****************************************************************************
//  atomic.h                                                        XLR project
// ****************************************************************************
//
//   File Description:
//
//    The atomic operations we may need in XL
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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "base.h"

XL_BEGIN

// ============================================================================
//
//   Interface
//
// ============================================================================

template <typename Value>
struct Atomic
// ----------------------------------------------------------------------------
//   A value of type T with atomic properties
// ----------------------------------------------------------------------------
{
    typedef Value                               value_t;

public:
    Atomic()                                    : value() {}
    Atomic(value_t v)                           : value(v) {}
    Atomic(const Atomic &o)                     : value(o.Get()) {}

    operator            value_t() const         { return value; }
    operator            value_t&()              { return value; }

    bool                Get()                   { return value; }

    value_t             Set(value_t from, value_t to);
    bool                SetQ(value_t from, value_t to);
    value_t             Add(value_t delta);
    value_t             Sub(value_t delta);

    // Increment and decrement can be specialized for processors that
    // have special instructions with limited constant range (e.g. Itanium)
    template <int d>
    value_t             Increment()             { return Add(d); }
    template <int d>
    value_t             Decrement()             { return Sub(d); }

    Atomic<Value> &     operator+=(value_t d)   { Add(d); return *this; }
    Atomic<Value> &     operator-=(value_t d)   { Sub(d); return *this; }

    value_t             operator++()            { return Increment<1>(); }
    value_t             operator--()            { return Decrement<1>(); }

    value_t             operator++(int)         { return Increment<1>() - 1; }
    value_t             operator--(int)         { return Decrement<1>() + 1; }

protected:
    volatile value_t            value;
};



#ifdef __GNUC__
// ============================================================================
//
//   Implementation using GCC builtins
//
// ============================================================================

template<typename Value> inline
Value Atomic<Value>::Set(Value from, Value to)
// ----------------------------------------------------------------------------
//   Test that we have 'from' and set to 'to', return value before write
// ----------------------------------------------------------------------------
{
    return __sync_val_compare_and_swap(&value, from, to);
}


template<typename Value> inline
bool Atomic<Value>::SetQ(Value from, Value to)
// ----------------------------------------------------------------------------
//   Test that we have 'from' and set to 'to'
// ----------------------------------------------------------------------------
{
    return __sync_bool_compare_and_swap(&value, from, to);
}


template<typename Value> inline
Value Atomic<Value>::Add(value_t delta)
// ----------------------------------------------------------------------------
//   Atomically add the value, return the the value after update
// ----------------------------------------------------------------------------
{
    return __sync_add_and_fetch(&value, delta);
}


template<typename Value> inline
Value Atomic<Value>::Sub(value_t delta)
// ----------------------------------------------------------------------------
//   Atomically subtract the value, return the the value after update
// ----------------------------------------------------------------------------
{
    return __sync_sub_and_fetch(&value, delta);
}

#else
// ============================================================================
// 
//   Default implementation (pure C)
// 
// ============================================================================

#include <pthread.h>

#error "Not yet implemented if GCC primitives are not available"

#endif

XL_END

#endif // ATOMIC_H
