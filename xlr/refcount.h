#ifndef REFCOUNT_H
#define REFCOUNT_H
// ****************************************************************************
//  refcount.h                                                      Tao project
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Jérôme Forissier <jerome@taodyne.com>
//  (C) 2010 Catherine Burvelle <cathy@taodyne.com>
//  (C) 2010 Lionel Schaffhauser <lionel@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

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
