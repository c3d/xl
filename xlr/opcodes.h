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

typedef Tree *(*opcode_fn)();

struct Opcode : Info
// ----------------------------------------------------------------------------
//    An opcode, registered at initialization time
// ----------------------------------------------------------------------------
//    This mechanism is designed to allow compile-time registration of opcodes
//    in a way that does not depend on C++ initialization order
//    Can't use C++ static objects here, as they may be initialized later
//    than the objects we register.
{
    typedef std::vector<Opcode *> Opcodes;

    enum Arity
    {
        ARITY_NONE,             // No input parameter
        ARITY_ONE,              // One input parameter
        ARITY_TWO,              // Two input parameters
        ARITY_CONTEXT_ONE,      // Context and one input parameter
        ARITY_CONTEXT_TWO,      // Context and two input parameters
        ARITY_FUNCTION,         // Argument list
        ARITY_SELF              // Pass self as argument
    };

    // Implementation for various arities
    typedef Tree *(*arity_none_fn)();
    typedef Tree *(*arity_one_fn)(Tree *);
    typedef Tree *(*arity_two_fn)(Tree *, Tree *);
    typedef Tree *(*arity_ctxone_fn)(Context *, Tree *);
    typedef Tree *(*arity_ctxtwo_fn)(Context *, Tree *, Tree *);
    typedef Tree *(*arity_function_fn)(TreeList &args);
    typedef Tree *(*arity_self_fn)();

public:
    Opcode(kstring name, arity_none_fn fn, Arity arity)
        : arity_none(fn), name(name), arity(arity)
    {
        if (!opcodes)
            opcodes = new Opcodes;
        opcodes->push_back(this);
    }
    virtual void                Delete(){ /* Not owned by the tree */ }
    virtual void                Register(Context *);
    virtual Tree *              Shape() { return NULL; }

    
    Tree *Run(Context *context, Tree *self, TreeList &args)
    {
        uint size = args.size();
        switch(arity)
        {
        case Opcode::ARITY_NONE:
            if (size == 0)
                return arity_none();
            break;
        case Opcode::ARITY_ONE:
            if (size == 1)
                return arity_one(args[0]);
            break;
        case Opcode::ARITY_TWO:
            if (size == 2)
                return arity_two(args[0], args[1]);
            break;
        case Opcode::ARITY_CONTEXT_ONE:
            if (size == 1)
                return arity_ctxone(context, args[0]);
            break;
        case Opcode::ARITY_CONTEXT_TWO:
            if (size == 2)
                return arity_ctxtwo(context, args[0], args[1]);
            break;
        case Opcode::ARITY_FUNCTION:
            return arity_function(args);

        case Opcode::ARITY_SELF:
            return self;
        }
        return NULL;
    }
        
public:
    union
    {
        arity_none_fn           arity_none;
        arity_one_fn            arity_one;
        arity_two_fn            arity_two;
        arity_ctxone_fn         arity_ctxone;
        arity_ctxtwo_fn         arity_ctxtwo;
        arity_function_fn       arity_function;
        arity_self_fn           arity_self;
    };
    kstring                     name;
    Arity                       arity;

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
        : Opcode(name, NULL, ARITY_SELF), toDefine(toDefine)
    {
        toDefine = new Name(name);
    }
    NameOpcode(kstring name, arity_none_fn fn, Name_p &toDefine, kstring symbol)
        : Opcode(name, fn, ARITY_NONE), toDefine(toDefine)
    {
        toDefine = new Name(symbol);
    }
    
    virtual void                Register(Context *);
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
    InfixOpcode(kstring name, arity_two_fn fn,
                kstring infix, Name_p &leftTy, Name_p &rightTy, Name_p &resTy,
                Opcode::Arity arity = ARITY_TWO)
        : Opcode(name, (arity_none_fn) fn, arity),
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
    PrefixOpcode(kstring name, arity_one_fn fn,
                 kstring prefix, Name_p &argTy, Name_p &resTy,
                 Arity arity = ARITY_ONE)
        : Opcode(name, (arity_none_fn) fn, arity),
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
    PostfixOpcode(kstring name, arity_one_fn fn,
                 kstring postfix, Name_p &argTy, Name_p &resTy)
        : Opcode(name, (arity_none_fn) fn, ARITY_ONE),
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
    FunctionOpcode(kstring name, arity_function_fn fn)
        : Opcode(name, (arity_none_fn) fn, ARITY_FUNCTION),
          result(NULL), ptr(&result) {}
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

#define RESULT(Code)                                                    \
/* ------------------------------------------------------------ */      \
/*  Check if the argument count matches what is expected        */      \
/* ------------------------------------------------------------ */      \
    return Code;

#define LEFT            left.value
#define RIGHT           right.value
#define ULEFT           ((ulonglong) LEFT)
#define URIGHT          ((ulonglong) RIGHT)
#define RIGHT0          ( RIGHT != 0 ? RIGHT : DIV0)
#define URIGHT0         (URIGHT != 0 ? URIGHT : DIV0)
#define DIV0            (Ooops("Divide by 0 for $1", rightPtr), 1)
#define LEFT_B          (left.value == "true")
#define RIGHT_B         (right.value == "true")
#define AS_INT(x)       (new Integer((x), left.Position()))
#define AS_REAL(x)      (new Real((x), left.Position()))
#define AS_BOOL(x)      ((x) ? xl_true : xl_false)
#define AS_TEXT(x)      (new Text(x, left.Position()))
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
#undef INFIX_CTX
#undef PREFIX
#undef PREFIX_FN
#undef PREFIX_CTX
#undef POSTFIX
#undef OVERLOAD
#undef FUNCTION
#undef PARM
#undef NAME
#undef NAME_FN
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
#define INFIX_CTX(Name, ResTy, LeftTy, Symbol, RightTy, Code)
#define PREFIX(Name, ResTy, Symbol, RightTy, Code)
#define PREFIX_FN(Name, ResTy, RightTy, Code)
#define PREFIX_CTX(Name, ResTy, Symbol, RightTy, Code)
#define POSTFIX(Name, ResTy, RightTy, Symbol, Code)
#define OVERLOAD(Name, Symbol, ResTy, Parms, Code)
#define FUNCTION(Name, ResTy, Parms, Code)
#define PARM(Name, Type)
#define NAME(symbol)            extern Name_p xl_##symbol;
#define NAME_FN(Name, ResTy, Symbol, Code)


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


#define ARG(Name, Type, Value)                                          \
/* ------------------------------------------------------------ */      \
/*  Extract an argument from the argument list                  */      \
/* ------------------------------------------------------------ */      \
    Type##_r *Name##Ptr = (Value)->As<Type##_r>();                      \
    if (!Name##Ptr)                                                     \
    {                                                                   \
        Ooops("Argument $1 to $2 is not a " #Type, (Value))             \
            .Arg(#Name);                                                \
        return NULL;                                                    \
    }                                                                   \
    Type##_r &Name = *Name##Ptr; (void) Name;


#define UNARY(Name, ResTy, LeftTy, Code)                                \
/* ------------------------------------------------------------ */      \
/*  Create a unary opcode (for 'opcode X' declarations)         */      \
/* ------------------------------------------------------------ */      \
    static Tree *Opcode_U_##Name (Tree *input)                          \
    {                                                                   \
        ARG(left, LeftTy, input);                                       \
        RESULT(Code);                                                   \
    }                                                                   \
    static Opcode                                                       \
    init_opcode_U_##Name(#Name, opcode_fn(Opcode_U_##Name),             \
                         Opcode::ARITY_ONE);


#define BINARY(Name, ResTy, LeftTy, RightTy, Code)                      \
/* ------------------------------------------------------------ */      \
/*  Create a binary opcode (for 'opcode X' declarations)        */      \
/* ------------------------------------------------------------ */      \
    static Tree * Opcode_B_##Name(Tree *arg1, Tree *arg2)               \
    {                                                                   \
        ARG(left,  LeftTy, arg1);                                       \
        ARG(right, RightTy, arg2);                                      \
        RESULT(Code);                                                   \
    }                                                                   \
    static Opcode                                                       \
    init_opcode_B_##Name(#Name, opcode_fn(Opcode_B_##Name),             \
                         Opcode::ARITY_TWO);


#define INFIX(Name, ResTy, LeftTy, Symbol, RightTy, Code)               \
/* ------------------------------------------------------------ */      \
/*  Create an infix opcode, also generates infix declaration    */      \
/* ------------------------------------------------------------ */      \
    static Tree * Opcode_I_##Name(Tree *arg1, Tree *arg2)               \
    {                                                                   \
        ARG(left,  LeftTy, arg1);                                       \
        ARG(right, RightTy, arg2);                                      \
        Code;                                                           \
    }                                                                   \
                                                                        \
    static InfixOpcode                                                  \
    init_opcode_I_##Name (#Name, Opcode_I_##Name, Symbol,               \
                          LeftTy##_type, RightTy##_type, ResTy##_type);


#define INFIX_CTX(Name, ResTy, LeftTy, Symbol, RightTy, Code)           \
/* ------------------------------------------------------------ */      \
/*  Create an infix opcode, also generates infix declaration    */      \
/* ------------------------------------------------------------ */      \
    static Tree * Opcode_I_##Name(Context *context,                     \
                                  Tree *arg1, Tree *arg2)               \
    {                                                                   \
        ARG(left,  LeftTy, arg1);                                       \
        ARG(right, RightTy, arg2);                                      \
        Code;                                                           \
    }                                                                   \
                                                                        \
    static InfixOpcode                                                  \
    init_opcode_I_##Name (#Name, Opcode::arity_two_fn(Opcode_I_##Name), \
                          Symbol,                                       \
                          LeftTy##_type, RightTy##_type, ResTy##_type,  \
                          Opcode::ARITY_CONTEXT_TWO);


