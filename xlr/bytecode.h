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
#include <map>
#include <iostream>

XL_BEGIN


struct Op;                      // An individual operation
struct Code;                    // A sequence of operations
struct Data;                    // Data on which the code operates
struct CodeBuilder;             // Code generator



// ============================================================================
// 
//     Main entry point
// 
// ============================================================================

Tree *EvaluateWithBytecode(Context *context, Tree *input);



// ============================================================================
// 
//    Code representation
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
        ARITY_SELF,             // Pass self as argument
        ARITY_OP,               // Pass op as argument
        ARITY_DATA,             // Pass data context
        ARITY_OPDATA            // Pass data and op
    };

    // Implementation for various arities
    typedef Tree *(*arity_none_fn)();
    typedef Tree *(*arity_one_fn)(Tree *);
    typedef Tree *(*arity_two_fn)(Tree *, Tree *);
    typedef Tree *(*arity_ctxone_fn)(Context *, Tree *);
    typedef Tree *(*arity_ctxtwo_fn)(Context *, Tree *, Tree *);
    typedef Tree *(*arity_function_fn)(TreeList &args);
    typedef Tree *(*arity_self_fn)();
    typedef Tree *(*arity_op_fn)(Op *);
    typedef Tree *(*arity_data_fn)(Data &);
    typedef Op   *(*arity_opdata_fn)(Op *, Data &);

public:
    Op(kstring name, arity_none_fn fn, Arity arity)
        : arity_none(fn), name(name), success(), arity(arity) {}
    Op(kstring name, arity_none_fn fn)
        : arity_none(fn), name(name), success(), arity(ARITY_NONE) {}
    Op(kstring name, arity_one_fn fn)
        : arity_one(fn), name(name), success(), arity(ARITY_ONE) {}
    Op(kstring name, arity_two_fn fn)
        : arity_two(fn), name(name), success(), arity(ARITY_TWO) {}
    Op(kstring name, arity_ctxone_fn fn)
        : arity_ctxone(fn), name(name), success(), arity(ARITY_CONTEXT_ONE) {}
    Op(kstring name, arity_ctxtwo_fn fn)
        : arity_ctxtwo(fn), name(name), success(), arity(ARITY_CONTEXT_TWO) {}
    Op(kstring name, arity_function_fn fn)
        : arity_function(fn), name(name), success(), arity(ARITY_FUNCTION) {}
    Op(kstring name, arity_op_fn fn)
        : arity_op(fn), name(name), success(), arity(ARITY_OP) {}
    Op(kstring name, arity_data_fn fn)
        : arity_data(fn), name(name), success(), arity(ARITY_DATA) {}
    Op(kstring name, arity_opdata_fn fn)
        : arity_opdata(fn), name(name), success(), arity(ARITY_OPDATA) {}
    Op(kstring name)
        : arity_none(NULL), name(name), success(), arity(ARITY_SELF) {}
    Op(const Op &o)
        : arity_none(o.arity_none), name(o.name), success(), arity(o.arity) {}

    virtual             ~Op()                   {}
    virtual Op *        Fail()                  { return NULL; }
    virtual void        Dump(std::ostream &out) { out << name; }

    Op *                Run(Data &data);
    Tree *              Run(Context *context, Tree *self, TreeList &args);
    static void         Delete(Op *);

    
public:
    // Thte 
    union
    {
        arity_none_fn           arity_none;
        arity_one_fn            arity_one;
        arity_two_fn            arity_two;
        arity_ctxone_fn         arity_ctxone;
        arity_ctxtwo_fn         arity_ctxtwo;
        arity_function_fn       arity_function;
        arity_self_fn           arity_self;
        arity_op_fn             arity_op;
        arity_data_fn           arity_data;
        arity_opdata_fn         arity_opdata;
    };
    kstring                     name;
    Op *                        success;
    Arity                       arity;
};


