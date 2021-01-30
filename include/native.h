#ifndef NATIVE_H
#define NATIVE_H
// *****************************************************************************
// native.h                                                           XL project
// *****************************************************************************
//
// File description:
//
//     Native interface between XL and native code
//
//     The new interface requires C++11 features, notably variadic templates
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2019-2020, Christophe de Dinechin <christophe@dinechin.org>
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

#include "basics.h"
#include "compiler.h"
#include "llvm-crap.h"
#include "tree.h"

#include <recorder/recorder.h>
#include <type_traits>

RECORDER_DECLARE(native);

namespace XL
{
// ============================================================================
//
//    xl_type: Convert a native type into its XL counterpart
//
// ============================================================================

template<typename T, typename Enable = void>
struct xl_type
// ----------------------------------------------------------------------------
//   Provide a common interface to convert an XL type into its
// ----------------------------------------------------------------------------
{
    typedef Tree *            tree_type;
    typedef T                 native_type;
    static JIT::PointerType_p TreeType(Compiler &c)   { return c.treePtrTy; }
    static JIT::PointerType_p NativeType(Compiler &c) { return c.treePtrTy; }
    static Tree *             Shape()                 { return XL::tree_type; }
};


template <typename Num>
struct xl_type<Num,
               typename std::enable_if<std::is_integral<Num>::value>::type>
// ----------------------------------------------------------------------------
//   Specialization for natural types
// ----------------------------------------------------------------------------
{
    typedef Natural *    tree_type;
    typedef Num          native_type;

    static JIT::PointerType_p TreeType(Compiler &c)
    {
        return c.naturalTreePtrTy;
    }

    static JIT::Type_p NativeType(Compiler &c)
    {
        return c.jit.IntegerType<Num>();
    }
    static Tree *Shape()
    {
        return natural_type;
    }
};


template <> inline Tree *xl_type<bool>  ::Shape() { return boolean_type; }
template <> inline Tree *xl_type<int8>  ::Shape() { return integer8_type; }
template <> inline Tree *xl_type<int16> ::Shape() { return integer16_type; }
template <> inline Tree *xl_type<int32> ::Shape() { return integer32_type; }
template <> inline Tree *xl_type<int64> ::Shape() { return integer64_type; }
template <> inline Tree *xl_type<uint8> ::Shape() { return natural8_type; }
template <> inline Tree *xl_type<uint16>::Shape() { return natural16_type; }
template <> inline Tree *xl_type<uint32>::Shape() { return natural32_type; }
template <> inline Tree *xl_type<uint64>::Shape() { return natural64_type; }

template <typename Num>
struct xl_type<Num,
          typename std::enable_if<std::is_floating_point<Num>::value>::type>
// ----------------------------------------------------------------------------
//   Specialization for floating-point types
// ----------------------------------------------------------------------------
{
    typedef Real *       tree_type;
    typedef Num          native_type;

    static JIT::PointerType_p TreeType(Compiler &c)
    {
        return c.realTreePtrTy;
    }

    static JIT::Type_p NativeType(Compiler &c)
    {
        return c.jit.FloatType(c.jit.BitsPerByte * sizeof(Num));
    }

    static Tree *Shape()
    {
        return real_type;
    }
};


template <> inline Tree *xl_type<float>  ::Shape()  { return real32_type; }
template <> inline Tree *xl_type<double> ::Shape()  { return real64_type; }

template <>
struct xl_type<kstring>
// ----------------------------------------------------------------------------
//   Specialization for C-style C strings
// ----------------------------------------------------------------------------
{
    typedef Text *       tree_type;
    typedef kstring      native_type;

    static JIT::PointerType_p TreeType(Compiler &c)
    {
        return c.textTreePtrTy;
    }

    static JIT::Type_p NativeType(Compiler &c)
    {
        return c.charPtrTy;
    }

    static Tree *Shape()
    {
        return text_type;
    }
};


template <>
struct xl_type<text>
// ----------------------------------------------------------------------------
//   Specialization for C++-style C strings
// ----------------------------------------------------------------------------
{
    typedef Text *       tree_type;
    typedef text         native_type;

    static JIT::PointerType_p TreeType(Compiler &c)
    {
        return c.textTreePtrTy;
    }

    static JIT::Type_p NativeType(Compiler &c)
    {
        return c.textPtrTy;
    }

    static Tree *Shape()
    {
        return text_type;
    }
};


template <>
struct xl_type<Scope *>
// ----------------------------------------------------------------------------
//   Specialization for C++-style C strings
// ----------------------------------------------------------------------------
{
    typedef Tree *       tree_type;
    typedef Scope *      native_type;

    static JIT::PointerType_p TreeType(Compiler &c)
    {
        return c.scopePtrTy;
    }

    static JIT::Type_p NativeType(Compiler &c)
    {
        return c.scopePtrTy;
    }

    static Tree *Shape()
    {
        return scope_type;
    }
};



// ============================================================================
//
//    Extracting information about a function type
//
// ============================================================================

template <typename F> struct function_type;

template <typename R>
struct function_type<R(*)()>
// ----------------------------------------------------------------------------
//   Special case for functions without arguments
// ----------------------------------------------------------------------------
{
    typedef R return_type;
    static void         Args(Compiler &, JIT::Signature &)  {}
    static Tree *       Shape()
    {
        record(native, "Shape returns null");
        return nullptr;
    }
    static Tree_p       ReturnShape()
    {
        Tree_p ret = xl_type<R>::Shape();
        record(native, "ReturnShape () = %t", ret);
        return ret;
    }
};


template <typename R, typename T>
struct function_type<R(*)(T)>
// ----------------------------------------------------------------------------
//   Special case for a function with a single argument
// ----------------------------------------------------------------------------
{
    typedef R return_type;
    static void Args(Compiler &compiler, JIT::Signature &signature)
    {
        JIT::Type_p argTy = xl_type<T>::NativeType(compiler);
        signature.push_back(argTy);
    }
    static Tree_p Shape(uint &index)
    {
        Tree_p type = xl_type<T>::Shape();
        Name_p name = new Name(text(1, 'A' + index), Tree::BUILTIN);
        ++index;
        if (type)
        {
            Infix_p infix = new Infix(":", name, type);
            record(native,
                   "Shape %u infix %t : %t = %t", index, type, name, infix);
            return infix;
        }
        record(native, "Shape %u name %t", index, name);
        return name;
    }
    static Tree_p ReturnShape()
    {
        Tree_p ret = xl_type<R>::Shape();
        record(native, "Return shape (one) %t", ret);
        return ret;
    }
};


template <typename R, typename T, typename ... A>
struct function_type<R(*)(T,A...)>
// ----------------------------------------------------------------------------
//   Recursive for the rest
// ----------------------------------------------------------------------------
{
    typedef R return_type;
    static void Args(Compiler &compiler, JIT::Signature &signature)
    {
        function_type<R(*)(T)>::Args(compiler, signature);
        function_type<R(*)(A...)>::Args(compiler, signature);
    }
    static Tree_p Shape(uint &index)
    {
        Tree_p left = function_type<R(*)(T)>::Shape(index);
        Tree_p right = function_type<R(*)(A...)>::Shape(index);
        Infix_p infix = new Infix(",", left, right);
        record(native, "Shape %u (...) %t,%t = %t", index, left, right, infix);
        return infix;
    }
    static Tree_p ReturnShape()
    {
        Tree_p ret = xl_type<R>::Shape();
        record(native, "Return shape (...) %t", ret);
        return ret;
    }
};



// ============================================================================
//
//    JIT interface for function types
//
// ============================================================================

struct NativeInterface
// ----------------------------------------------------------------------------
//   Base functions to generate JIT code for a native function
// ----------------------------------------------------------------------------
{
    virtual     ~NativeInterface() {}
    virtual     JIT::Type_p          ReturnType(Compiler &)              = 0;
    virtual     JIT::FunctionType_p  FunctionType(Compiler &)            = 0;
    virtual     JIT::Function_p      Prototype(Compiler &, text name)    = 0;
    virtual     Tree_p               Shape(uint &index)                  = 0;
};


template<typename fntype>
struct NativeImplementation : NativeInterface
// ----------------------------------------------------------------------------
//   Generate the interface
// ----------------------------------------------------------------------------
{
    virtual JIT::Type_p ReturnType(Compiler &compiler) override
    {
        typedef typename function_type<fntype>::return_type return_type;
        xl_type<return_type> xlt;
        return xlt.NativeType(compiler);
    }

    virtual JIT::FunctionType_p FunctionType(Compiler &compiler) override
    {
        JIT::Type_p rty = ReturnType(compiler);

        function_type<fntype> ft;
        JIT::Signature sig;
        ft.Args(compiler, sig);
        return compiler.jit.FunctionType(rty, sig);
    }

    virtual JIT::Function_p Prototype(Compiler &compiler, text name) override
    {
        JIT::FunctionType_p fty = FunctionType(compiler);
        JIT::Function_p f = compiler.jit.ExternFunction(fty, name);
        return f;
    }

    virtual Tree_p Shape(uint &index) override
    {
        function_type<fntype> ft;
        Tree_p result = ft.Shape(index);
        Tree_p retType = ft.ReturnShape();
        if (retType)
            result = new Infix("as", result, retType);
        record(native, "Native shape %u %t return %t", index, result, retType);
        return result;
    }
};



// ============================================================================
//
//    Native interface builder
//
// ============================================================================

struct Native
// ----------------------------------------------------------------------------
//   Native interface for a function with the given signature
// ----------------------------------------------------------------------------
{
    template<typename fntype>
    Native(kstring symbol, fntype input)
        : symbol(symbol),
          address((void *) input),
          implementation(new NativeImplementation<fntype>),
          shape(nullptr),
          next(list)
    {
        list = this;
    }

    ~Native();

    static Native *     First()                 { return list; }
    Native *            Next()                  { return next; }

    JIT::Type_p         ReturnType(Compiler &compiler);
    JIT::FunctionType_p FunctionType(Compiler &compiler);
    JIT::Function_p     Prototype(Compiler &compiler, text name);

    Tree_p              Shape();

    static void         EnterPrototypes(Compiler &compiler);

public:
    kstring             symbol;
    void *              address;
    NativeInterface *   implementation;
    Tree_p              shape;

private:
    static Native      *list;
    Native             *next;
};


inline JIT::Type_p Native::ReturnType(Compiler &compiler)
// ----------------------------------------------------------------------------
//   Delegate the return type computation to the implementation
// ----------------------------------------------------------------------------
{
    return implementation->ReturnType(compiler);
}


inline JIT::FunctionType_p Native::FunctionType(Compiler &compiler)
// ----------------------------------------------------------------------------
//   Delegate the function type computation to the implementation
// ----------------------------------------------------------------------------
{
    return implementation->FunctionType(compiler);
}


inline JIT::Function_p Native::Prototype(Compiler &compiler, text name)
// ----------------------------------------------------------------------------
//   Delegate the prototype generation to the implementation
// ----------------------------------------------------------------------------
{
    return implementation->Prototype(compiler, name);
}


inline Tree_p Native::Shape()
// ----------------------------------------------------------------------------
//   Delegate the shape generation to the implementation
// ----------------------------------------------------------------------------
{
    if (!shape)
    {
        uint index = 0;
        Name_p name = new Name(symbol, Tree::BUILTIN);
        if (Tree_p args = implementation->Shape(index))
        {
            Prefix_p prefix = new Prefix(name, args, Tree::BUILTIN);
            shape = prefix;
        }
        else
        {
            shape = name;
        }
    }
    return shape;
}

} // namespace XL

#define NATIVE(Name)    static Native xl_##Name##_native(#Name, Name)
#define XL_NATIVE(Name) static Native xl_##Name##_native(#Name, xl_##Name)

#endif // NATIVE_H
