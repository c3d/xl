#ifndef OPCODES_H
#define OPCODES_H
// ****************************************************************************
//  opcodes.h                                                   XLR project
// ****************************************************************************
//
//   File Description:
//
//    Opcodes are native trees generated as part of compilation/optimization
//    to speed up execution. They represent a step in the evaluation of
//    the code.
//
//    To add an extension to the list of builtin opcodes, use:
//       #include "opcodes.h"
//       #include "my-extension.tbl"
//
//    To see how my-extension.tbl is defined by example, see basics.tbl
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

#include "tree.h"
#include "context.h"
#include "errors.h"
#include "save.h"
#include "interpreter.h"

XL_BEGIN



// ============================================================================
// 
//    Forward types declared here
// 
// ============================================================================
//
//  Each XL type defined in opcodes or in .tbl file is represented as:
//  - A C++ class deriving from one of the parse tree types.
//    The name is [name]_r, for example class boolean_r derives from Name.
//  - A pointer type [name]_p, for example boolean_p
//  - A Name_p for the type name, called [name]_type, for example boolean_type
//
//  The TYPE macro also creates:
//  - A specialization for Tree::As that does the proper type checks
//    or return NULL if the item does not have the expected type
//  - A function called OpcodeType() to get [name]_type from a [name]_p.

extern Name_p   tree_type;
extern Name_p   integer_type;
extern Name_p   real_type;
extern Name_p   text_type;
extern Name_p   name_type;
extern Name_p   block_type;
extern Name_p   prefix_type;
extern Name_p   postfix_type;
extern Name_p   infix_type;

typedef Tree    Tree_r;
typedef Integer Integer_r;
typedef Real    Real_r;
typedef Text    Text_r;
typedef Name    Name_r;
typedef Block   Block_r;
typedef Prefix  Prefix_r;
typedef Postfix Postfix_r;
typedef Infix   Infix_r;


// ============================================================================
//
//    Registration classes
//
// ============================================================================

struct Opcode : Info, Op
// ----------------------------------------------------------------------------
//    An opcode, registered at initialization time
// ----------------------------------------------------------------------------
//    This mechanism is designed to allow compile-time registration of opcodes
//    in a way that does not depend on C++ initialization order
//    Can't use C++ static objects here, as they may be initialized later
//    than the objects we register.
{
    typedef std::vector<Opcode *> Opcodes;

public:
    Opcode(kstring name) : name(name)
    {
        if (!opcodes)
            opcodes = new Opcodes;
        opcodes->push_back(this);
    }
    virtual void                Delete() { /* Not owned by the tree */ }
    virtual void                Register(Context *);
    virtual Tree *              Shape()  { return NULL; }
    virtual bool                Run(Data &data)
    {
        XL_ASSERT(!"Unimplemented opcode");
        return false;
    }

public:
    kstring                     name;

public:
    static void                 Enter(Context *context);
    static Opcode *             Find(text name);
    static Opcodes *            opcodes;
};


struct NameOpcode : Opcode
// ----------------------------------------------------------------------------
//    Opcode for names and types
// ----------------------------------------------------------------------------
{
    NameOpcode(kstring name, Name_p &toDefine)
        : Opcode(name), toDefine(toDefine)
    {
        toDefine = new Name(name);
    }
    
    virtual void                Register(Context *);
    virtual bool                Run(Data &data) { return true; }
    Name_p &                    toDefine;
};


struct TypeCheckOpcode : NameOpcode
// ----------------------------------------------------------------------------
//    A structure to quickly do the most common type checks
// ----------------------------------------------------------------------------
{
    TypeCheckOpcode(kstring name, Name_p &toDefine)
        : NameOpcode(name, toDefine) {}
    virtual Tree *              Check(Context *ctx, Tree *what)
    {
        return what;
    }
};


struct InfixOpcode : Opcode
// ----------------------------------------------------------------------------
//   An infix opcode, regisered at initialization time
// ----------------------------------------------------------------------------
//   We need to keep references to the original type names, as they
//   may not be initialized at construction time yet
{
    InfixOpcode(kstring name,
                kstring infix, Name_p &leftTy, Name_p &rightTy, Name_p &resTy)
        : Opcode(name),
          infix(infix), leftTy(leftTy), rightTy(rightTy), resTy(resTy) {}

    virtual Tree *Shape()
    {
        Save<TreePosition> savePos(Tree::NOWHERE, Tree::BUILTIN);
        return
            new Infix("as",
                      new Infix(infix,
                                new Infix(":", new Name("left"),
                                          leftTy),
                                new Infix(":", new Name("right"),
                                          rightTy)),
                      resTy);
    }

    kstring     infix;
    Name_p &    leftTy;
    Name_p &    rightTy;
    Name_p &    resTy;
};


