#ifndef REFCOUNT_H
#define REFCOUNT_H
// ****************************************************************************
//  refcount.h                                                    ELIOT project
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

ELIOT_BEGIN

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

ELIOT_END

#endif // REFCOUNT_H