#define PREFIX(Name, ResTy, Symbol, RightTy, Code)                      \
/* ------------------------------------------------------------ */      \
/*  Create a prefix opcode, also generates prefix declaration   */      \
/* ------------------------------------------------------------ */      \
    static Tree *Opcode_P_##Name(Tree *input)                           \
    {                                                                   \
        ARG(left, RightTy, input);                                      \
        Code;                                                           \
    }                                                                   \
                                                                        \
    static PrefixOpcode                                                 \
    init_opcode_P_##Name (#Name, Opcode_P_##Name,                       \
                          Symbol, RightTy##_type, ResTy##_type,         \
                          Opcode::ARITY_ONE);


#define PREFIX_FN(Name, ResTy, RightTy, Code)                           \
/* ------------------------------------------------------------ */      \
/*  Create a prefix opcode for a single-argument function       */      \
/* ------------------------------------------------------------ */      \
    PREFIX(Name, ResTy, #Name, RightTy, Code)


#define PREFIX_CTX(Name, ResTy, Symbol, RightTy, Code)                  \
/* ------------------------------------------------------------ */      \
/*  Create a prefixopcode, also generates prefix declaration    */      \
/* ------------------------------------------------------------ */      \
    static Tree *Opcode_P_##Name(Context *context, Tree *input)         \
    {                                                                   \
        ARG(left, RightTy, input);                                      \
        Code;                                                           \
    }                                                                   \
                                                                        \
    static PrefixOpcode                                                 \
    init_opcode_P_##Name (#Name, Opcode::arity_one_fn(Opcode_P_##Name), \
                          Symbol, RightTy##_type, ResTy##_type,         \
                          Opcode::ARITY_CONTEXT_ONE);


