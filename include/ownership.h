#ifndef OWNERSHIP_H
#define OWNERSHIP_H
// *****************************************************************************
// ownership.h                                                        XL project
// *****************************************************************************
//
// File description:
//
//     Classes handling ownership the way XL does
//
//     This is loosely inspired by Rust's ownership model
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2019, Christophe de Dinechin <christophe@dinechin.org>
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

namespace XL
{
template<typename T>    class var;
template<typename T>    class ref;
template<typename T>    class own;
template<typename T>    class use;
template<typename T>    class any;
template<typename T>    class in;
template<typename T>    class out;
template<typename T>    class inout;


// ============================================================================
//
//    Input arguments
//
// ============================================================================

template<typename T, bool CanCopy>
struct in_
// ----------------------------------------------------------------------------
//   Helper struct for the best way to copy an input argument
// ----------------------------------------------------------------------------
{
    typedef const T&  in_type;
};


template<typename T>
struct in_<T, true>
// ----------------------------------------------------------------------------
//   Partial specialization for easy-to-copy items
// ----------------------------------------------------------------------------
{
    typedef const T   in_type;
};


template<typename T>
class in
// ----------------------------------------------------------------------------
//    A class used to copy arguments efficiently around functions
// ----------------------------------------------------------------------------
{
    using in_type = typename in_<T, sizeof(T) <= 2 * sizeof(void *)>::in_type;
    in_type     value;
public:
    in(in_type value): value(value)             {}
    ~in()                                       {}
    operator in_type()                          { return value; }
};



// ============================================================================
//
//   In/out arguments
//
// ============================================================================

template<typename T, bool CanCopy>
class inout_
// ----------------------------------------------------------------------------
//   A class used to copy in/out arguments efficiently
// ----------------------------------------------------------------------------
{
    T &         ref;
public:
    inout_(T &ref): ref(ref)                    {}
    ~inout_()                                   {}
    operator T &()                              { return ref; }
    template<typename U>
    inout_ &operator=(const U &val)             { ref = val; return *this; }
};


template<typename T>
class inout_<T, true>
// ----------------------------------------------------------------------------
//   Partial specialization when copy is cheap
// ----------------------------------------------------------------------------
{
    T &         ref;
    T           value;
public:
    inout_(T &ref): ref(ref), value(ref)        {}
    ~inout_()                                   { ref = value; }
    operator T&()                               { return value; }
    template<typename U>
    inout_ &operator=(const U &val)             { value = val; return *this; }
};


template<typename T>
class inout
// ----------------------------------------------------------------------------
//    A class used to copy arguments efficiently around functions
// ----------------------------------------------------------------------------
{
    using inout_type = inout_<T, sizeof(T) <= 2 * sizeof(void *)>;
    inout_type  value;
public:
    inout(T& ref): value(ref)                   {}
    ~inout()                                    {}
    operator T&()                               { return value; }
    template<class U>
    inout &operator=(const U &val)              { value = val; return *this; }
};



// ============================================================================
//
//   Output arguments
//
// ============================================================================

template<typename T>
class out
// ----------------------------------------------------------------------------
//    A class used to copy arguments efficiently around functions
// ----------------------------------------------------------------------------
{
    using out_type = inout_<T, sizeof(T) <= 2 * sizeof(void *)>;
    out_type  value;
public:
    out(T& ref): value(ref)                     {}
    ~out()                                      {}
    operator T&()                               { return value; }
    template<class U>
    out &operator=(const U &val)                { value = val; return *this; }
};



// ============================================================================
//
//    Variable values
//
// ============================================================================

template<typename T>
class var
// ----------------------------------------------------------------------------
//   A variable value of type T
// ----------------------------------------------------------------------------
{
    T           value;

public:
    var(const T& value): value(value)           {}
    ~var()                                      {}
    operator T&()                               { return value; }
    template <typename U>
    var &operator=(const U &val)                { value = val; return *this; }
};




// ============================================================================
//
//   An owned reference
//
// ============================================================================

template<typename T>
class own
// ----------------------------------------------------------------------------
//    A reference owning a value
// ----------------------------------------------------------------------------
{
    T *         value;

    T *Move()
    {
        refcheck();
        T *moving = value;
        value = nullptr;
        return moving;
    }
    void Recapture(own &other)
    {
        assert(!value);
        refcheck();
        value = other.Move();
    }

public:
    own() : value(nullptr), refcount()          {}
    own(own<T> &other)
        : value(other.Move()), refcount()       {}
    template<typename ... Args>
    own(Args ... args)
        : value(new T(args...)),
          refcount()                            {}
    ~own()                                      { refcheck(); delete value; }
    operator T&()                               { return *value; }
    own &operator=(own &val)
    {
        refcheck();
        delete value;
        value = val.Move();
        return *this;
    }
    template <typename U>
    own &operator=(const U &val)                { *value = val; return *this; }

private:
    friend class ref<own<T>>;
#ifdef NO_REFCOUNT
    void ref()                                  {}
    void unref()                                {}
    void refcheck()                             {}
    struct {} refcount;
#else // NO_REFCOUNT
    void ref()                                  { refcount++; }
    void unref()                                { refcount--; }
    void refcheck()                             { assert(refcount == 0); }
    unsigned refcount;
#endif // NO_REFCOUNT
};



// ============================================================================
//
//    Read-only reference to another value
//
// ============================================================================

template<typename T>
class ref
// ----------------------------------------------------------------------------
//   A non-owning reference
// ----------------------------------------------------------------------------
{
    const T& value;
public:
    ref(const T& value): value(value)           {}
    ~ref()                                      {}
    operator const T&()                         { return value; }
};


template<typename T>
class ref<own<T>>
// ----------------------------------------------------------------------------
//   When referencing an owned reference, refcount it
// ----------------------------------------------------------------------------
{
    const T& value;
public:
    ref(const own<T> &value): value(value)      { value.ref(); }
    ~ref()                                      { value.unref(); }
    operator const T&()                         { return value; }
};



// ============================================================================
//
//    Mutable refernece to another value
//
// ============================================================================

template <typename T>
class use
// ----------------------------------------------------------------------------
//   A mutable borrowed reference
// ----------------------------------------------------------------------------
{
    own<T> &owner;
    own<T> value;
public:
    use(own<T> &value)
        : owner(value), value(value)            {}
    ~use()                                      { owner.Recapture(value); }
    operator T&()                               { return value; }
    template<typename U>
    use &operator=(const U &val)                { value = val; return *this; }
};



// ============================================================================
//
//    Polymorphic type
//
// ============================================================================

template<typename T>
class any
// ----------------------------------------------------------------------------
//   A polymorphic reference
// ----------------------------------------------------------------------------
{
    own<T> value;
public:
    template <typename ... Args>
    any(Args ... args): value(args...)          {}
    ~any()                                      {}
    operator T&()                               { return value; }
    template <typename U>
    any &operator=(const U &val)                { value = val; return *this; }
};

} // namespace XL

#endif // OWNERSHIP_H
