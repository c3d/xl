#ifndef REFCOUNT_H
#define REFCOUNT_H
// *****************************************************************************
// refcount.h                                                         XL project
// *****************************************************************************
//
// File description:
//
//     Pointers keeping reference count in the target
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
// (C) 2010,2015-2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
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

#include "base.h"

XL_BEGIN

template<class T>
struct ReferenceCountPointer
// ----------------------------------------------------------------------------
//   Behaves like a pointer, but maintains a reference count
// ----------------------------------------------------------------------------
{
    typedef T   data_t;

public:
    ReferenceCountPointer()
        : pointer(0)
    {}
    ReferenceCountPointer(T *pointer)
        : pointer(pointer)
    {
        if (pointer)
            pointer->Acquire();
    }
    ReferenceCountPointer(const ReferenceCountPointer &o)
        : pointer(o.pointer)
    {
        if (pointer)
            pointer->Acquire();
    }
    template<class U>
    ReferenceCountPointer(const ReferenceCountPointer<U> &o)
        : pointer((T *) (const U *) o)
    {
        if (pointer)
            pointer->Acquire();
    }
    ~ReferenceCountPointer()
    {
        if (pointer)
            pointer->Release();
    }
    ReferenceCountPointer &operator=(const ReferenceCountPointer &o)
    {
        if (pointer)
            pointer->Release();
        pointer = o.pointer;
        if (pointer)
            pointer->Acquire();
        return *this;
    }
    const T* operator->() const
    {
        return pointer;
    }
    T* operator->()
    {
        return pointer;
    }
    const T& operator*() const
    {
        return *pointer;
    }
    T& operator*()
    {
        return *pointer;
    }
    operator T*()
    {
        return pointer;
    }
    operator const T*() const
    {
        return pointer;
    }
    template<class U>
    bool operator==(const ReferenceCountPointer<U> &o) const
    {
        return pointer == o.operator const U* ();
    }
    bool operator==(const T *o) const
    {
        return pointer == o;
    }
    template<class U>
    bool operator<(const ReferenceCountPointer<U> &o) const
    {
        return pointer < o.pointer;
    }
    template <class U>
    operator ReferenceCountPointer<U>()
    {
        return ReferenceCountPointer<U> ((U *) pointer);
    }
    bool operator!()
    {
        return !pointer;
    }

protected:
    T *         pointer;
};


template<class T>
struct ReferenceCountReference
// ----------------------------------------------------------------------------
//   Behaves like a reference, but maintains a reference count
// ----------------------------------------------------------------------------
{
    typedef T   data_t;

public:
    ReferenceCountReference(T &target) : target(target)
    {
        target.Acquire();
    }
    ReferenceCountReference(const ReferenceCountReference &o): target(o.target)
    {
        target.Acquire();
    }
    ~ReferenceCountReference()
    {
        target.Release();
    }
    operator const T&() const
    {
        return target;
    }
    operator  T&()
    {
        return target;
    }
    ReferenceCountPointer<T> operator&()
    {
        return ReferenceCountPointer<T>(&target);
    }
    template <class U>
    operator ReferenceCountReference<U>()
    {
        return ReferenceCountReference<U> ((U &) target);
    }

protected:
    T &target;
};

XL_END

#endif // REFCOUNT_H
