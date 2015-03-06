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

struct Opcode : Info
// ----------------------------------------------------------------------------
//    An opcode, registered at initialization time
// ----------------------------------------------------------------------------
//    This mechanism is designed to allow compile-time registration of opcodes
//    in a way that does not depend on C++ initialization order
//    Can't use C++ static objects here, as they may be initialized later
//    than the objects we register.
{
    typedef Tree *(*callback_fn) (Context *ctx, Tree *self, TreeList &args);
    typedef std::vector<Opcode *> List;

public:
    Opcode(kstring name, callback_fn fn) : Invoke(fn), name(name)
    {
        if (!list)
            list = new List;
        list->push_back(this);
    }
    virtual void                Delete() { /* Not owned by the tree */ }
    virtual void                Register(Context *);
    virtual Tree *              Shape()  { return NULL; }

    callback_fn                 Invoke;
    kstring                     name;

public:
    static void                 Enter(Context *context);
    static Opcode *             Find(text name);
    static List *               list;
};


struct NameOpcode : Opcode
// ----------------------------------------------------------------------------
//    Opcode for names and types
// ----------------------------------------------------------------------------
{
    NameOpcode(kstring name, Name_p &toDefine)
        : Opcode(name, Evaluate), toDefine(toDefine)
    {
        toDefine = new Name(name);
    }
    
    virtual void                Register(Context *);
    static Tree *               Evaluate(Context *, Tree *self, TreeList&);
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
    InfixOpcode(kstring name,  callback_fn fn,
                kstring infix, Name_p &leftTy, Name_p &rightTy, Name_p &resTy)
        : Opcode(name, fn),
          infix(infix), leftTy(leftTy), rightTy(rightTy), resTy(resTy) {}

    virtual Tree *Shape()
    {
        return
            new Infix("as",
                      new Infix(infix,
                                new Infix(":", new Name("left"), leftTy),
                                new Infix(":", new Name("right"), rightTy)),
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
    PrefixOpcode(kstring name,  callback_fn fn,
                 kstring prefix, Name_p &argTy, Name_p &resTy)
        : Opcode(name, fn),
          prefix(prefix), argTy(argTy), resTy(resTy) {}

    virtual Tree *Shape()
    {
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
    PostfixOpcode(kstring name,  callback_fn fn,
                 kstring postfix, Name_p &argTy, Name_p &resTy)
        : Opcode(name, fn),
          postfix(postfix), argTy(argTy), resTy(resTy) {}

    virtual Tree *Shape()
    {
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
    FunctionOpcode(kstring name,  callback_fn fn)
        : Opcode(name, fn), result(NULL), ptr(&result) {}
    virtual Tree *Shape() = 0;

    template<class TreeType>
    TreeType *Parameter(kstring name)
    {
        Tree *type = OpcodeType((TreeType *) 0);
        Infix *parmDecl = new Infix(":", new Name(name), type);
        if (!result)
            *ptr = parmDecl;
        else
            *ptr = new Infix(",", *ptr, parmDecl);
        ptr = &parmDecl->right;
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

#define ARG(Name, Type, Value)                                          \
/* ------------------------------------------------------------ */      \
/*  Extract an argument from the argument list                  */      \
/* ------------------------------------------------------------ */      \
    Type##_p Name##Ptr = (Value)->As<Type##_r>();                       \
    if (!Name##Ptr)                                                     \
    {                                                                   \
        Ooops("Argument $1 is not a " #Type, (Value));                  \
        return self;                                                    \
    }                                                                   \
    Type##_r &Name = *Name##Ptr; (void) Name;


#define ARGCOUNT(N)                                                     \
/* ------------------------------------------------------------ */      \
/*  Check if the argument count matches what is expected        */      \
/* ------------------------------------------------------------ */      \
    (void) context;  /* Get rid of warnings */                          \
    if (args.size() != N)                                               \
    {                                                                   \
        Ooops("Invalid opcode argument count in $1", self);             \
        return self;                                                    \
    }


#define UNARY(Name, ResTy, LeftTy, Code)                                \
/* ------------------------------------------------------------ */      \
/*  Create a unary opcode (for 'opcode X' declarations)         */      \
/* ------------------------------------------------------------ */      \
    static Tree *                                                       \
    opcode_U_##Name(Context *context, Tree *self, TreeList &args)       \
    {                                                                   \
        ARGCOUNT(1);                                                    \
        ARG(left, LeftTy, args[0]);                                     \
        return Code;                                                    \
    }                                                                   \
    static Opcode init_opcode_U_##Name(#Name, opcode_U_##Name);


#define BINARY(Name, ResTy, LeftTy, RightTy, Code)                      \
/* ------------------------------------------------------------ */      \
/*  Create a binary opcode (for 'opcode X' declarations)        */      \
/* ------------------------------------------------------------ */      \
    static Tree *                                                       \
    opcode_B_##Name(Context *context, Tree *self, TreeList &args)       \
    {                                                                   \
        ARGCOUNT(2);                                                    \
        ARG(left,  LeftTy,  args[0]);                                   \
        ARG(right, RightTy, args[1]);                                   \
        return Code;                                                    \
    }                                                                   \
    static Opcode init_opcode_B_##Name(#Name, opcode_B_##Name);


#define INFIX(Name, ResTy, LeftTy, Symbol, RightTy, Code)               \
/* ------------------------------------------------------------ */      \
/*  Create an infix opcode, also generates infix declaration    */      \
/* ------------------------------------------------------------ */      \
    static Tree *                                                       \
    opcode_I_##Name(Context *context, Tree *self, TreeList &args)       \
    {                                                                   \
        ARGCOUNT(2);                                                    \
        ARG(left,  LeftTy,  args[0]);                                   \
        ARG(right, RightTy, args[1]);                                   \
        Code;                                                           \
    }                                                                   \
    static InfixOpcode                                                  \
    init_opcode_I_##Name (#Name, opcode_I_##Name, Symbol,               \
                          LeftTy##_type, RightTy##_type, ResTy##_type);


#define PREFIX(Name, ResTy, Symbol, RightTy, Code)                      \
/* ------------------------------------------------------------ */      \
/*  Create a prefixopcode, also generates prefix declaration    */      \
/* ------------------------------------------------------------ */      \
    static Tree *                                                       \
    opcode_P_##Name(Context *context, Tree *self, TreeList &args)       \
    {                                                                   \
        ARGCOUNT(1);                                                    \
        ARG(arg, RightTy, args[0]);                                     \
        Code;                                                           \
    }                                                                   \
    static PrefixOpcode                                                 \
    init_opcode_P_##Name (#Name, opcode_Pl_##Name, Symbol,              \
                          RightTy##_type, ResTy##_type);


#define POSTFIX(Name, ResTy, LeftTy, Symbol, Code)                      \
/* ------------------------------------------------------------ */      \
/*  Create a prefixopcode, also generates prefix declaration    */      \
/* ------------------------------------------------------------ */      \
    static Tree *                                                       \
    opcode_P_##Name(Context *context, Tree *self, TreeList &args)       \
    {                                                                   \
        ARGCOUNT(1);                                                    \
        ARG(arg, LeftTy, args[0]);                                      \
        Code;                                                           \
    }                                                                   \
    static PostfixOpcode                                                \
    init_opcode_P_##Name (#Name, opcode_Pl_##Name, Symbol,              \
                          LeftTy##_type, ResTy##_type);


#define OVERLOAD(FName, Symbol, ResTy, Parms, Code)                     \
/* ------------------------------------------------------------ */      \
/*  Create a function opcode, also generates prefix declaration */      \
/* ------------------------------------------------------------ */      \
    static Tree *                                                       \
    opcode_F_##FName(Context *context, Tree *self, TreeList &args)      \
    {                                                                   \
        (void) context;                                                 \
        FunctionArguments _XLparms(args);                               \
        Parms;                                                          \
        if (_XLparms.index != args.size())                              \
        {                                                               \
            Ooops("Invalid argument count for " #FName " in $1", self); \
            return self;                                                \
        }                                                               \
                                                                        \
        Code;                                                           \
    }                                                                   \
                                                                        \
    struct FName##FunctionOpcode : FunctionOpcode                       \
    {                                                                   \
        FName##FunctionOpcode(kstring name, callback_fn fn)             \
            : FunctionOpcode(name, fn) {}                               \
        virtual Tree *Shape()                                           \
        {                                                               \
            Tree *self = xl_nil;                                        \
            FName##FunctionOpcode &_XLparms = *this;                    \
            Parms;                                                      \
            if (result)                                                 \
                result = new Prefix(new Name(Symbol), result);          \
            else                                                        \
                result = new Name(name);                                \
            self = new Infix("as",                                      \
                             new Prefix(new Name(Symbol), result),      \
                             ResTy##_type);                             \
            return self;                                                \
        }                                                               \
    };                                                                  \
                                                                        \
    static FName##FunctionOpcode                                        \
    init_opcode_F_##FName (#FName, opcode_F_##FName);


#define FUNCTION(Name, ResTy, Parms, Code)                              \
/* ------------------------------------------------------------ */      \
/*  Declare a function with the given name                      */      \
/* ------------------------------------------------------------ */      \
        OVERLOAD(Name, Name, ResTy, Parms, Code)


#define PARM(Name, Type)                                                \
/* ------------------------------------------------------------ */      \
/*  Declare a parameter for the FUNCTION macro above            */      \
/* ------------------------------------------------------------ */      \
    Type##_p Name##Ptr = _XLparms.Parameter<Type##_r>(#Name);           \
    if (!Name##Ptr)                                                     \
        return self;                                                    \
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
                                                                        \
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