#define POSTFIX(Name, ResTy, LeftTy, Symbol, Code)                      \
/* ------------------------------------------------------------ */      \
/*  Create a prefixopcode, also generates prefix declaration    */      \
/* ------------------------------------------------------------ */      \
    static Tree *Opcode_P_##Name(Tree *input)                           \
    {                                                                   \
        ARG(left, LeftTy, input);                                       \
        Code;                                                           \
    }                                                                   \
                                                                        \
    static PostfixOpcode                                                \
    init_opcode_P_##Name (#Name, Opcode_P_##Name,                       \
                          Symbol, LeftTy##_type, ResTy##_type);


#define OVERLOAD(FName, Symbol, ResTy, Parms, Code)                     \
/* ------------------------------------------------------------ */      \
/*  Create a function opcode, also generates prefix declaration */      \
/* ------------------------------------------------------------ */      \
                                                                        \
    static Tree *Opcode_F_##FName(TreeList &args)                       \
    {                                                                   \
        FunctionArguments _XLparms(args);                               \
        Parms;                                                          \
        if (_XLparms.index != args.size())                              \
        {                                                               \
            return Ooops("Invalid argument count for " #FName           \
                         " after $1", args[0]);                         \
        }                                                               \
        Code;                                                           \
    }                                                                   \
                                                                        \
    struct Opcode_FC_##FName : FunctionOpcode                           \
    {                                                                   \
        Opcode_FC_##FName()                                             \
            : FunctionOpcode(#FName, Opcode_F_##FName) {}               \
                                                                        \
        virtual Tree *Shape()                                           \
        {                                                               \
            Tree *self = xl_nil;                                        \
            Opcode_FC_##FName &_XLparms = *this; (void) _XLparms;       \
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
    static Opcode_FC_##FName init_opcode_FC_##FName;


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


#define NAME_FN(Name, ResTy, Symbol, Code)                              \
/* ------------------------------------------------------------ */      \
/*  Create a function with zero argument                        */      \
/* ------------------------------------------------------------ */      \
    static Tree *Opcode_N_##Name()                                      \
    {                                                                   \
        Code;                                                           \
    }                                                                   \
    Name_p xl_##Name;                                                   \
    static NameOpcode                                                   \
    init_opcode_N_##Name (#Name, Opcode_N_##Name,                       \
                          xl_##Name, Symbol);


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
