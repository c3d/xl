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
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010,2019, Christophe de Dinechin <christophe@dinechin.org>
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

struct OrderedAtomicMode;
struct AcquireAtomicMode;       // REVISIT: Not implemented yet
struct ReleaseAtomicMode;       // REVISIT: Not implemented yet


template <typename Value, typename Mode = OrderedAtomicMode>
struct Atomic
// ----------------------------------------------------------------------------
//   A value of type T with atomic properties
// ----------------------------------------------------------------------------
{
    typedef Value               value_t;
    typedef int                 diff_t;

public:
    Atomic()                    : value() {}
    Atomic(value_t v)           : value(v) {}
    Atomic(const Atomic &o)     : value(o.Get()) {}

    operator value_t() cons     { return value; }
    operator value_t&()         { return value; }

    bool  Get()                 { return value; }
    bool  Set(value_t from, value_t to);
    value_t Swap(value_t swap);
    value_t Add(diff_t delta);

    // Increment and decrement can be specialized for processors that
    // have special instructions with limited constant range (e.g. Itanium)
    template <int delta = 1>
    value_t Increment()         { return Add(delta); }
    template <int delta = 1>
    value_t Decrement()         { return Add(-delta); }

protected:
    volatile value_t            value;
};



#if defined(__x86_64__)
// ============================================================================
//
//   Implementation for x86-64
//
// ============================================================================

template<> inline
bool Atomic<int,OrderedAtomicMode>::Set(int from, int to)
// ----------------------------------------------------------------------------
//   Test that we have 'from' and set to 'to'
// ----------------------------------------------------------------------------
{
    unsigned char ret;
    asm volatile("lock\n"
                 "cmpxchgl %3,%2\n"
                 "sete %1\n"
                 : "=a" (to), "=qm" (ret), "+m" (value)
                 : "r" (to), "0" (from)
                 : "memory");
    return ret != 0;
}


template<> inline
int Atomic<int,OrderedAtomicMode>::Swap(int swap)
// ----------------------------------------------------------------------------
//   Atomically swap the value and return what was there before
// ----------------------------------------------------------------------------
{
    asm volatile("xchgl %0,%1"
                 : "=r" (swap), "+m" (value)
                 : "0" (swap)
                 : "memory");
    return swap;
}


template<> inline
int Atomic<int,OrderedAtomicMode>::Add(int delta)
// ----------------------------------------------------------------------------
//   Atomically increment the given value
// ----------------------------------------------------------------------------
{
    asm volatile("lock\n"
                 "xaddl %0,%1"
                 : "=r" (delta), "+m" (value)
                 : "0" (delta)
                 : "memory");
    return delta;
}


template<typename V> inline
bool Atomic<V *,OrderedAtomicMode>::Set(V *from, V *to)
// ----------------------------------------------------------------------------
//   Test that we have 'from' and set to 'to'
// ----------------------------------------------------------------------------
{
    unsigned char ret;
    asm volatile("lock\n"
                 "cmpxchgq %3,%2\n"
                 "sete %1\n"
                 : "=a" (to), "=qm" (ret), "+m" (value)
                 : "r" (to), "0" (from)
                 : "memory");
    return ret != 0;
}


template<typename V> inline
V Atomic<V *,OrderedAtomicMode>::Swap(V *swap)
// ----------------------------------------------------------------------------
//   Atomically swap the value and return what was there before
// ----------------------------------------------------------------------------
{
    asm volatile("xchgq %0,%1"
                 : "=r" (swap), "+m" (value)
                 : "0" (swap)
                 : "memory");
    return swap;
}


template<typename V> inline
V *Atomic<V*, OrderedAtomicMode>::Add(int delta)
// ----------------------------------------------------------------------------
//   Atomically increment the given value for pointers
// ----------------------------------------------------------------------------
{
    asm volatile("lock\n"
                 "xaddq %0,%1"
                 : "=r" (delta), "+m" (value)
                 : "0" (delta * sizeof(V))
                 : "memory");
    return reinterpret_cast<V> (delta);
}


template<> inline
int Atomic<int,OrderedAtomicMode>::Increment<1>()
// ----------------------------------------------------------------------------
//   If we increment by 1, use increment instead of add
// ----------------------------------------------------------------------------
{
    unsigned char ret;
    asm volatile("lock\n"
                 "incl %0"
                 : "=m" (value)
                 : "m" (value)
                 : "memory");
    return value;
}


template<> inline
int Atomic<int,OrderedAtomicMode>::Decrement<1>()
// ----------------------------------------------------------------------------
//   If we decrement by 1, use increment instead of add
// ----------------------------------------------------------------------------
{
    unsigned char ret;
    asm volatile("lock\n"
                 "decl %0"
                 : "=m" (value)
                 : "m" (value)
                 : "memory");
    return value;
    
}


#elif defined(__i386__)
// ============================================================================
// 
//   Implementation for "regular" x86
// 
// ============================================================================

#error "Not yet implemented"

#elif defined(__ppc__)
// ============================================================================
// 
//   Implementation for PowerPC
// 
// ============================================================================

#error "Not yet implemented"

#else
// ============================================================================
// 
//   Default implementation (pure C)
// 
// ============================================================================

#include <pthread.h>

#error "Not yet implemented"

#endif

XL_END

#endif // ATOMIC_H
