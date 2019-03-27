#ifndef BYTECODE_H
#define BYTECODE_H
// *****************************************************************************
// bytecode.h                                                         XL project
// *****************************************************************************
//
// File description:
//
//     Implementation of a bytecode to evaluate XL programs faster
//     without the need to generate machine code directly
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2015-2019, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can r redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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
//
//   All bytecode operations are represented by an instance of the 'Op'
//   class, which has a virtual member 'Run' taking a 'Data' argument.
//
//   The 'Data' argument is an array of Tree_p represented by a pointer.
//   Local values are stored at positive offsets, input arguments and
//   closure data at negative offsets.
//    [0]       : Self / result
//    [1]       : Evaluation context (more precisely, the Scope for it)
//    [2..N]    : Local variables, temporaries, output slots
//    [-1..-M]  : Input arguments, captured values (closure)

#include "tree.h"
#include "context.h"
#include "evaluator.h"

#include <vector>
#include <map>
#include <iostream>

XL_BEGIN

struct Op;                     // An individual operation
struct Code;                   // A sequence of operations
struct Procedure;              // Internal representation of functions
struct CallOp;                 // A call operation
struct CodeBuilder;            // Code generator
typedef std::vector<Op *> Ops; // Sequence of operations
typedef std::map<Tree *, int>  TreeIDs;
typedef std::map<Tree *, Op *> TreeOps;
typedef std::vector<int>       ParmOrder;
typedef Tree_p *               Data;



// ============================================================================
//
//     Main entry point
//
// ============================================================================

class Bytecode : public Evaluator
// ----------------------------------------------------------------------------
//   An evaluation based on some intermediate byte code
// ----------------------------------------------------------------------------
{
public:
    Bytecode();
    ~Bytecode();

    Tree *              Evaluate(Scope *, Tree *input) override;
    Tree *              TypeCheck(Scope *, Tree *type, Tree *val) override;
    bool                TypeAnalysis(Scope *, Tree *input) override;

public:
    static Procedure *  Compile(Context *context,
                                Tree *input, Tree *type,
                                TreeIDs &parms, TreeList &captured);
};


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
public:
    Op *                success;
public:
    // The action to perform
    virtual Op *        Run(Data data XL_UNUSED) { return success; }

public:
                        Op()                    : success(NULL) {}
    virtual             ~Op()                   {}
    virtual Op *        Fail()                  { return NULL; }
    virtual void        Dump(std::ostream &out) { out << OpID(); }
    virtual kstring     OpID()                  { return "op"; }
};


struct Code : public Op, Info
// ----------------------------------------------------------------------------
//    A sequence of operations (may be local evaluation code in a function)
// ----------------------------------------------------------------------------
{
    Context_p           context;
    Tree_p              self;
    Op *                ops;
    Ops                 instrs;
public:
    Code(Context *, Tree *self);
    Code(Context *, Tree *self, Op *instr);
    ~Code();

    virtual Op *        Run(Data data);

    void                SetOps(Op **ops, Ops *instr, uint outId);
    virtual void        Dump(std::ostream &out);
    static void         Dump(std::ostream &out, Op *ops, Ops &instrs);
    static text         Ref(Op *op, text sep, text set, text null);
    virtual uint        Inputs()        { return 0; }
    virtual uint        Locals()        { return 0; }
    virtual kstring     OpID()          { return "code"; }
};


struct Procedure : Code
// ----------------------------------------------------------------------------
//   A sequence of operations with its own scope
// ----------------------------------------------------------------------------
{
    uint                nInputs, nLocals;
    TreeList            captured;
public:
    Procedure(Context *context, Tree *self, uint nInputs, uint nLocals);
    Procedure(Procedure *original, Data data, ParmOrder &capture);
    ~Procedure();

    virtual Op *        Run(Data data);
    virtual void        Dump(std::ostream &out);
    virtual uint        Inputs()        { return nInputs; }
    virtual uint        Locals()        { return nLocals; }
    virtual kstring     OpID()          { return "function"; }

    uint                Closures()      { return captured.size(); }
    Tree_p *            ClosureData()   { return &captured[0]; }
    uint                OffsetSize()    { return Inputs() + Closures(); }
    uint                FrameSize()     { return 2 + OffsetSize() + Locals(); }
};



// ============================================================================
//
//    Build code from the input
//
// ============================================================================

