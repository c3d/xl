#ifndef BYTECODE_H
#define BYTECODE_H
// ****************************************************************************
//  bytecode.h                                                     XLR project 
// ****************************************************************************
// 
//   File Description:
// 
//     Representation of a bytecode for XLR
// 
// 
// 
// 
// 
// 
// 
// 
// ****************************************************************************
//  (C) 2015 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2015 Taodyne SAS
// ****************************************************************************

#include "tree.h"
#include "context.h"

#include <vector>

XL_BEGIN

// ============================================================================
// 
//     Main entry point
// 
// ============================================================================

struct Op
// ----------------------------------------------------------------------------
//   An individual operation
// ----------------------------------------------------------------------------
{
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
    Op(kstring name, arity_none_fn fn, Arity arity)
        : arity_none(fn), name(name), arity(arity) {}
    Op(kstring name, arity_none_fn fn)
        : arity_none(fn), name(name), arity(ARITY_NONE) {}
    Op(kstring name, arity_one_fn fn)
        : arity_one(fn), name(name), arity(ARITY_ONE) {}
    Op(kstring name, arity_two_fn fn)
        : arity_two(fn), name(name), arity(ARITY_TWO) {}
    Op(kstring name, arity_ctxone_fn fn)
        : arity_ctxone(fn), name(name), arity(ARITY_CONTEXT_ONE) {}
    Op(kstring name, arity_ctxtwo_fn fn)
        : arity_ctxtwo(fn), name(name), arity(ARITY_CONTEXT_TWO) {}
    Op(kstring name, arity_function_fn fn)
        : arity_function(fn), name(name), arity(ARITY_FUNCTION) {}
    Op(kstring name)
        : arity_none(NULL), name(name), arity(ARITY_SELF) {}

    Tree *Run(Context *context, Tree *self, TreeList &args)
    {
        uint size = args.size();
        switch(arity)
        {
        case ARITY_NONE:
            if (size == 0)
                return arity_none();
            break;
        case ARITY_ONE:
            if (size == 1)
                return arity_one(args[0]);
            break;
        case ARITY_TWO:
            if (size == 2)
                return arity_two(args[0], args[1]);
            break;
        case ARITY_CONTEXT_ONE:
            if (size == 1)
                return arity_ctxone(context, args[0]);
            break;
        case ARITY_CONTEXT_TWO:
            if (size == 2)
                return arity_ctxtwo(context, args[0], args[1]);
            break;
        case ARITY_FUNCTION:
            return arity_function(args);

        case ARITY_SELF:
            return self;
        }
        return NULL;
    }

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
};
typedef std::vector<Op *> Ops;


struct Code
// ----------------------------------------------------------------------------
//    The bytecode to rapidly evaluate a tree
// ----------------------------------------------------------------------------
{
    Code() {}
        
    uint        parms;          // Parameters
    uint        locals;         // Local variables
    uint        upvalues;       // 
};

XL_END

#endif // BYTECODE_H