inline std::ostream &operator<<(std::ostream &out, Op *op)
// ----------------------------------------------------------------------------
//   Dump one specific opcode
// ----------------------------------------------------------------------------
{
    if (op)
        op->Dump(out);
    else
        out << "NULL";
    return out;
}


inline std::ostream &operator<<(std::ostream &out, Op &ops)
// ----------------------------------------------------------------------------
//   Dump all the opcodes in a sequence
// ----------------------------------------------------------------------------
{
    Op *op = &ops;
    while (op)
    {
        op->Dump(out);
        op = op->success;
        if (op)
            out << "\n";
    }
    return out;
}


struct Code : Info, Op
// ----------------------------------------------------------------------------
//    A sequence of operations
// ----------------------------------------------------------------------------
{
    Code(Context *, Tree *self, uint nArgs);
    Code(Context *, Tree *self, Op *instr, uint nArgs = 0);
    ~Code();
    
    void                SetOps(Op **ops);
    static Op *         runCode(Op *op, Data &data);
    static Op *         runCodeWithScope(Op *op, Data &data);
    virtual void        Dump(std::ostream &out);

public:
    Context_p           context;
    Tree_p              self;
    Op *                ops;
    uint                nArgs, nVars, nEvals, nParms;
};


struct Data
// ----------------------------------------------------------------------------
//    The data on which a given code operates
// ----------------------------------------------------------------------------
{
    Data(Context *context, Tree *self, TreeList &args)
        : context(context), self(self), args(&args[0]),
          result(NULL), left(NULL),
          values(NULL), vars(NULL), parms(NULL),
          nArgs(args.size()), nValues(0), nVars(0), nParms(0) {}
    Data(Context *context, Tree *self, Tree_p *args, uint nArgs)
        : context(context), self(self), args(args),
          result(NULL), left(NULL),
          values(NULL), vars(NULL), parms(NULL),
          nArgs(nArgs), nValues(0), nVars(0), nParms(0) {}
    ~Data()             { delete[] values; }

    void Allocate(uint nvars, uint nevals, uint nparms)
    {
        XL_ASSERT(!values);

        this->values = new Tree_p[nvars + nevals + nparms];
        this->vars = values + nevals;
        this->parms = vars + nvars;
        
        this->nValues = nevals;
        this->nVars   = nvars;
        this->nParms  = nparms;
    }

    Tree *Arg(uint id)          { XL_ASSERT(id<nArgs);   return args[id];     }
    Tree *Arg(uint id, Tree *v) { XL_ASSERT(id<nArgs);   return args[id]  =v; }
    Tree *Value(uint id)        { XL_ASSERT(id<nValues); return values[id];   }
    Tree *Value(uint id,Tree *v){ XL_ASSERT(id<nValues); return values[id]=v; }
    Tree *Var(uint id)          { XL_ASSERT(id<nVars);   return vars[id];   }
    Tree *Var(uint id,Tree *v)  { XL_ASSERT(id<nVars);   return vars[id]=v; }
    Tree *Parm(uint id)         { XL_ASSERT(id<nParms);  return parms[id];    }
    Tree *Parm(uint id,Tree *v) { XL_ASSERT(id<nParms);  return parms[id] =v; }

public:
    // Input to evaluation
    Context_p   context;        // Context of evaluation
    Tree_p      self;           // What we are evaluating
    Tree_p *    args;           // Input arguments

    // Generated during evaluation
    Tree_p      result;         // Result of expression
    Tree_p      left;           // Left operand for two-operand operations
    Tree_p *    values;         // Values (evals, locals, parms)
    Tree_p *    vars;           // Pointer to local values
    Tree_p *    parms;          // Pointer to local values

    // Size of the different frames
    uint        nArgs;
    uint        nValues;
    uint        nVars;
    uint        nParms;
};