struct PrefixOpcode : Opcode
// ----------------------------------------------------------------------------
//   An unary prefix opcode, regisered at initialization time
// ----------------------------------------------------------------------------
{
    PrefixOpcode(kstring name,
                 kstring prefix, Name_p &argTy, Name_p &resTy)
        : Opcode(name),
          prefix(prefix), argTy(argTy), resTy(resTy) {}

    virtual Tree *Shape()
    {
        Save<TreePosition> savePos(Tree::NOWHERE, Tree::BUILTIN);
        return
            new Infix("as",
                      new Prefix(new Name(prefix),
                                 new Infix(":", new Name("left"), argTy)),
                      resTy);
    }

    kstring     prefix;
    Name_p &    argTy;
    Name_p &    resTy;
};


struct PostfixOpcode : Opcode
// ----------------------------------------------------------------------------
//   An unary postfix opcode, regisered at initialization time
// ----------------------------------------------------------------------------
{
    PostfixOpcode(kstring name,
                 kstring postfix, Name_p &argTy, Name_p &resTy)
        : Opcode(name),
          postfix(postfix), argTy(argTy), resTy(resTy) {}

    virtual Tree *Shape()
    {
        Save<TreePosition> savePos(Tree::NOWHERE, Tree::BUILTIN);
        return
            new Infix("as",
                      new Postfix(new Infix(":", new Name("left"), argTy),
                                  new Name(postfix)),
                      resTy);
    }

    kstring     postfix;
    Name_p &    argTy;
    Name_p &    resTy;
};


struct FunctionOpcode : Opcode
// ----------------------------------------------------------------------------
//   A function opcode - Build the parameter list at initialization time
// ----------------------------------------------------------------------------
//   This is intended to be used with the PARM macro below
{
    FunctionOpcode(kstring name)
        : Opcode(name), result(NULL), ptr(&result) {}
    virtual Tree *Shape() = 0;

    template<class TreeType>
    TreeType *Parameter(kstring name)
    {
        Save<TreePosition> savePos(Tree::NOWHERE, Tree::BUILTIN);
        Tree *type = OpcodeType((TreeType *) 0);
        Infix *parmDecl = new Infix(":", new Name(name), type);
        if (!result)
        {
            *ptr = parmDecl;
        }
        else
        {
            Infix *comma = new Infix(",", *ptr, parmDecl);
            *ptr = comma;
            ptr = &comma->right;
        }
        return (TreeType *) parmDecl; // Only need it to be non-zero
    }

    Tree_p  result;
    Tree_p *ptr;
};


struct FunctionArguments
// ----------------------------------------------------------------------------
//    A structure used to extract function arguments safely
// ----------------------------------------------------------------------------
{
    FunctionArguments(TreeList &args) : args(args), index(0) {}

    template <class TreeType>
    TreeType *Parameter(kstring name)
    {
        if (index >= args.size())
        {
            Ooops("Not enough arguments for parameter $1").Arg(name);
            return NULL;
        }
        Tree *tree = args[index++];
        TreeType *result = tree->As<TreeType>();
        if (!result)
            Ooops("Value of $2 is $1, expected $3", tree)
                .Arg(name).Arg(OpcodeType((TreeType *) 0));
        return result;
    }

    TreeList &  args;
    uint        index;
};


XL_END



// ============================================================================
// 
//    Macros to make it easier to write computation built-ins
// 
// ============================================================================

#define INIT_CONTEXT_AND_SELF                                           \
/* ------------------------------------------------------------ */      \
/*  Check if the argument count matches what is expected        */      \
/* ------------------------------------------------------------ */      \
    Context *context = data.context;                                    \
    (void) context;  /* Get rid of warnings */                          \
    Tree *self = data.Pop();                                            \


#define RESULT(Code)                                                    \
/* ------------------------------------------------------------ */      \
/*  Check if the argument count matches what is expected        */      \
/* ------------------------------------------------------------ */      \
    Tree *result = Code;                                                \
    data.Push(result);                                                  \
    return true;

