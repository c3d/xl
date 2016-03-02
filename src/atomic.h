#ifndef ATOMIC_H
#define ATOMIC_H
// ****************************************************************************
//  atomic.h                                                     ELFE project
// ****************************************************************************
//
//   File Description:
//
//    The atomic operations we may need in ELFE
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

ELFE_BEGIN

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
    typedef volatile Value &                    value_v;

public:
    Atomic()                                    : value() {}
    Atomic(value_t v)                           : value(v) {}
    Atomic(const Atomic &o)                     : value(o.Get()) {}

    operator            value_t() const         { return value; }
    operator            volatile value_t&()     { return value; }

    value_t             Get() const             { return value; }


    static value_t      Set (value_v ref, value_t from, value_t to);
    static bool         SetQ(value_v ref, value_t from, value_t to);
    static value_t      Add (value_v ref, value_t delta);
    static value_t      Sub (value_v ref, value_t delta);
    static value_t      Or  (value_v ref, value_t delta);
    static value_t      Xor (value_v ref, value_t delta);
    static value_t      And (value_v ref, value_t delta);
    static value_t      Nand(value_v ref, value_t delta);

    value_t             Set(value_t f,value_t t){ return Set (value, f, t); }
    bool                SetQ(value_t f,value_t t){ return SetQ(value, f, t); }
    value_t             Add(value_t delta)      { return Add (value, delta); }
    value_t             Sub(value_t delta)      { return Sub (value, delta); }
    value_t             Or (value_t delta)      { return Or  (value, delta); }
    value_t             Xor(value_t delta)      { return Xor (value, delta); }
    value_t             And(value_t delta)      { return And (value, delta); }
    value_t             Nand(value_t delta)     { return Nand(value, delta); }

    value_t             operator =(value_t d)   { value = d; return value; }

    value_t             operator++()            { return Add(1) + 1; }
    value_t             operator--()            { return Sub(1) - 1; }

    value_t             operator++(int)         { return Add(1); }
    value_t             operator--(int)         { return Sub(1); }

    // WARNING: The following operators differ from the C equivalents
    // in that they return the value BEFORE operation. That's on purpose,
    // as in general, that's the value which is useful for testing
    value_t             operator +=(value_t d)  { return Add(d); }
    value_t             operator -=(value_t d)  { return Sub(d); }
    value_t             operator |=(value_t d)  { return Or(d); }
    value_t             operator &=(value_t d)  { return And(d); }
    value_t             operator ^=(value_t d)  { return Xor(d); }

    value_t             Minimize(value_t newValue)
    {
        value_t old = value;
        while (old > newValue && !SetQ(old, newValue))
            old = value;
        return old;
    }
    value_t             Maximize(value_t newValue)
    {
        value_t old = value;
        while (old < newValue && !SetQ(old, newValue))
            old = value;
        return old;
    }

protected:
    volatile value_t    value;
};


template <class Link>
inline void LinkedListInsert(Atomic<Link> &list, Link link)
// ----------------------------------------------------------------------------
//    Insert an element in a linked list
// ----------------------------------------------------------------------------
{
    Link next;
    do
    {
        next = list;
        link->next = next;
    } while (!list.SetQ(next, link));
}


template <class Link>
inline Link LinkedListPopFront(Atomic<Link> &list)
// ----------------------------------------------------------------------------
//    Remove the head of a linked list
// ----------------------------------------------------------------------------
{
    Link head = list;
    while (!list.SetQ(head, head->next))
        head = list;
    return head;
}


#ifdef __GNUC__
// ============================================================================
//
//   Implementation using GCC builtins
//
// ============================================================================

template<typename Value> inline
Value Atomic<Value>::Set(value_v ref, value_t from, value_t to)
// ----------------------------------------------------------------------------
//   Test that we have 'from' and set to 'to', return value before write
// ----------------------------------------------------------------------------
{
    return __sync_val_compare_and_swap(&ref, from, to);
}


template<typename Value> inline
bool Atomic<Value>::SetQ(value_v ref, value_t from, value_t to)
// ----------------------------------------------------------------------------
//   Test that we have 'from' and set to 'to'
// ----------------------------------------------------------------------------
{
    return __sync_bool_compare_and_swap(&ref, from, to);
}


template<typename Value> inline
Value Atomic<Value>::Add(value_v ref, value_t delta)
// ----------------------------------------------------------------------------
//   Atomically add the value, return the value before update
// ----------------------------------------------------------------------------
{
    return __sync_fetch_and_add(&ref, delta);
}


template<typename Value> inline
Value Atomic<Value>::Sub(value_v ref, value_t delta)
// ----------------------------------------------------------------------------
//   Atomically subtract the value, return the value before update
// ----------------------------------------------------------------------------
{
    return __sync_fetch_and_sub(&ref, delta);
}

template<typename Value> inline
Value Atomic<Value>::Or(value_v ref, value_t delta)
// ----------------------------------------------------------------------------
//   Atomically or the value, return the value before update
// ----------------------------------------------------------------------------
{
    return __sync_fetch_and_or(&ref, delta);
}

template<typename Value> inline
Value Atomic<Value>::Xor(value_v ref, value_t delta)
// ----------------------------------------------------------------------------
//   Atomically xor the value, return the value before update
// ----------------------------------------------------------------------------
{
    return __sync_fetch_and_xor(&ref, delta);
}

template<typename Value> inline
Value Atomic<Value>::And(value_v ref, value_t delta)
// ----------------------------------------------------------------------------
//   Atomically and the value, return the value before update
// ----------------------------------------------------------------------------
{
    return __sync_fetch_and_and(&ref, delta);
}


template<typename Value> inline
Value Atomic<Value>::Nand(value_v ref, value_t delta)
// ----------------------------------------------------------------------------
//   Atomically and the value, return the value before update
// ----------------------------------------------------------------------------
{
    return __sync_fetch_and_and(&ref, ~delta);
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

ELFE_END

#endif // ATOMIC_H
