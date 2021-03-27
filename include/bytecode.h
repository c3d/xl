#ifndef BYTECODE_H
#define BYTECODE_H
// *****************************************************************************
// bytecode.h                                                        XL project
// *****************************************************************************
//
// File description:
//
//     A bytecode-based translation of XL programs
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
// (C) 2015-2021, Christophe de Dinechin <christophe@dinechin.org>
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
#include "context.h"
#include "evaluator.h"


RECORDER_DECLARE(bytecode);
RECORDER_DECLARE(typecheck);
RECORDER_DECLARE(opcode);
RECORDER_DECLARE(opcode_run);
RECORDER_DECLARE(opcode_error);

XL_BEGIN

struct Bytecode;
struct RunState;
struct RunValue;
struct Bytecode;

typedef std::vector<Rewrite_p>          RewriteList;
typedef size_t                          opaddr_t;


class BytecodeEvaluator : public Evaluator
// ----------------------------------------------------------------------------
//   Base class for all ways to evaluate an XL tree
// ----------------------------------------------------------------------------
{
public:
    BytecodeEvaluator();
    virtual ~BytecodeEvaluator();

    Tree *Evaluate(Scope *, Tree *source) override;
    Tree *TypeCheck(Scope *, Tree *type, Tree *value) override;

    static void InitializeBuiltins();
    static void InitializeContext(Context &context);
};



// ============================================================================
//
//   Runtime representation of typed values
//
// ============================================================================

enum MachineType
// ----------------------------------------------------------------------------
//   An enum identifying all the machine types
// ----------------------------------------------------------------------------
{
    runvalue_unset = 0,         // For quick check if runvalue is set

    runvalue_pc, runvalue_bytecode, runvalue_locals,

#define RUNTIME_TYPE(Name, Rep, BC)     Name##_mtype,
#include "machine-types.tbl"

    BUILTIN_TYPES
};


inline bool IsTree(MachineType mtype)
// ----------------------------------------------------------------------------
//   Return true if this is a tree type
// ----------------------------------------------------------------------------
{
    switch(mtype)
    {
#define TREE_TYPE(Name, Rep, Code)      case  Name##_mtype:
#include "machine-types.tbl"
        return true;
    default:
        return false;
    }
}


inline bool IsNatural(MachineType mtype)
// ----------------------------------------------------------------------------
//   Return true if this is a natural machine type
// ----------------------------------------------------------------------------
{
    switch(mtype)
    {
#define NATURAL_TYPE(Name, Rep)         case  Name##_mtype:
#include "machine-types.tbl"
        return true;
    default:
        return false;
    }
}


inline bool IsInteger(MachineType mtype)
// ----------------------------------------------------------------------------
//   Return true if this is a integer machine type
// ----------------------------------------------------------------------------
{
    switch(mtype)
    {
#define INTEGER_TYPE(Name, Rep)         case  Name##_mtype:
#include "machine-types.tbl"
        return true;
    default:
        return false;
    }
}


inline bool IsReal(MachineType mtype)
// ----------------------------------------------------------------------------
//   Return true if this is a real machine type
// ----------------------------------------------------------------------------
{
    switch(mtype)
    {
#define REAL_TYPE(Name, Rep)         case  Name##_mtype:
#include "machine-types.tbl"
        return true;
    default:
        return false;
    }
}


inline bool IsText(MachineType mtype)
// ----------------------------------------------------------------------------
//   Return true if this is a text machine type
// ----------------------------------------------------------------------------
{
    return (mtype == text_mtype || mtype == character_mtype);
}


inline bool IsScalar(MachineType mtype)
// ----------------------------------------------------------------------------
//   Return true if this is a scalar type (non-tree)
// ----------------------------------------------------------------------------
{
    return !IsTree(mtype);
}


#ifdef HAVE_FLOAT16
inline std::ostream &operator<<(std::ostream &out, _Float16 f)
// ----------------------------------------------------------------------------
//    The type is not known to ostream, convert to double
// ----------------------------------------------------------------------------

{
    return out << (double) f;
}
#endif


