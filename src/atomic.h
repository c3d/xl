#ifndef ATOMIC_H
#define ATOMIC_H
// *****************************************************************************
// atomic.h                                                           XL project
// *****************************************************************************
//
// File description:
//
//    The atomic operations we may need in XL
//
//    Note that with C++ 11, all this is largely obsolete,
//    but this allows code to support older compilers, which is still
//    useful in this day and age. Will change it someday.
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010,2015-2017,2019, Christophe de Dinechin <cdedinechin@dxo.com>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
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

XL_END

#endif // ATOMIC_H