#define LEFT            left.value
#define RIGHT           right.value
#define ULEFT           ((ulonglong) LEFT)
#define URIGHT          ((ulonglong) RIGHT)
#define RIGHT0          ( RIGHT != 0 ? RIGHT : DIV0)
#define URIGHT0         (URIGHT != 0 ? URIGHT : DIV0)
#define DIV0            (Ooops("Divide by 0 in $1", self), 1)
#define LEFT_B          (left.value == "true")
#define RIGHT_B         (right.value == "true")
#define AS_INT(x)       (new Integer((x), self->Position()))
#define AS_REAL(x)      (new Real((x), self->Position()))
#define AS_BOOL(x)      ((x) ? xl_true : xl_false)
#define AS_TEXT(x)      (new Text(x, self->Position()))
#define R_INT(x)        RESULT(AS_INT(x))
#define R_REAL(x)       RESULT(AS_REAL(x))
#define R_BOOL(x)       RESULT(AS_BOOL(x))
#define R_TEXT(x)       RESULT(AS_TEXT(x))

#endif // OPCODES_H


#ifdef TBL_HEADER
#  undef OPCODES_TBL
#endif


#ifndef OPCODES_TBL
#define OPCODES_TBL
// ============================================================================
//
//    Declarations macros for use in TBL files
//
// ============================================================================

#undef INIT_GC
#undef INIT_ALLOCATOR
#undef UNARY
#undef BINARY
#undef INFIX
#undef PREFIX
#undef POSTFIX
#undef OVERLOAD
#undef FUNCTION
#undef PARM
#undef NAME
#undef TYPE



#ifdef TBL_HEADER

// ============================================================================
//
//    Declarations in header files
//
// ============================================================================

#define INIT_GC
#define INIT_ALLOCATOR(type)
#define UNARY(Name, ResTy, LeftTy, Code)
#define BINARY(Name, ResTy, LeftTy, RightTy, Code)
#define INFIX(Name, ResTy, LeftTy, Symbol, RightTy, Code)
#define PREFIX(Name, ResTy, Symbol, RightTy, Code)
#define POSTFIX(Name, ResTy, RightTy, Symbol, Code)
#define OVERLOAD(Name, Symbol, ResTy, Parms, Code)
#define FUNCTION(Name, ResTy, Parms, Code)
#define PARM(Name, Type)
#define NAME(symbol)            extern Name_p xl_##symbol;

