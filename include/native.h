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
// (C) 2019-2021, Christophe de Dinechin <christophe@dinechin.org>
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

#include "tree.h"
#include "builtins.h"
#include "interpreter.h"
#include "errors.h"

#ifndef INTERPRETER_ONLY
#include "compiler.h"
#include "llvm-crap.h"
#endif

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
    typedef Tree BoxType;
    typedef T    native_type;

#ifndef INTERPRETER_ONLY
    static JIT::PointerType_p TreeType(Compiler &c)   { return c.treePtrTy; }
    static JIT::PointerType_p NativeType(Compiler &c) { return c.treePtrTy; }
#endif
    static Tree *Shape()
    {
        return XL::tree_type;
    }
    static BoxType *Box(native_type x, TreePosition pos)
    {
        return x;
    }
    static native_type Unbox(BoxType *x)
    {
        return x;
    }
};


template<>
struct xl_type<void>
// ----------------------------------------------------------------------------
//   Provide a common interface to convert an XL type into its
// ----------------------------------------------------------------------------
{
    typedef Tree BoxType;
    typedef void native_type;

#ifndef INTERPRETER_ONLY
    static JIT::PointerType_p TreeType(Compiler &c)   { return c.treePtrTy; }
    static JIT::Type_p        NativeType(Compiler &c) { return c.voidTy; }
#endif
    static Tree *Shape()
    {
        return XL::tree_type;
    }
    static native_type Unbox(BoxType *x)
    {
        return;
    }
};


template <typename Num>
struct xl_type<Num,
               typename std::enable_if<std::is_integral<Num>::value>::type>
