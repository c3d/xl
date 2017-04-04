#ifndef OPCODES_H
#define OPCODES_H
// ****************************************************************************
//  opcodes.h                                                    ELFE project
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
#include "bytecode.h"


ELFE_BEGIN

// ============================================================================
//
//    Forward types declared here
//
// ============================================================================
//
//  Each ELFE type defined in opcodes or in .tbl file is represented as:
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

#define Tree_type       tree_type
#define Integer_type    integer_type
#define Real_type       real_type
#define Text_type       text_type
#define Name_type       name_type
#define Block_type      block_type
#define Prefix_type     prefix_type
#define Postfix_type    postfix_type
#define Infix_type      infix_type

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

struct Opcode : Op, Info
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
    Opcode()
    {
        if (!opcodes)
            opcodes = new Opcodes;
        opcodes->push_back(this);
    }
    Opcode(const Opcode &other): Op(other), Info()
    {
        ELFE_ASSERT(success == NULL);
        ELFE_ASSERT(next == NULL);
        ELFE_ASSERT(owner == NULL);
    }
    
    virtual void                Delete(){ /* Not owned by the tree */ }
    virtual void                Register(Context *);
    virtual Tree *              Shape() { return NULL; }
    virtual Opcode *            Clone() = 0;
    virtual Op *                Run(Data data) = 0;
    virtual void                SetParms(ParmOrder &parms)  {}

public:
    static void                 Enter(Context *context);
    static Opcode *             Find(Tree *self, text name);
    static Opcodes *            opcodes;
};


struct NameOpcode : Opcode
// ----------------------------------------------------------------------------
//    Opcode for names and types
// ----------------------------------------------------------------------------
{
    NameOpcode(kstring name, Name_p &toDefine)
        : toDefine(toDefine)
    {
        toDefine = new Name(name);
    }

    virtual Op *                Run(Data data)
    {
        DataResult(data, toDefine);
        return success;
    }
    
    virtual Tree *              Shape() { return toDefine; }
    virtual void                Register(Context *);
    virtual Opcode *            Clone() { return new NameOpcode(*this); }
    virtual kstring             OpID()  { return toDefine->value.c_str(); }
    virtual void                Dump(std::ostream &out)
    {
        out << "name\t" << toDefine->value;
    }
    Name_p &                    toDefine;
};