struct CodeBuilder
// ----------------------------------------------------------------------------
//    Build the bytecode to rapidly evaluate a tree
// ----------------------------------------------------------------------------
{
    CodeBuilder();
    ~CodeBuilder();

public:
    Code *      Compile(Context *context, Tree *tree, uint nArgs = 0);
    bool        Instructions(Context *context, Tree *tree);

public:
    // Tree::Do interface
    typedef bool value_type;

    bool        DoInteger(Integer *what);
    bool        DoReal(Real *what);
    bool        DoText(Text *what);
    bool        DoName(Name *what);
    bool        DoPrefix(Prefix *what);
    bool        DoPostfix(Postfix *what);
    bool        DoInfix(Infix *what);
    bool        DoBlock(Block *what);

    // Evaluation and binding of values
    uint        EvaluationID(Tree *);
    uint        Evaluate(Context *, Tree *,bool saveLeft=false);
    uint        EvaluationTemporary(Tree *);
    void        Enclose(Context *context, Scope *old, Tree *what);
    uint        Bind(Name *name, Tree *value, bool closure);
    uint        Reference(Tree *tree, Infix *decl);

    // Adding an opcode
    void        Add(Op *op);

    // Success at end of declaration
    void        Success();


public:
    typedef std::map<Tree *, uint> TreeIndices;

    Op *        ops;            // List of operations to evaluate that tree
    Op **       lastOp;         // Last operation being written to
    TreeIndices parms;          // Function's parameters
    TreeIndices variables;      // Local variables
    TreeIndices evals;          // Evaluation of arguments

    uint        candidates;     // Number of candidates found
    Tree_p      test;           // Current form to test
    Tree_p      resultType;     // Result type declared in rewrite
    Context_p   context;        // Evaluation context
    Context_p   locals;         // Local parameter declarations
    Op *        failOp;         // Exit instruction if evaluation fails
    Op *        successOp;      // Exit instruction in case of success
};



// ============================================================================
// 
//    Evaluation of opcodes
// 
// ============================================================================

inline Tree *Op::Run(Context *context, Tree *self, TreeList &args)
// ----------------------------------------------------------------------------
//   Run a single opcode
// ----------------------------------------------------------------------------
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

    case ARITY_OP:
        return arity_op(this);

    case ARITY_DATA:
    {
        Data data(context, self, args);
        return arity_data(data);
    }
    case ARITY_OPDATA:
    {
        Data data(context, self, args);
        Op *next = arity_opdata(this, data);
        while (next)
            next = next->Run(data);
        return data.result;
    }
    }
    return NULL;
}


inline Op *Op::Run(Data &data)
// ----------------------------------------------------------------------------
//   Evaluate the instruction in a code sequence
// ----------------------------------------------------------------------------
{
    Tree *result = data.result;
    switch(arity)
    {
    case ARITY_NONE:
        result = arity_none();
        break;
    case ARITY_ONE:
        result = arity_one(result);
        break;
    case ARITY_TWO:
        result = arity_two(data.left, result);
        break;
    case ARITY_CONTEXT_ONE:
        result = arity_ctxone(data.context, result);
        break;
    case ARITY_CONTEXT_TWO:
        result = arity_ctxtwo(data.context, data.left, result);
        break;
    case ARITY_FUNCTION:
    {
        TreeList parms(data.parms, data.parms + data.nParms);
        result = arity_function(parms);
        break;
    }
    case ARITY_SELF:
        result = data.self;
        break;
    case ARITY_OP:
        result = arity_op(this);
        break;
    case ARITY_DATA:
        result = arity_data(data);
        break;
    case ARITY_OPDATA:
        return arity_opdata(this, data);
    }

    // If the evaluation did not work, go to the fail opcode
    if (!result)
        return Fail();

    data.result = result;
    return success;
}


inline void Op::Delete(Op *op)
// ----------------------------------------------------------------------------
//    Delete a sequence of ops
// ----------------------------------------------------------------------------
{
    while (op)
    {
        Op *next = op->success;
        delete op;
        op = next;
    }
}



XL_END

#endif // BYTECODE_H