// ----------------------------------------------------------------------------
//   Specialization for natural types
// ----------------------------------------------------------------------------
{
    typedef Natural BoxType;
    typedef Num     native_type;

#ifndef INTERPRETER_ONLY
    static JIT::PointerType_p TreeType(Compiler &c)
    {
        return c.naturalTreePtrTy;
    }

    static JIT::Type_p NativeType(Compiler &c)
    {
        return c.jit.IntegerType<Num>();
    }
#endif

    static Tree *Shape()
    {
        return natural_type;
    }
    static BoxType *Box(native_type x, TreePosition pos)
    {
        BoxType *result = new BoxType(x, pos);
        if (std::numeric_limits<Num>::is_signed)
            result = result->MakeSigned();
        return result;
    }
    static native_type Unbox(BoxType *x)
    {
        return x->value;
    }
    static native_type Unbox(Tree *x)
    {
        if (BoxType *check = x->As<BoxType>())
            return Unbox(check);
        Ooops("Expected a natural value, got $1", x);
        return 0;
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
    typedef Real BoxType;
    typedef Num  native_type;

#ifndef INTERPRETER_ONLY
    static JIT::PointerType_p TreeType(Compiler &c)
    {
        return c.realTreePtrTy;
    }

    static JIT::Type_p NativeType(Compiler &c)
    {
        return c.jit.FloatType(c.jit.BitsPerByte * sizeof(Num));
    }
#endif

    static Tree *Shape()
    {
        return real_type;
    }
    static BoxType *Box(native_type x, TreePosition pos)
    {
        return new BoxType(x, pos);
    }
    static native_type Unbox(BoxType *x)
    {
        return x->value;
    }
    static native_type Unbox(Tree *x)
    {
        if (BoxType *check = x->As<BoxType>())
            return Unbox(check);
        Ooops("Expected a real value, got $1", x);
        return 0;
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
    typedef Text         BoxType;
    typedef kstring      native_type;

#ifndef INTERPRETER_ONLY
    static JIT::PointerType_p TreeType(Compiler &c)
    {
        return c.textTreePtrTy;
    }

    static JIT::Type_p NativeType(Compiler &c)
    {
        return c.charPtrTy;
    }
#endif

    static Tree *Shape()
    {
        return text_type;
    }
    static BoxType *Box(native_type x, TreePosition pos)
    {
        return new BoxType(x, pos);
    }
    static native_type Unbox(BoxType *x)
    {
        return x->value.c_str();
    }
    static native_type Unbox(Tree *x)
    {
        if (BoxType *check = x->As<BoxType>())
            return Unbox(check);
        Ooops("Expected a text value, got $1", x);
        return 0;
    }
};


template <>
struct xl_type<text>
// ----------------------------------------------------------------------------
//   Specialization for C++-style C strings
// ----------------------------------------------------------------------------
{
    typedef Text         BoxType;
    typedef text         native_type;

#ifndef INTERPRETER_ONLY
    static JIT::PointerType_p TreeType(Compiler &c)
    {
        return c.textTreePtrTy;
    }

    static JIT::Type_p NativeType(Compiler &c)
    {
        return c.textPtrTy;
    }
#endif

    static Tree *Shape()
    {
        return text_type;
    }
    static BoxType *Box(native_type x, TreePosition pos)
    {
        return new BoxType(x, pos);
    }
    static native_type Unbox(BoxType *x)
    {
        return x->value;
    }
    static native_type Unbox(Tree *x)
    {
        if (BoxType *check = x->As<BoxType>())
            return Unbox(check);
        Ooops("Expected a text value, got $1", x);
        return 0;
    }
};


template <>
struct xl_type<char>
// ----------------------------------------------------------------------------
//   Specialization for C++-style characters
// ----------------------------------------------------------------------------
{
    typedef Text         BoxType;
    typedef char         native_type;

#ifndef INTERPRETER_ONLY
    static JIT::PointerType_p TreeType(Compiler &c)
    {
        return c.textTreePtrTy;
    }

    static JIT::Type_p NativeType(Compiler &c)
    {
        return c.characterTy;
    }
#endif

    static Tree *Shape()
    {
        return text_type;
    }
    static BoxType *Box(native_type x, TreePosition pos)
    {
        return new BoxType(text(x, 1), "'", "'", pos);
    }
    static native_type Unbox(BoxType *x)
    {
        return x->value[0];
    }
    static native_type Unbox(Tree *x)
    {
        if (BoxType *check = x->As<BoxType>())
            if (check->IsCharacter())
                return Unbox(check);
        Ooops("Expected a character value, got $1", x);
        return 0;
    }
};


template <>
struct xl_type<Scope *>
// ----------------------------------------------------------------------------
//   Specialization for scope pointers
// ----------------------------------------------------------------------------
{
    typedef Scope        BoxType;
    typedef Scope *      native_type;

#ifndef INTERPRETER_ONLY
    static JIT::PointerType_p TreeType(Compiler &c)
    {
        return c.scopePtrTy;
    }

    static JIT::Type_p NativeType(Compiler &c)
    {
        return c.scopePtrTy;
    }
#endif

    static Tree *Shape()
    {
        return scope_type;
    }
    static BoxType *Box(native_type x, TreePosition pos)
    {
        return x;
    }
    static native_type Unbox(BoxType *x)
    {
        return x;
    }
    static native_type Unbox(Tree *x)
    {
        if (BoxType *check = x->As<BoxType>())
            return Unbox(check);
        Ooops("Expected a scope, got $1", x);
        return 0;
    }
};



// ============================================================================
//
//    Check if a type is a scope
//
// ============================================================================

template <typename T>
struct xl_is_scope              { static const bool is_scope = false; };

template<>
struct xl_is_scope<Scope *>     { static const bool is_scope = true; };



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
    typedef R (*pointer_type)();
    typedef typename xl_type<R>::BoxType BoxType;
    static const unsigned Arity = 0;
    static const bool IsXLFunction = false;

#ifndef INTERPRETER_ONLY
    static void         Args(Compiler &, JIT::Signature &)  {}
#endif
    static Tree_p       ParameterShape(uint &index)
    {
        record(native, "ParameterShape returns null");
        return nullptr;
    }
    static Tree_p       ReturnShape()
    {
        Tree_p ret = xl_type<R>::Shape();
        record(native, "ReturnShape () = %t", ret);
        return ret;
    }

    static BoxType *Call(pointer_type callee, Tree *self, Tree_p *args)
    {
        return xl_type<R>::Box(callee(), self->Position());
    }
};


template <>
struct function_type<void(*)()>
// ----------------------------------------------------------------------------
//   Special case for void return type
// ----------------------------------------------------------------------------
{
    typedef void return_type;
    typedef void (*pointer_type)();
    typedef typename xl_type<void>::BoxType BoxType;
    static const unsigned Arity = 0;
    static const bool IsXLFunction = false;

#ifndef INTERPRETER_ONLY
    static void         Args(Compiler &, JIT::Signature &)  {}
#endif
    static Tree_p       ParameterShape(uint &index)
    {
        record(native, "ParameterShape returns null");
        return nullptr;
    }
    static Tree_p       ReturnShape()
    {
        Tree_p ret = xl_type<void>::Shape();
        record(native, "ReturnShape () = %t", ret);
        return ret;
    }

    static BoxType *Call(pointer_type callee, Tree *self, Tree_p *args)
    {
        callee();
        return xl_nil;
    }
};


template <typename R, typename T>
struct function_type<R(*)(T)>
// ----------------------------------------------------------------------------
//   Special case for a function with a single argument
// ----------------------------------------------------------------------------
{
    typedef R return_type;
    typedef R (*pointer_type)(T);
    typedef typename xl_type<R>::BoxType BoxType;
    static const unsigned Arity = 1;
    static const bool IsXLFunction = xl_is_scope<T>::is_scope;

#ifndef INTERPRETER_ONLY
    static void Args(Compiler &compiler, JIT::Signature &signature)
    {
        JIT::Type_p argTy = xl_type<T>::NativeType(compiler);
        signature.push_back(argTy);
    }
#endif
    static Tree_p ParameterShape(uint &index)
    {
        Tree_p type = xl_type<T>::Shape();
        Name_p name = new Name(text(1, 'a' + index), Tree::BUILTIN);
        ++index;
        if (type)
        {
            Infix_p infix = new Infix(":", name, type);
            record(native,
                   "ParameterShape %u infix %t : %t = %t",
                   index, type, name, infix);
            return infix;
        }
        record(native, "ParameterShape %u name %t", index, name);
        return name;
    }
    static Tree_p ReturnShape()
    {
        Tree_p ret = xl_type<R>::Shape();
        record(native, "Return shape (one) %t", ret);
        return ret;
    }

    static BoxType *Call(pointer_type callee, Tree *self, Tree_p *args)
    {
        return xl_type<R>::Box(callee(xl_type<T>::Unbox(args[0])),
                               self->Position());
    }
};


template <typename T>
struct function_type<void(*)(T)>
// ----------------------------------------------------------------------------
//   Special case for a function with a single argument
// ----------------------------------------------------------------------------
{
    typedef void return_type;
    typedef void (*pointer_type)(T);
    typedef typename xl_type<void>::BoxType BoxType;
    static const unsigned Arity = 1;
    static const bool IsXLFunction = xl_is_scope<T>::is_scope;

#ifndef INTERPRETER_ONLY
    static void Args(Compiler &compiler, JIT::Signature &signature)
    {
        JIT::Type_p argTy = xl_type<T>::NativeType(compiler);
        signature.push_back(argTy);
    }
#endif
    static Tree_p ParameterShape(uint &index)
    {
        Tree_p type = xl_type<T>::Shape();
        Name_p name = new Name(text(1, 'a' + index), Tree::BUILTIN);
        ++index;
        if (type)
        {
            Infix_p infix = new Infix(":", name, type);
            record(native,
                   "ParameterShape %u infix %t : %t = %t",
                   index, type, name, infix);
            return infix;
        }
        record(native, "ParameterShape %u name %t", index, name);
        return name;
    }
    static Tree_p ReturnShape()
    {
        Tree_p ret = xl_type<void>::Shape();
        record(native, "Return shape (one) %t", ret);
        return ret;
    }

    static BoxType *Call(pointer_type callee, Tree *self, Tree_p *args)
    {
        callee(xl_type<T>::Unbox(args[0]));
        return xl_nil;
    }
};


template <typename R, typename T, typename ... A>
struct function_type<R(*)(T,A...)>
// ----------------------------------------------------------------------------
//   Recursive for the rest
// ----------------------------------------------------------------------------
{
    typedef R return_type;
    typedef R (*pointer_type)(T, A...);
    typedef typename xl_type<R>::BoxType BoxType;
    static const unsigned Arity = 1 + sizeof...(A);
    static const bool IsXLFunction = xl_is_scope<T>::is_scope;

#ifndef INTERPRETER_ONLY
    static void Args(Compiler &compiler, JIT::Signature &signature)
    {
        function_type<R(*)(T)>::Args(compiler, signature);
        function_type<R(*)(A...)>::Args(compiler, signature);
    }
#endif
    static Tree_p ParameterShape(uint &index)
    {
        Tree_p left = function_type<R(*)(T)>::ParameterShape(index);
        Tree_p right = function_type<R(*)(A...)>::ParameterShape(index);
        Infix_p infix = new Infix(",", left, right);
        record(native, "ParameterShape %u (...) %t,%t = %t",
               index, left, right, infix);
        return infix;
    }
    static Tree_p ReturnShape()
    {
        Tree_p ret = xl_type<R>::Shape();
        record(native, "Return shape (...) %t", ret);
        return ret;
    }

    static size_t Next(size_t &index)
    {
        // This is to silence the compiler about sequence points, but
        // this is really playing fast and loose. Works for now.
        return index++;
    }

    static BoxType *Call(pointer_type callee, Tree *self, Tree_p *args)
    {
        size_t index = 0;
        auto value = callee(xl_type<T>::Unbox(args[Next(index)]),
                            xl_type<A>::Unbox(args[Next(index)])...);
        return xl_type<R>::Box(value, self->Position());
    }
};


template <typename T, typename ... A>
struct function_type<void(*)(T,A...)>
// ----------------------------------------------------------------------------
//   Recursive for the rest
// ----------------------------------------------------------------------------
{
    typedef void return_type;
    typedef void (*pointer_type)(T, A...);
    typedef typename xl_type<void>::BoxType BoxType;
    static const unsigned Arity = 1 + sizeof...(A);
    static const bool IsXLFunction = xl_is_scope<T>::is_scope;

#ifndef INTERPRETER_ONLY
    static void Args(Compiler &compiler, JIT::Signature &signature)
    {
        function_type<void(*)(T)>::Args(compiler, signature);
        function_type<void(*)(A...)>::Args(compiler, signature);
    }
#endif
    static Tree_p ParameterShape(uint &index)
    {
        Tree_p left = function_type<void(*)(T)>::ParameterShape(index);
        Tree_p right = function_type<void(*)(A...)>::ParameterShape(index);
        Infix_p infix = new Infix(",", left, right);
        record(native, "ParameterShape %u (...) %t,%t = %t",
               index, left, right, infix);
        return infix;
    }
    static Tree_p ReturnShape()
    {
        Tree_p ret = xl_type<void>::Shape();
        record(native, "Return shape (...) %t", ret);
        return ret;
    }

    static size_t Next(size_t &index)
    {
        return index++;
    }

    static BoxType *Call(pointer_type callee, Tree *self, Tree_p *args)
    {
        size_t index = 0;
        callee(xl_type<T>::Unbox(args[Next(index)]),
               xl_type<A>::Unbox(args[Next(index)])...);
        return xl_nil;
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
    typedef Interpreter::builtin_fn builtin_fn;

    virtual     ~NativeInterface() {}
#ifndef INTERPRETER_ONLY
    virtual     JIT::Type_p          ReturnType(Compiler &)              = 0;
    virtual     JIT::FunctionType_p  FunctionType(Compiler &)            = 0;
    virtual     JIT::Function_p      Prototype(Compiler &, text name)    = 0;
#endif
    virtual     Tree_p               Shape(Name_p, uint &index)          = 0;
    virtual     Tree_p               Call(Bindings &bindings)            = 0;
};


template<typename FType>
struct NativeImplementation : NativeInterface
// ----------------------------------------------------------------------------
//   Generate the interface
// ----------------------------------------------------------------------------
{
    typedef function_type<FType>             ftype;
    typedef typename ftype::return_type      rtype;
    typedef typename ftype::pointer_type     ptype;
    typedef typename xl_type<rtype>::BoxType BoxType;

    NativeImplementation(ptype function): function(function) {}

#ifndef INTERPRETER_ONLY
    virtual JIT::Type_p ReturnType(Compiler &compiler) override
    {
        xl_type<rtype> xlt;
        return xlt.NativeType(compiler);
    }

    virtual JIT::FunctionType_p FunctionType(Compiler &compiler) override
    {
        JIT::Type_p rty = ReturnType(compiler);

        ftype ft;
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
#endif

    virtual Tree_p Shape(Name_p name, uint &index) override
    {
        ftype ft;
        Tree_p shape = name;
        Tree_p parms = ft.ParameterShape(index);
        if (parms)
            shape = new Prefix(shape, parms, Tree::BUILTIN);
        Tree_p retType = ft.ReturnShape();
        if (retType)
            shape = new Infix("as", shape, retType, Tree::BUILTIN);
        record(native, "Native shape %t arity %u", shape, index);
        return shape;
    }

    static Tree *Call(ptype function, Bindings &bindings)
    {
        size_t max = bindings.Size();
        if (ftype::IsXLFunction)
            max++;
        if (max != ftype::Arity)
        {
            Ooops("Wrong number of arguments for native $1 ($2 instead of $3)")
                .Arg(bindings.Self()).Arg(max).Arg(ftype::Arity);
            return nullptr;
        }

        TreeList args;
        if (ftype::IsXLFunction)
            args.push_back(bindings.EvaluationScope());
        for (size_t a = 0; a < bindings.Size(); a++)
            args.push_back(bindings.Argument(a));
        return ftype::Call(function, bindings.Self(), &args[0]);
    }

    virtual Tree_p Call(Bindings &bindings) override
    {
        return Call(function, bindings);
    }

    ptype       function;
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
    typedef Interpreter::builtin_fn builtin_fn;

    template<typename fntype>
    Native(fntype function, kstring name)
        : symbol(name),
          implementation(new NativeImplementation<fntype>(function)),
          shape(nullptr),
          next(list)
    {
        list = this;
    }
    ~Native();

    static Native *     First()                 { return list; }
    Native *            Next()                  { return next; }
    kstring             Symbol()                { return symbol; }

#ifndef INTERPRETER_ONLY
    JIT::Type_p         ReturnType(Compiler &compiler);
    JIT::FunctionType_p FunctionType(Compiler &compiler);
    JIT::Function_p     Prototype(Compiler &compiler, text name);
    static void         EnterPrototypes(Compiler &compiler);
#endif

    Tree_p              Shape();
    Tree_p              Call(Bindings &bindings);

public:
    kstring             symbol;
    NativeInterface *   implementation;
    Tree_p              shape;

private:
    static Native      *list;
    Native             *next;
};


#ifndef INTERPRETER_ONLY
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
#endif


inline Tree_p Native::Shape()
// ----------------------------------------------------------------------------
//   Delegate the shape generation to the implementation
// ----------------------------------------------------------------------------
{
    if (!shape)
    {
        uint index = 0;
        Name_p name = new Name(Normalize(symbol), symbol, Tree::BUILTIN);
        shape = implementation->Shape(name, index);
    }
    return shape;
}


inline Tree_p Native::Call(Bindings &bindings)
// ----------------------------------------------------------------------------
//   Delegate the shape generation to the implementation
// ----------------------------------------------------------------------------
{
    return implementation->Call(bindings);
}

} // namespace XL

#define NATIVE(Name)    static Native xl_##Name##_native(Name, #Name);

#endif // NATIVE_H