struct TypeCheckOpcode : NameOpcode
// ----------------------------------------------------------------------------
//    A structure to quickly do the most common type checks
// ----------------------------------------------------------------------------
{
    TypeCheckOpcode(kstring name, Name_p &toDefine)
        : NameOpcode(name, toDefine) {}
    virtual Opcode *            Clone() { return new TypeCheckOpcode(*this); }
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
    InfixOpcode(kstring infix, Name_p &leftTy, Name_p &rightTy, Name_p &resTy)
        : infix(infix), leftTy(leftTy), rightTy(rightTy), resTy(resTy) {}

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

    virtual uint        Inputs()        { return 2; }
    virtual kstring     OpID()          { return infix; }
    virtual void        Dump(std::ostream &out)
    {
        out << "infix\t" << infix;
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
    PrefixOpcode(kstring prefix, Name_p &argTy, Name_p &resTy)
        : prefix(prefix), argTy(argTy), resTy(resTy) {}

    virtual Tree *Shape()
    {
        Save<TreePosition> savePos(Tree::NOWHERE, Tree::BUILTIN);
        return
            new Infix("as",
                      new Prefix(new Name(prefix),
                                 new Infix(":", new Name("left"), argTy)),
                      resTy);
    }

    virtual uint        Inputs()        { return 1; }
    virtual kstring     OpID()          { return prefix; }
    virtual void        Dump(std::ostream &out)
    {
        out << "prefix\t" << prefix;
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
    PostfixOpcode(kstring postfix, Name_p &argTy, Name_p &resTy)
        : postfix(postfix), argTy(argTy), resTy(resTy) {}

    virtual Tree *Shape()
    {
        Save<TreePosition> savePos(Tree::NOWHERE, Tree::BUILTIN);
        return
            new Infix("as",
                      new Postfix(new Infix(":", new Name("left"), argTy),
                                  new Name(postfix)),
                      resTy);
    }

    virtual uint        Inputs()        { return 1; }
    virtual kstring     OpID()          { return postfix; }
    virtual void        Dump(std::ostream &out)
    {
        out << "postfix\t" << postfix;
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
    FunctionOpcode() : result(NULL), ptr(&result), size(0) {}
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
        size++;
        return (TreeType *) parmDecl; // Only need it to be non-zero
    }

    Tree_p  result;
    Tree_p *ptr;
    uint    size;
};


struct FunctionArguments
// ----------------------------------------------------------------------------
//    A structure used to extract function arguments safely
// ----------------------------------------------------------------------------
{
    FunctionArguments(Data args, uint size): args(args), index(0), size(size) {}

    template <class TreeType>
    TreeType *Parameter(kstring name)
    {
        if ((uint) -index >= size)
        {
            Ooops("Not enough arguments for parameter $1").Arg(name);
            return NULL;
        }
        Tree *tree = args[--index];
        TreeType *result = tree->As<TreeType>();
        if (!result)
            Ooops("Value of $2 is $1, expected $3", tree)
                .Arg(name).Arg(OpcodeType((TreeType *) 0));
        return result;
    }

    Data        args;
    int         index;
    uint        size;
};

ELFE_END



// ============================================================================
//
//    Macros to make it easier to write computation built-ins
//
// ============================================================================

#define RESULT(Code)                                                    \
/* ------------------------------------------------------------ */      \
/*  Check if the argument count matches what is expected        */      \
/* ------------------------------------------------------------ */      \
    Tree *result = Code;                                                \
    DataResult(data, result);                                           \
    return success;

#define LEFT            left.value
#define RIGHT           right.value
#define ULEFT           ((ulonglong) LEFT)
#define URIGHT          ((ulonglong) RIGHT)
#define RIGHT0          ( RIGHT != 0 ? RIGHT : DIV0)
#define URIGHT0         (URIGHT != 0 ? URIGHT : DIV0)
#define DIV0            (Ooops("Divide by 0 for $1", rightPtr), 1)
#define LEFT_B          (left.value == "true")
#define RIGHT_B         (right.value == "true")
#define POSITION        (DataSelf(data)->Position())
#define SCOPE           (DataScope(data))
#define CONTEXT         (Context(SCOPE).Pointer())
#define AS_INT(x)       (new Integer((x), POSITION))
#define AS_REAL(x)      (new Real((x), POSITION))
#define AS_BOOL(x)      ((x) ? elfe_true : elfe_false)
#define AS_TEXT(x)      (new Text(x, POSITION))
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
#define NAME(symbol)            extern Name_p elfe_##symbol;
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
    inline Name *OpcodeType(symbol##_p)                                 \
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


#define UNARY(UName, ResTy, LeftTy, Code)                               \
/* ------------------------------------------------------------ */      \
/*  Create a unary opcode (for 'opcode X' declarations)         */      \
/* ------------------------------------------------------------ */      \
    struct Opcode_U_##UName : PrefixOpcode                              \
    {                                                                   \
        Opcode_U_##UName(int argID = -1)                                \
            : PrefixOpcode(#UName, LeftTy##_type, ResTy##_type),        \
              argID(argID) {}                                           \
        virtual Op *Run(Data data)                                      \
        {                                                               \
            ARG(left, LeftTy, data[argID]);                             \
            Code;                                                       \
            return success;                                             \
        }                                                               \
        virtual kstring OpID()  { return #UName; }                      \
        virtual void Dump(std::ostream &out)                            \
        {                                                               \
            out << #UName "\t" << argID;                                \
        }                                                               \
        virtual Opcode *Clone()                                         \
        {                                                               \
            return new Opcode_U_##UName(*this);                         \
        }                                                               \
        virtual void SetParms(ParmOrder &parms)                         \
        {                                                               \
            ELFE_ASSERT(parms.size() == 1);                            \
            argID = parms[0];                                           \
        }                                                               \
                                                                        \
        int argID;                                                      \
    };                                                                  \
                                                                        \
    static Opcode_U_##UName init_opcode_U_##UName;


#define BINARY(BName, ResTy, LeftTy, RightTy, Code)                     \
/* ------------------------------------------------------------ */      \
/*  Create a binary opcode (for 'opcode X' declarations)        */      \
/* ------------------------------------------------------------ */      \
    struct Opcode_B_##BName : InfixOpcode                               \
    {                                                                   \
        Opcode_B_##BName(int leftID = -1, int rightID = -2)             \
            : InfixOpcode(#BName,                                       \
                          LeftTy##_type, RightTy##_type, ResTy##_type), \
              leftID(leftID), rightID(rightID) {}                       \
        virtual Op *Run(Data data)                                      \
        {                                                               \
            ARG(left, LeftTy, data[leftID]);                            \
            ARG(right, RightTy, data[rightID]);                         \
            Code;                                                       \
            return success;                                             \
        }                                                               \
        virtual kstring OpID()  { return #BName; }                      \
        virtual void Dump(std::ostream &out)                            \
        {                                                               \
            out << #BName "\t" << leftID << "," << rightID;             \
        }                                                               \
        virtual Opcode *Clone()                                         \
        {                                                               \
            return new Opcode_B_##BName(*this);                         \
        }                                                               \
        virtual void SetParms(ParmOrder &parms)                         \
        {                                                               \
            ELFE_ASSERT(parms.size() == 2);                             \
            leftID = parms[0];                                          \
            rightID = parms[1];                                         \
        }                                                               \
        int leftID, rightID;                                            \
    };                                                                  \
                                                                        \
    static Opcode_B_##BName init_opcode_B_##BName;


#define INFIX(IName, ResTy, LeftTy, Symbol, RightTy, Code)              \
/* ------------------------------------------------------------ */      \
/*  Create an infix opcode, also generates infix declaration    */      \
/* ------------------------------------------------------------ */      \
    struct Opcode_I_##IName : InfixOpcode                               \
    {                                                                   \
        Opcode_I_##IName(int leftID = -1, int rightID = -2)             \
            : InfixOpcode(Symbol,                                       \
                          LeftTy##_type, RightTy##_type, ResTy##_type), \
              leftID(leftID), rightID(rightID) {}                       \
        virtual Op *Run(Data data)                                      \
        {                                                               \
            ARG(left, LeftTy, data[leftID]);                            \
            ARG(right, RightTy, data[rightID]);                         \
            Code;                                                       \
            return success;                                             \
        }                                                               \
        virtual kstring OpID()  { return #IName; }                      \
        virtual void Dump(std::ostream &out)                            \
        {                                                               \
            out << #IName "\t" << leftID << Symbol << rightID;          \
        }                                                               \
        virtual Opcode *Clone()                                         \
        {                                                               \
            return new Opcode_I_##IName(*this);                         \
        }                                                               \
        virtual void SetParms(ParmOrder &parms)                         \
        {                                                               \
            ELFE_ASSERT(parms.size() == 2);                            \
            leftID = parms[0];                                          \
            rightID = parms[1];                                         \
        }                                                               \
        int leftID, rightID;                                            \
    };                                                                  \
                                                                        \
    static Opcode_I_##IName init_opcode_I_##IName;


#define INFIX_CTX(IName, ResTy, LeftTy, Symbol, RightTy, Code)          \
/* ------------------------------------------------------------ */      \
/*  Create an infix opcode, also generates infix declaration    */      \
/* ------------------------------------------------------------ */      \
    INFIX(IName, ResTy, LeftTy, Symbol, RightTy,                        \
          Scope_p scope = DataScope(data);                              \
          Context_p context = new Context(scope);                       \
          Code)


#define PREFIX(PName, ResTy, Symbol, RightTy, Code)                     \
/* ------------------------------------------------------------ */      \
/*  Create a prefix opcode, also generates prefix declaration   */      \
/* ------------------------------------------------------------ */      \
    struct Opcode_P_##PName : PrefixOpcode                              \
    {                                                                   \
        Opcode_P_##PName(int argID = -1)                                \
            : PrefixOpcode(Symbol, RightTy##_type, ResTy##_type),       \
              argID(argID) {}                                           \
        virtual Op *Run(Data data)                                      \
        {                                                               \
            ARG(left, RightTy, data[argID]);                            \
            Code;                                                       \
            return success;                                             \
        }                                                               \
        virtual kstring OpID()  { return #PName; }                      \
        virtual void Dump(std::ostream &out)                            \
        {                                                               \
            out << #PName "\t" << argID;                                \
        }                                                               \
        virtual Opcode *Clone()                                         \
        {                                                               \
            return new Opcode_P_##PName(*this);                         \
        }                                                               \
        virtual void SetParms(ParmOrder &parms)                         \
        {                                                               \
            ELFE_ASSERT(parms.size() == 1);                            \
            argID = parms[0];                                           \
        }                                                               \
        int argID;                                                      \
    };                                                                  \
                                                                        \
    static Opcode_P_##PName init_opcode_P_##PName;


#define PREFIX_FN(Name, ResTy, RightTy, Code)                           \
/* ------------------------------------------------------------ */      \
/*  Create a prefix opcode for a single-argument function       */      \
/* ------------------------------------------------------------ */      \
    PREFIX(Name, ResTy, #Name, RightTy, Code)


#define PREFIX_CTX(Name, ResTy, Symbol, RightTy, Code)                  \
/* ------------------------------------------------------------ */      \
/*  Create a prefixopcode, also generates prefix declaration    */      \
/* ------------------------------------------------------------ */      \
    PREFIX(Name, ResTy, Symbol, RightTy,                                \
          Scope_p scope = DataScope(data);                              \
          Context_p context = new Context(scope);                       \
          Code)


#define POSTFIX(PName, ResTy, LeftTy, Symbol, Code)                     \
/* ------------------------------------------------------------ */      \
/*  Create a prefixopcode, also generates prefix declaration    */      \
/* ------------------------------------------------------------ */      \
    struct Opcode_p_##PName : PrefixOpcode                              \
    {                                                                   \
        Opcode_p_##PName(int argID = -1)                                \
            : PostfixOpcode(Symbol, LeftTy##_type, ResTy##_type),       \
              argID(argID) {}                                           \
        virtual Op *Run(Data data)                                      \
        {                                                               \
            ARG(left, LeftTy, data[argID]);                             \
            Code;                                                       \
            return success;                                             \
        }                                                               \
        virtual kstring OpID()  { return #PName; }                      \
        virtual void Dump(std::ostream &out)                            \
        {                                                               \
            out << #PName "\t" << argID;                                \
        }                                                               \
        virtual Opcode *Clone()                                         \
        {                                                               \
            return new Opcode_p_##PName(*this);                         \
        }                                                               \
        int argID;                                                      \
    };                                                                  \
                                                                        \
    static Opcode_p_##PName init_opcode_p_##PName;


#define OVERLOAD(FName, Symbol, ResTy, Parms, Code)                     \
/* ------------------------------------------------------------ */      \
/*  Create a function opcode, also generates prefix declaration */      \
/* ------------------------------------------------------------ */      \
    struct Opcode_FC_##FName : FunctionOpcode                           \
    {                                                                   \
        Opcode_FC_##FName() : FunctionOpcode() {}                       \
                                                                        \
        virtual Tree *Shape()                                           \
        {                                                               \
            Tree *self = elfe_nil;                                     \
            Opcode_FC_##FName &_parms = *this;                          \
            _parms.size = 0;                                            \
            Parms;                                                      \
            if (result)                                                 \
                result = new Prefix(new Name(Symbol), result);          \
            else                                                        \
                result = new Name(Symbol);                              \
            self = new Infix("as", result, ResTy##_type);               \
            return self;                                                \
        }                                                               \
                                                                        \
        virtual Op *Run(Data data)                                      \
        {                                                               \
            FunctionArguments _parms(data, size);                       \
            Parms;                                                      \
            if (_parms.size != size)                                    \
                Ooops("Invalid argument count for " #FName              \
                      " in $1", data[0]);                               \
            Code;                                                       \
            return success;                                             \
        }                                                               \
                                                                        \
        virtual kstring OpID() { return #FName; }                       \
        virtual void Dump(std::ostream &out)                            \
        {                                                               \
             static Tree_p shape = Shape();                             \
             out << #FName "\t" << shape;                               \
        }                                                               \
                                                                        \
        virtual Opcode *Clone()                                         \
        {                                                               \
            return new Opcode_FC_##FName(*this);                        \
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
    Type##_r *Name##Ptr = _parms.Parameter<Type##_r>(#Name);            \
    if (!Name##Ptr)                                                     \
        return 0;                                                       \
    Type##_r &Name = *Name##Ptr; (void) &Name;


#define NAME(symbol)                                                    \
/* ------------------------------------------------------------ */      \
/*  Declare a simple name such as 'true', 'false', 'nil', etc   */      \
/* ------------------------------------------------------------ */      \
    Name_p elfe_##symbol;                                              \
    static NameOpcode init_opcode_N_##symbol(#symbol, elfe_##symbol);


#define NAME_FN(FName, ResTy, Symbol, Code)                             \
/* ------------------------------------------------------------ */      \
/*  Create a function with zero argument                        */      \
/* ------------------------------------------------------------ */      \
                                                                        \
    struct Opcode_N_##FName : NameOpcode                                \
    {                                                                   \
        Opcode_N_##FName(Name_p &toDefine)                              \
            : NameOpcode(Symbol, toDefine) {}                           \
                                                                        \
        virtual Op *Run(Data data)                                      \
        {                                                               \
            Code;                                                       \
            return success;                                             \
        }                                                               \
        virtual Opcode *Clone() { return new Opcode_N_##FName(*this); } \
        virtual kstring OpID()  { return #FName; }                      \
    };                                                                  \
                                                                        \
    Name_p elfe_##FName;                                               \
    static Opcode_N_##FName init_opcode_N_##FName (elfe_##FName);


#define TYPE(sym, BaseType, Condition)                                  \
/* ------------------------------------------------------------ */      \
/*  Declare a type with the condition to match it               */      \
/* ------------------------------------------------------------ */      \
    struct sym##TypeCheckOpcode : TypeCheckOpcode                       \
    {                                                                   \
        sym##TypeCheckOpcode(Name_p &which):                            \
            TypeCheckOpcode(#sym, which) {}                             \
                                                                        \
        virtual Opcode *Clone()                                         \
        {                                                               \
            return new sym##TypeCheckOpcode(*this);                     \
        }                                                               \
        virtual Tree *Check(Context *context, Tree *what)               \
        {                                                               \
            return what->As<sym##_r>(context);                          \
        }                                                               \
    };                                                                  \
                                                                        \
    Name_p sym##_type;                                                  \
    static sym##TypeCheckOpcode init_opcode_T_##sym(sym##_type);

#endif // TBL_HEADER

#endif // OPCODES_TBL