#define TYPE(symbol, BaseType, Condition)                               \
/* ------------------------------------------------------------ */      \
/*  Declare a type and the related type conversions             */      \
/* ------------------------------------------------------------ */      \
    extern  Name_p symbol##_type;                                       \
    struct  symbol##_r : BaseType##_r {};                               \
    typedef symbol##_r *symbol##_p;                                     \
    typedef symbol##_r::value_t symbol##_t;                             \
    template<> inline                                                   \
    symbol##_r *Tree::As<symbol##_r>(Context *context)                  \
    {                                                                   \
        (void) context;                                                 \
        Condition;                                                      \
        return NULL;                                                    \
    }                                                                   \
                                                                        \
    inline Tree *OpcodeType(symbol##_p)                                 \
    {                                                                   \
        return symbol##_type;                                           \
    }
    

#undef OPCODES_TBL
#undef TBL_HEADER

#else // TBL_HEADER

// ============================================================================
//
//    Definitions in implementation files
//
// ============================================================================

#define INIT_GC                                                         \
/* ------------------------------------------------------------ */      \
/*  Initialize the garbage collector                            */      \
/* ------------------------------------------------------------ */      \
    static void *init_allocator_for_##type =                            \
        (void *) GarbageCollector::CreateSingleton();


#define INIT_ALLOCATOR(type)                                            \
/* ------------------------------------------------------------ */      \
/*  Initialize a type allocator                                 */      \
/* ------------------------------------------------------------ */      \
    template<>                                                          \
    Allocator<type> *Allocator<type>::allocator =                       \
        Allocator<type>::CreateSingleton();


#define ARG(Name, Type, Index)                                          \
/* ------------------------------------------------------------ */      \
/*  Extract an argument from the argument list                  */      \
/* ------------------------------------------------------------ */      \
    Type##_r *Name##Ptr = data.vars[Index]->As<Type##_r>();             \
    if (!Name##Ptr)                                                     \
    {                                                                   \
        Ooops("Argument $1 to $2 is not a " #Type, data.vars[Index])    \
            .Arg(__FUNCTION__);                                         \
        return false;                                                   \
    }                                                                   \
    Type##_r &Name = *Name##Ptr; (void) Name;


#define ARGCOUNT(N)                                                     \
/* ------------------------------------------------------------ */      \
/*  Check if the argument count matches what is expected        */      \
/* ------------------------------------------------------------ */      \
    INIT_CONTEXT_AND_SELF;                                              \
    if (data.vars.size() != N)                                          \
    {                                                                   \
        Ooops("Invalid opcode argument count in $1", self);             \
        return false;                                                   \
    }


#define UNARY(Name, ResTy, LeftTy, Code)                                \
/* ------------------------------------------------------------ */      \
/*  Create a unary opcode (for 'opcode X' declarations)         */      \
/* ------------------------------------------------------------ */      \
    struct Opcode_U_##Name : Opcode                                     \
    {                                                                   \
        Opcode_U_##Name(kstring name): Opcode(name) {}                  \
                                                                        \
        virtual bool Run(Data &data)                                    \
        {                                                               \
            ARGCOUNT(1);                                                \
            ARG(left, LeftTy, 0);                                       \
            RESULT(Code);                                               \
        }                                                               \
    };                                                                  \
   static Opcode_U_##Name init_opcode_U_##Name(#Name);


#define BINARY(Name, ResTy, LeftTy, RightTy, Code)                      \
/* ------------------------------------------------------------ */      \
/*  Create a binary opcode (for 'opcode X' declarations)        */      \
/* ------------------------------------------------------------ */      \
    struct Opcode_B_##Name : Opcode                                     \
    {                                                                   \
        Opcode_B_##Name(kstring name): Opcode(name) {}                  \
                                                                        \
        virtual bool Run(Data &data)                                    \
        {                                                               \
            ARGCOUNT(2);                                                \
            ARG(left,  LeftTy,  0);                                     \
            ARG(right, RightTy, 1);                                     \
            RESULT(Code);                                               \
        }                                                               \
    };                                                                  \
    static Opcode_B_##Name init_opcode_B_##Name(#Name);


#define INFIX(Name, ResTy, LeftTy, Symbol, RightTy, Code)               \
/* ------------------------------------------------------------ */      \
/*  Create an infix opcode, also generates infix declaration    */      \
/* ------------------------------------------------------------ */      \
    struct Opcode_I_##Name : InfixOpcode                                \
    {                                                                   \
        Opcode_I_##Name(kstring name,                                   \
                        kstring i, Name_p &l, Name_p &r, Name_p &res)   \
            : InfixOpcode(name, i, l, r, res) {}                        \
                                                                        \
        virtual bool Run(Data &data)                                    \
        {                                                               \
            ARGCOUNT(2);                                                \
            ARG(left,  LeftTy,  0);                                     \
            ARG(right, RightTy, 1);                                     \
            Code;                                                       \
        }                                                               \
    };                                                                  \
                                                                        \
    static Opcode_I_##Name                                              \
    init_opcode_I_##Name (#Name, Symbol,                                \
                          LeftTy##_type, RightTy##_type,                \
                          ResTy##_type);


#define PREFIX(Name, ResTy, Symbol, RightTy, Code)                      \
/* ------------------------------------------------------------ */      \
/*  Create a prefixopcode, also generates prefix declaration    */      \
/* ------------------------------------------------------------ */      \
    struct Opcode_P_##Name : PrefixOpcode                               \
    {                                                                   \
        Opcode_P_##Name(kstring name,                                   \
                        kstring prefix, Name_p &r, Name_p &res)         \
            : PrefixOpcode(name, prefix, r, res) {}                     \
                                                                        \
        virtual bool Run(Data &data)                                    \
        {                                                               \
            ARGCOUNT(1);                                                \
            ARG(arg, RightTy,  0);                                      \
            Code;                                                       \
        }                                                               \
    };                                                                  \
                                                                        \
    static Opcode_P_##Name                                              \
    init_opcode_P_##Name (#Name, Symbol, RightTy##_type, ResTy##_type);


#define POSTFIX(Name, ResTy, LeftTy, Symbol, Code)                      \
/* ------------------------------------------------------------ */      \
/*  Create a prefixopcode, also generates prefix declaration    */      \
/* ------------------------------------------------------------ */      \
    struct Opcode_P_##Name : PostfixOpcode                              \
    {                                                                   \
        Opcode_P_##Name(kstring name,                                   \
                        kstring postfix, Name_p &l, Name_p &res)        \
            : PostfixOpcode(name, prefix, l, res) {}                    \
                                                                        \
        virtual bool Run(Data &data)                                    \
        {                                                               \
            ARGCOUNT(1);                                                \
            ARG(arg, LeftTy,  args[0]);                                 \
            Code;                                                       \
        }                                                               \
    };                                                                  \
                                                                        \
    static Opcode_P_##Name                                              \
    init_opcode_P_##Name (#Name, Symbol, LeftTy##_type, ResTy##_type);