struct CodeBuilder
// ----------------------------------------------------------------------------
//    Build the bytecode to rapidly evaluate a tree
// ----------------------------------------------------------------------------
{
public:
    CodeBuilder(TreeList &captured);
    ~CodeBuilder();

public:
    Procedure * Compile(Context *context,Tree *tree,TreeIDs &parms,Tree *type);
    Op *        CompileInternal(Context *context, Tree *what, bool defer);
    bool        Instructions(Context *context, Tree *tree);

public:
    // Tree::Do interface
    enum strength { NEVER, SOMETIMES, ALWAYS };
    typedef strength value_type;

    strength    DoInteger(Integer *what);
    strength    DoReal(Real *what);
    strength    DoText(Text *what);
    strength    DoName(Name *what);
    strength    DoPrefix(Prefix *what);
    strength    DoPostfix(Postfix *what);
    strength    DoInfix(Infix *what);
    strength    DoBlock(Block *what);
    strength    DoLeftRight(Tree *wl, Tree *wr, Tree *l, Tree *r);

    // Evaluation and binding of values
    int         ValueID(Tree *);
    int         CaptureID(Tree *);
    int         Evaluate(Context *, Tree *, bool deferEval = false);
    int         EvaluationTemporary(Tree *);
    void        Enclose(Context *context, Scope *old, Tree *what);
    int         Bind(Name *name, Tree *value, Tree *type=NULL);
    CallOp *    Call(Context *context, Tree *value, Tree *type,
                     TreeIDs &inputs, ParmOrder &parms);

    // Contexts management
    enum depth  { LOCAL, PARAMETER, ENCLOSING, GLOBAL };
    depth       ScopeDepth(Scope *scope);

    // Adding an opcode
    void        Add(Op *op);
    void        AddEval(int id, Op *op);
    void        AddTypeCheck(Context *, Tree *value, Tree *type);

    // Success at end of declaration
    void        Success();
    void        InstructionsSuccess(uint oldNumEvals);

public:
    Op *        ops;            // List of operations to evaluate that tree
    Op **       lastOp;         // Last operation being written to
    TreeIDs     inputs;         // Input arguments
    TreeIDs     outputs;        // Output arguments
    TreeIDs     values;         // Local variables and evaluation temporaries
    TreeList   &captured;       // Captured values from enclosing contexts
    uint        nEvals;         // Max number of evals on all candidates
    uint        nParms;         // Max number of parms on all candidates
    uint        candidates;     // Number of candidates found
    Tree_p      test;           // Current form to test
    Tree_p      resultType;     // Result type declared in rewrite
    Context_p   context;        // Evaluation context
    Context_p   parmsCtx;       // Parameter declarations for current function
    Context_p   argsCtx;        // Argument declarations for called functions
    Op *        failOp;         // Exit instruction if evaluation fails
    Op *        successOp;      // Exit instruction in case of success
    Ops         instrs;         // All instructions
    TreeOps     subexprs;       // Code generated for sub-expressions
    ParmOrder   parms;          // Indices for parameters
    bool        defer;          // Deferred evaluation
};



// ============================================================================
//
//    Functions returning specific items in a data scope
//
// ============================================================================

inline Tree *DataSelf(Data data)
// ----------------------------------------------------------------------------
//   Return the 'self' input argument in the input data
// ----------------------------------------------------------------------------
{
    return data[0];
}


inline Tree *DataResult(Data data)
// ----------------------------------------------------------------------------
//   Update the result
// ----------------------------------------------------------------------------
{
    return data[0];
}


inline void DataResult(Data data, Tree *result)
// ----------------------------------------------------------------------------
//   Update the result
// ----------------------------------------------------------------------------
{
    data[0] = result;
}


inline Scope *DataScope(Data data)
// ----------------------------------------------------------------------------
//   Return the scope in the input data
// ----------------------------------------------------------------------------
{
    return data[1]->As<Scope>();
}


inline void DataScope(Data data, Scope *scope)
// ----------------------------------------------------------------------------
//   Set the scope in the input data
// ----------------------------------------------------------------------------
{
    data[1] = scope;
}


inline Tree *DataArg(Data data, uint index)
// ----------------------------------------------------------------------------
//    Return the Nth input argument
// ----------------------------------------------------------------------------
{
    return data[~index];
}


inline Tree *DataValue(Data data, uint index)
// ----------------------------------------------------------------------------
//    Return the Nth input argument
// ----------------------------------------------------------------------------
{
    return data[2+index];
}


inline void DataValue(Data data, uint index, Tree *value)
// ----------------------------------------------------------------------------
//    Return the Nth input argument
// ----------------------------------------------------------------------------
{
    data[2+index] = value;
}



// ============================================================================
//
//    Streaming operators
//
// ============================================================================

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


inline std::ostream &operator<<(std::ostream &out, Ops &instrs)
// ----------------------------------------------------------------------------
//   Dump all the opcodes in a sequence
// ----------------------------------------------------------------------------
{
    Code::Dump(out, NULL, instrs);
    return out;
}

XL_END

#endif // BYTECODE_H