struct RunValue
// ----------------------------------------------------------------------------
//   A representation that fits in two words and can be used efficiently
// ----------------------------------------------------------------------------
//   There are two small complications in the implementation of this class.
//
//   The first complication is the text class (and possibly others later)
//   It does not necessarily fit in a word (e.g. 3 words on clang macosx),
//   and may have a different representation for literals (const char *).
//   In a RunValue, we store a 'text *' allocated dynamically.
//   This is handled by the Representation class.
//
//   The second complication is handling tree types safely for the
//   garbage collector, which requires Acquire / Release.
{
public:
    typedef std::vector<RunValue>       ValueList;
    typedef std::map<Tree_p, RunValue>  ValueMap;

public:
    // Default constructor creates a nil value
    RunValue(MachineType mtype = runvalue_unset)
        : type(mtype), flags(0), as_tree(nullptr)               { }

    RunValue(opaddr_t addr, MachineType mtype)
        : type (mtype), flags(0), as_natural(addr)              { }

    RunValue(void *addr, MachineType mtype)
        : type (mtype), flags(0), as_natural((uintptr_t) addr) { }

    // Copy constructor
    RunValue(const RunValue &rv)
        : type(rv.type), flags(rv.flags),
          as_natural(rv.as_natural)                             { Acquire(); }

    // Destructor
    ~RunValue()                                                 { Release(); }

    // Assignment
    RunValue &operator=(const RunValue &rv)
    {
        Release();
        type = rv.type;
        as_tree = rv.as_tree;
        Acquire();
        return *this;
    }

    // Constructor from trees and constant trees
    RunValue(Tree *t, MachineType mtype = tree_mtype)
        : type(mtype), flags(0), as_tree(t)                     { Acquire(t); }
    RunValue(Natural *n)
        : type(n->IsSigned() ? integer_mtype : natural_mtype),
          flags(0), as_natural(n->value)                        { }
    RunValue(Real *r)
        : type(real_mtype), flags(0), as_real(r->value)         { }
    RunValue(Text *t)
        : type(t->IsCharacter() ? character_mtype : text_mtype),
          flags(0)
    {
        if (t->IsCharacter())
            as_character = t->value[0];
        else
            as_text = new text(t->value);
    }

    template<typename Rep>
    struct Representation
    // ------------------------------------------------------------------------
    //   Dependencies on the represented type
    // ------------------------------------------------------------------------
    //   By default, all representations are identical
    {
        typedef Rep value_t;
        typedef Rep literal_t;
        typedef Rep union_t;
        static  Rep ToUnion(Rep r)              { return r; }
        static  Rep ToValue(Rep r)              { return r; }
    };


    template<>
    struct Representation<text>
    // ------------------------------------------------------------------------
    //    Specialization for `text`
    // ------------------------------------------------------------------------
    {
        typedef text value_t;
        typedef kstring literal_t;
        typedef text *union_t;
        static  text *ToUnion(const text &t)    { return new text(t); }
        static  text  ToValue(text *ptr)        { return *ptr; }
    };


    template<MachineType mtype>
    struct Type
    // ------------------------------------------------------------------------
    //   Get the type of a field
    // ------------------------------------------------------------------------
    {
        typedef Tree *type;
    };


    template<MachineType mtype>
    typename Type<mtype>::type &Field()
    // ------------------------------------------------------------------------
    //   Get a field
    // ------------------------------------------------------------------------
    {
        return as_tree;
    }

#define RUNTIME_TYPE(Name, Rep, BC)                                     \
    template<> struct Type<Name##_mtype>                                \
    {                                                                   \
        typedef Representation<Rep>::union_t type;                      \
    };                                                                  \
    template<> typename Type<Name##_mtype>::type &Field<Name##_mtype>() \
    {                                                                   \
        return as_##Name;                                               \
    }
#include "machine-types.tbl"

    template<MachineType mtype>
    struct AsTreeType
    // ------------------------------------------------------------------------
    //   Type to use as an arg to AsTree
    // ------------------------------------------------------------------------
    {
        using type = typename Type<mtype>::type;
    };

#define NATURAL_TYPE(Name, Rep)                                         \
    template<> struct AsTreeType<Name##_mtype>    { typedef ulonglong type; };
#define INTEGER_TYPE(Name, Rep)                                         \
    template<> struct AsTreeType<Name##_mtype>    { typedef longlong  type; };
#define REAL_TYPE(Name, Rep)                                            \
    template<> struct AsTreeType<Name##_mtype>    { typedef double    type; };
#include "machine-types.tbl"

    // Construction and conversion from machine types
#define TREE_TYPE(Name, Rep, Cast)
#define MACHINE_TYPE(Name, Rep, BC)                     \
    RunValue(Representation<Rep>::value_t rep)          \
        : type(Name##_mtype), flags(0)                  \
    {                                                   \
        as_##Name = Representation<Rep>::ToUnion(rep);  \
    }                                                   \
    operator Rep() const                                \
    {                                                   \
        XL_ASSERT(type == Name##_mtype);                \
        return Representation<Rep>::ToValue(as_##Name); \
    }
MACHINE_TYPE(integer, long, naught)
MACHINE_TYPE(natural, unsigned long, naught)
#include "machine-types.tbl"

    // Special case kstring for Native interface
    RunValue(kstring rep): type(text_mtype),flags(0),as_text(new text(rep)) {}
    operator kstring() const
    {
        XL_ASSERT(type == text_mtype);
        return as_text->c_str();
    }

    inline Tree *AsTree() const
    {
        switch(type)
        {
#define RUNTIME_TYPE(Name, Rep, BC)                                     \
        case Name##_mtype:                                              \
            return AsTree(AsTreeType<Name##_mtype>::type(as_##Name));
#include "machine-types.tbl"
        default:
            break;
        }
        return as_tree;
    }

    inline ulonglong AsNatural()
    {
        switch(type)
        {
#define NATURAL_TYPE(Name, Rep)         case Name##_mtype: return as_##Name;
#include "machine-types.tbl"
        default:
            XL_ASSERT("Invalid machine type for AsNatural()");
            break;
        }
        return 0;
    }

    inline longlong AsInteger()
    {
        switch(type)
        {
#define INTEGER_TYPE(Name, Rep)         case Name##_mtype: return as_##Name;
#include "machine-types.tbl"
        default:
            XL_ASSERT("Invalid machine type for AsInteger()");
            break;
        }
        return 0;
    }

    inline double AsReal()
    {
        switch(type)
        {
#define REAL_TYPE(Name, Rep)         case Name##_mtype: return as_##Name;
#include "machine-types.tbl"
        default:
            XL_ASSERT("Invalid machine type for AsReal()");
            break;
        }
        return 0;
    }

    inline text AsText()
    {
        switch(type)
        {
        case text_mtype:        return *as_text;
        case character_mtype:   return text(as_character, 1);
        default:
            XL_ASSERT("Invalid machine type for AsText()");
            break;
        }
        return "";
    }


    // Classify a tree into a more precise RunValue types
    RunValue &Classify()
    {
        if (type != tree_mtype)
        {
            return *this;
        }
        Tree *tree = as_tree;
        if (!tree)
        {
            type = nil_mtype;
            return *this;
        }
        switch(tree->Kind())
        {
        case NATURAL:
        {
            Natural *xn = (Natural *) tree;
            if (xn->IsSigned())
            {
                type = integer_mtype;
                as_integer = (longlong) xn->value;
            }
            else
            {
                type = natural_mtype;
                as_natural = xn->value;
            }
            Release(xn);
            return *this;
        }
        case REAL:
        {
            Real *xr = (Real *) tree;
            type = real_mtype;
            as_real = xr->value;
            Release(xr);
            return *this;
        }
        case TEXT:
        {
            Text *xt = (Text *) tree;
            if (xt->IsCharacter())
            {
                type = character_mtype;
                as_character = xt->value[0];
            }
            else
            {
                type = text_mtype;
                as_text = new text(xt->value);
            }
            Release(xt);
            return *this;
        }
        case NAME:
        {
            Name *xn = (Name *) tree;
            if (xn->IsName())
                type = name_mtype;
            else if (xn->IsOperator())
                type = operator_mtype;
            else
                type = symbol_mtype;
            return *this;
        }
        case INFIX:
            type = infix_mtype;
            return *this;
        case PREFIX:
            if (IsError(tree))
                type = error_mtype;
            else
                type = prefix_mtype;
            return *this;
        case POSTFIX:
            type = postfix_mtype;
            return *this;
        case BLOCK:
            type = block_mtype;
            return *this;
        }
        return *this;
    }

    static RunValue Classify(Tree *tree)
    {
        return RunValue(tree, tree_mtype).Classify();
    }


    template <typename Rep, MachineType mtype>
    bool CastNatural()
    {
        if (type != mtype && IsNatural(type))
        {
            ulonglong value = AsNatural();
            Rep cast = (Rep) value;
            if (cast == value)
            {
                type = mtype;
                Field<mtype>() = cast;
            }
        }
        return (type == mtype);
    }


    template <typename Rep, MachineType mtype>
    bool CastInteger()
    {
        if (type != mtype && IsInteger(type))
        {
            longlong value = AsInteger();
            Rep cast = (Rep) value;
            if (cast == value)
            {
                type = mtype;
                Field<mtype>() = cast;
            }
        }
        return (type == mtype);
    }


    template <typename Rep, MachineType mtype>
    bool CastReal()
    {
        if (type != mtype && IsReal(type))
        {
            double value = AsReal();
            Rep cast = (Rep) value;
            if (cast == value)
            {
                type = mtype;
                Field<mtype>() = cast;
            }
        }
        return (type == mtype);
    }


    friend inline std::ostream &operator<<(std::ostream &out, RunValue &rv)
    {
        switch(rv.type)
        {
#define RUNTIME_TYPE(Name, Rep, BC)                             \
        case Name##_mtype:                                      \
            return out << #Name "(" << rv.as_##Name << ")";
#include "machine-types.tbl"
        default:
            return out << "Invalid";
        }
    }

    static Tree *AsTree(Tree *t)        { return t; }
    static Tree *AsTree(longlong n)     { return Natural::Signed(n); }
    static Tree *AsTree(ulonglong n)    { return new Natural(n); }
    static Tree *AsTree(double d)       { return new Real(d); }
    static Tree *AsTree(text *t)        { return new Text(*t); }
    static Tree *AsTree(text t)         { return new Text(t); }
    static Tree *AsTree(char c)         { return Text::Character(c); }
    static Tree *AsTree(bool b)         { return b ? xl_true : xl_false; }

private:
    void Acquire(Tree *tree)
    {
        TypeAllocator::Acquire(tree);
    }

    void Release(Tree *tree)
    {
        TypeAllocator::Release(tree);
    }

    void Acquire(text *tptr)
    {
        as_text = new text(*tptr);
    }

    void Release(text *tptr)
    {
        delete tptr;
    }

    template<class T> void Acquire(T &t) { }
    template<class T> void Release(T &t) { }

    void Acquire()
    {
        switch(type)
        {
#define RUNTIME_TYPE(Name, Rep, BC)             \
        case Name##_mtype:                      \
            Acquire(as_##Name);                 \
            break;
#include "machine-types.tbl"
        default: break;
        }
    }

    void Release()
    {
        switch(type)
        {
#define RUNTIME_TYPE(Name, Rep, BC)             \
        case Name##_mtype:                      \
            Release(as_##Name);                 \
            break;
#include "machine-types.tbl"
        default: break;
        }
    }

public:
    MachineType         type            :  8;
    unsigned            flags           : 24;
    union
    {
#define RUNTIME_TYPE(Name, Rep, BC)     Representation<Rep>::union_t as_##Name;
#include "machine-types.tbl"
    };
};


// Representation of a stack at runtime
typedef RunValue::ValueList     RunStack;
typedef RunValue::ValueList     RunValues;



// ============================================================================
//
//   Instruction opcodes and evaluation stack
//
// ============================================================================

struct RunState
// ----------------------------------------------------------------------------
//   The state of the program during evaluation
// ----------------------------------------------------------------------------
//   During evaluation, the stack looks like this:
//   - [...]
//   - Outermost stack  <--- frames[0]
//   - [...]

//   - Enclosing stack  <--- frames.back()
//   - caller's stack
//   - arg 0            <--- frame[0], locals
//   - arg 1
//   - ...
//   - local temp 0     <--- frame[nparms], locals + nparms
//   - local temp 1
//   - ...
//   The difference between locals and frame is the number of formal parameters
//   bound for the current code.
{
    RunState(Scope *scope, Tree *expr)
        : stack(), scope(scope), self(expr), error()
    { }

    void        Push(const RunValue &rv)        { stack.push_back(rv); }
    RunValue    Pop();
    RunValue    Pop(MachineType mtype);
    RunValue &  Top();
    void        Set(const RunValue &top)        { stack.back() = top; }
    size_t      Depth()                         { return stack.size(); }
    void        Allocate(size_t size);
    void        Cut(size_t size);
    Scope_p     EvaluationScope()               { return scope; }
    void        Error(Tree *msg)                { error = msg; }
    Tree *      Error()                         { return error; }
    void        Dump(std::ostream &out);

    typedef std::vector<size_t> Frames;

    RunStack    stack;                  // Evaluation stack and parameters
    Scope_p     scope;                  // Current evaluation scope
    Tree_p      self;                   // What we are evaluating
    Tree_p      error;                  // Error messages
};


inline RunValue RunState::Pop()
// ----------------------------------------------------------------------------
//   Return the last item on the stack and remove it
// ----------------------------------------------------------------------------
{
    if (!stack.size())
    {
        record(opcode_error, "Popping from empty stack evaluating %t", self);
        return RunValue();
    }
    RunValue result = stack.back();
    stack.pop_back();
    return result;
}


inline RunValue RunState::Pop(MachineType mtype)
// ----------------------------------------------------------------------------
//   Return the last item on the stack and remove it
// ----------------------------------------------------------------------------
{
    RunValue result = Pop();
    XL_ASSERT(result.type == mtype);
    return result;
}


inline RunValue &RunState::Top()
// ----------------------------------------------------------------------------
//   Return the last item on the stack without removing it
// ----------------------------------------------------------------------------
{
    if (!stack.size())
    {
        record(opcode_error,"Getting top from empty stack evaluating %t",self);
        stack.push_back(RunValue());
    }
    return stack.back();
}


inline void RunState::Allocate(size_t size)
// ----------------------------------------------------------------------------
//   Cut the stack at the given level
// ----------------------------------------------------------------------------
{
    stack.resize(stack.size() + size);
}


inline void RunState::Cut(size_t size)
// ----------------------------------------------------------------------------
//   Resize the stack at the given size
// ----------------------------------------------------------------------------
{
    stack.resize(size);
}

XL_END

#endif // BYTECODE_H