#define OVERLOAD(FName, Symbol, ResTy, Parms, Code)                     \
/* ------------------------------------------------------------ */      \
/*  Create a function opcode, also generates prefix declaration */      \
/* ------------------------------------------------------------ */      \
    struct Opcode_F_##FName : FunctionOpcode                            \
    {                                                                   \
        Opcode_F_##FName(kstring name)                                  \
            : FunctionOpcode(name) {}                                   \
                                                                        \
        virtual bool Run(Data &data)                                    \
        {                                                               \
            INIT_CONTEXT_AND_SELF;                                      \
            FunctionArguments _XLparms(data.vars);                      \
            Parms;                                                      \
            if (_XLparms.index != data.vars.size())                     \
            {                                                           \
                Ooops("Invalid argument count for "                     \
                      #FName " in $1", self);                           \
                return false;                                           \
            }                                                           \
                                                                        \
            Code;                                                       \
        }                                                               \
                                                                        \
        virtual Tree *Shape()                                           \
        {                                                               \
            Tree *self = xl_nil;                                        \
            Opcode_F_##FName &_XLparms = *this; (void) _XLparms;        \
            Parms;                                                      \
            if (result)                                                 \
                result = new Prefix(new Name(Symbol), result);          \
            else                                                        \
                result = new Name(Symbol);                              \
            self = new Infix("as", result, ResTy##_type);               \
            return self;                                                \
        }                                                               \
    };                                                                  \
                                                                        \
    static Opcode_F_##FName init_opcode_F_##FName (#FName);


#define FUNCTION(Name, ResTy, Parms, Code)                              \
/* ------------------------------------------------------------ */      \
/*  Declare a function with the given name                      */      \
/* ------------------------------------------------------------ */      \
        OVERLOAD(Name, #Name, ResTy, Parms, Code)


#define PARM(Name, Type)                                                \
/* ------------------------------------------------------------ */      \
/*  Declare a parameter for the FUNCTION macro above            */      \
/* ------------------------------------------------------------ */      \
    Type##_r *Name##Ptr = _XLparms.Parameter<Type##_r>(#Name);          \
    if (!Name##Ptr)                                                     \
        return 0;                                                       \
    Type##_r &Name = *Name##Ptr; (void) &Name;


#define NAME(symbol)                                                    \
/* ------------------------------------------------------------ */      \
/*  Declare a simple name such as 'true', 'false', 'nil', etc   */      \
/* ------------------------------------------------------------ */      \
    Name_p xl_##symbol;                                                 \
    static NameOpcode init_opcode_N_##symbol(#symbol, xl_##symbol);


#define TYPE(symbol, BaseType, Conversion)                              \
/* ------------------------------------------------------------ */      \
/*  Declare a type with the condition to match it               */      \
/* ------------------------------------------------------------ */      \
    struct symbol##TypeCheckOpcode : TypeCheckOpcode                    \
    {                                                                   \
        symbol##TypeCheckOpcode(kstring name, Name_p &which):           \
            TypeCheckOpcode(name, which) {}                             \
                                                                        \
        virtual Tree *Check(Context *context, Tree *what)               \
        {                                                               \
            return what->As<symbol##_r>(context);                       \
        }                                                               \
    };                                                                  \
                                                                        \
    Name_p symbol##_type;                                               \
    static symbol##TypeCheckOpcode                                      \
    init_opcode_T_##symbol (#symbol, symbol##_type);

#endif // TBL_HEADER

#endif // OPCODES_TBL
