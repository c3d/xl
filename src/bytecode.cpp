// *****************************************************************************
// bytecode.cpp                                                       XL project
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

#include "bytecode.h"
#include "interpreter.h"
#include "save.h"
#include "errors.h"
#include "basics.h"
#include "runtime.h"

#include <algorithm>
#include <sstream>


RECORDER(bytecode_output, 64, "Output of the bytecode generator");
RECORDER(bytecode, 64, "Byte code generation");


XL_BEGIN

// ============================================================================
//
//    Main entry point
//
// ============================================================================

Tree *Bytecode::Evaluate(Scope *scope, Tree *what)
// ----------------------------------------------------------------------------
//   Compile bytecode and then evaluate it
// ----------------------------------------------------------------------------
{
    TreeIDs  noParms;
    TreeList captured;
    Context_p context = new Context(scope);

    Procedure *proc = Compile(context, what, NULL, noParms, captured);
    Tree_p result = what;
    if (proc)
    {
        XL_ASSERT(proc->Inputs() == 0);
        uint size = captured.size();
        captured.push_back(what);
        captured.push_back(scope);
        Data data = &captured[size];
        Op *op = proc;
        while(op)
            op = op->Run(data);
        result = DataResult(data);
    }
    return result;
}


Tree * Bytecode::TypeCheck(Scope *scope, Tree *type, Tree *val)
// ----------------------------------------------------------------------------
//   Perform a type check for the given value
// ----------------------------------------------------------------------------
{
    return val;
}


bool Bytecode::TypeAnalysis(Scope *scope, Tree *input)
// ----------------------------------------------------------------------------
//    No type analysis for the bytecode analyzer
// ----------------------------------------------------------------------------
{
    return true;
}


Procedure *Bytecode::Compile(Context *ctx, Tree *what, Tree *type,
                             TreeIDs &parms, TreeList &captured)
// ----------------------------------------------------------------------------
//    Compile a tree to bytecode
// ----------------------------------------------------------------------------
{
    CodeBuilder builder(captured);
    Procedure *proc = builder.Compile(ctx, what, parms, type);
    return proc;
}



// ============================================================================
//
//    Opcodes we use in this translation
//
// ============================================================================

struct FailOp : Op
// ----------------------------------------------------------------------------
//   Any op that has a fail exit
// ----------------------------------------------------------------------------
{
    FailOp(Op *fx): Op(), fail(fx) {}
    Op *fail;
    virtual Op *        Fail()  { return fail; }
    virtual kstring     OpID()  { return "fail"; }
};


struct LabelOp : Op
// ----------------------------------------------------------------------------
//    The target of a jump
// ----------------------------------------------------------------------------
{
    LabelOp(kstring name): name(name) {}
    virtual kstring     OpID()  { return name; }
    virtual void        Dump(std::ostream &out)
    {
        out << OpID() << "\t" << (void *) this;
    }
    kstring name;
};


struct ConstOp : Op
// ----------------------------------------------------------------------------
//   Evaluates a constant
// ----------------------------------------------------------------------------
{
    ConstOp(Tree *value): value(value) {}
    Tree_p value;

    virtual Op *        Run(Data data)
    {
        DataResult(data, value);
        return success;
    }
    virtual kstring     OpID()  { return "const"; }
    virtual void        Dump(std::ostream &out)
    {
        kstring kinds[] = { "integer", "real", "text", "name",
                            "block", "prefix", "postfix", "infix" };
        out << OpID() << "\t" << kinds[value->Kind()] << "\t" << value;
    }
};


struct SelfOp : Op
// ----------------------------------------------------------------------------
//   Evaluate self
// ----------------------------------------------------------------------------
{
    virtual kstring     OpID()  { return "self"; }
};


struct EvalOp : FailOp
// ----------------------------------------------------------------------------
//    Evaluate the given tree once and only once
// ----------------------------------------------------------------------------
{
    EvalOp(int id, Op *ops, Op *fail)
        : FailOp(fail), ops(ops), id(id) {}
    Op *ops;
    int id;

    virtual Op *        Run(Data data)
    {
        Tree *result = data[id];
        if (result)
        {
            DataResult(data, result);
            return success;
        }

        // Complete evaluation of the bytecode we were given
        Op *op = ops;
        while (op)
            op = op->Run(data);

        // Save the result if evaluation was successful
        if (Tree *result = DataResult(data))
        {
            data[id] = result;
            return success;
        }

        // Otherwise, go to the fail bytecode
        return fail;
    }

    virtual kstring     OpID()          { return "eval"; }
    virtual void        Dump(std::ostream &out)
    {
        out << OpID() << "\t" << id << "\t"
            << Code::Ref(ops, "\t", "code", "null");
    }
};


struct ArgEvalOp : FailOp
// ----------------------------------------------------------------------------
//    Evaluate the given tree once and only once
// ----------------------------------------------------------------------------
{
    ArgEvalOp(Context *context, int argId, int id, Op *fail)
        : FailOp(fail), context(context), argId(argId), id(id) {}
    Context_p context;
    int       argId, id;

    virtual Op *        Run(Data data)
    {
        Tree *result = data[id];
        if (result)
        {
            DataResult(data, result);
            return success;
        }

        // Evaluate in place
        Tree *self = data[argId];
        if (Tree *inside = Interpreter::IsClosure(self, &context))
            self = inside;
        Context *ctx = context;
        if (self->IsConstant())
            result = self;
        else
            result = xl_evaluate(ctx->CurrentScope(), self);
        if (result)
        {
            result = Interpreter::MakeClosure(ctx, result);
            data[id] = result;
            return success;
        }

        // Otherwise, go to the fail bytecode
        return fail;
    }

    virtual kstring     OpID()          { return "arg"; }
    virtual void        Dump(std::ostream &out)
    {
        out << OpID() << "\t" << id << "=" << argId
            << " @" << (void *) context->CurrentScope();
    }
};


struct ClearOp : Op
// ----------------------------------------------------------------------------
//    Clear a range of eval entries after a complete evaluation
// ----------------------------------------------------------------------------
{
    ClearOp(int lo, int hi): lo(lo), hi(hi) {}
    int lo, hi;

    virtual Op *        Run(Data data)
    {
        for (int v = lo; v <= hi; v++)
            data[v] = NULL;
        return success;
    }

    virtual kstring     OpID()  { return "clear"; }
    virtual void        Dump(std::ostream &out)
    {
        out << OpID() << "\t" << lo << ".." << hi;
    }
};


struct ClosureOp : Op
// ----------------------------------------------------------------------------
//    Create a closure with the result
// ----------------------------------------------------------------------------
{
    ClosureOp(Context *ctx): context(ctx) {}
    Context_p context;

    virtual Op *        Run(Data data)
    {
        Tree *result = DataResult(data);
        result = Interpreter::MakeClosure(context, result);
        DataResult(data, result);
        return success;
    }

    virtual kstring     OpID()  { return "closure"; }
    virtual void        Dump(std::ostream &out)
    {
        out << OpID();
    }
};


struct ValueOp : Op
// ----------------------------------------------------------------------------
//    Return a tree that we know was already evaluated
// ----------------------------------------------------------------------------
{
    ValueOp(int id): id(id) {}
    int id;
    virtual Op *        Run(Data data)
    {
        DataResult(data, data[id]);
        return success;
    }
    virtual kstring     OpID()  { return "value"; }
    virtual void        Dump(std::ostream &out)
    {
        out << OpID() << "\t" << id;
    }
};


struct StoreOp : Op
// ----------------------------------------------------------------------------
//    Store result in some other ID
// ----------------------------------------------------------------------------
{
    StoreOp(int id): id(id) {}
    int id;
    virtual Op *        Run(Data data)
    {
        data[id] = DataResult(data);
        return success;
    }
    virtual kstring     OpID()  { return "store"; }
    virtual void        Dump(std::ostream &out)
    {
        out << OpID() << "\t" << id;
    }
};


struct CallOp : Op
// ----------------------------------------------------------------------------
//    Call a subroutine using the given inputs
// ----------------------------------------------------------------------------
{
    CallOp(Code *target, uint outId, ParmOrder &parms)
        : target(target), outId(outId), parms(parms) {}
    Code  *     target;
    int         outId;
    ParmOrder   parms;

    virtual Op *Run(Data data)
    {
        uint sz = parms.size();
        Data out = data + outId;

        // Copy result and scope
        DataResult(out, DataResult(data));
        DataScope (out, DataScope (data));

        // Copy all parameters
        for (uint p = 0; p < sz; p++)
        {
            int parmId = parms[p];
            out[~p] = data[parmId];
        }
        Op *remaining = target->Run(out);
        XL_ASSERT(!remaining);
        if (remaining)
            return remaining;

        DataResult(data, out[0]);
        return success;
    }

    virtual kstring     OpID()  { return "call"; }
    virtual void        Dump(std::ostream &out)
    {
        out << OpID() << "\t" << Code::Ref(target, "\t", "code", "null")
            << "\t(";
        for (uint a = 0; a < parms.size(); a++)
            out << (a ? "," : "") << parms[a];
        out << ") @ " << outId;
    }
};


struct TypeCheckOp : FailOp
// ----------------------------------------------------------------------------
//   Check if a type matches the given type
// ----------------------------------------------------------------------------
{
    TypeCheckOp(int value, int type, Op *fail)
        : FailOp(fail), value(value), type(type) {}
    int value;
    int type;

    virtual Op *        Run(Data data)
    {
        Scope *scope = DataScope(data);
        Tree *cast = xl_typecheck(scope, data[type], data[value]);
        if (!cast)
            return fail;
        DataResult(data, cast);
        return success;
    }

    virtual kstring     OpID()  { return "typechk"; }
    virtual void        Dump(std::ostream &out)
    {
        out << OpID() << "\t" << value << ":" << type;
    }
};


struct IndexOp : FailOp
// ----------------------------------------------------------------------------
//    Index operation, i.e. unknown prefix
// ----------------------------------------------------------------------------
{
    IndexOp(int left, int right, Op *fail)
        : FailOp(fail), left(left), right(right) {}
    int left, right;

    virtual Op *        Run(Data data)
    {
        Tree *callee = data[left];
        Tree *arg = data[right];
        Scope_p scope = DataScope(data);

        // If the declaration was compiled, evaluate the code
        if (Code *code = callee->GetInfo<Code>())
        {
            uint inputs = code->Inputs();
            if (inputs == 0)
            {
                // If no arguments, evaluate as new callee
                Tree_p args[2] = { data[0], data[1] };
                Op *remaining = code->Run(args);
                XL_ASSERT(!remaining);
                if (remaining)
                    return remaining;
                callee = DataResult(args);
            }
            else if (inputs == 1)
            {
                // Looks like a prefix, use it as an argument
                Tree_p args[3] = { arg, data[0], data[1] };
                Op *remaining = code->Run(&args[1]);
                XL_ASSERT(!remaining);
                if (remaining)
                    return remaining;
                return success;
            }
            else
            {
                Ooops ("Invalid prefix code for $1, had $2 arguments", callee)
                    .Arg(inputs);
                return fail;
            }
        }

        // Check if we have a closure
        Context_p context = new Context(scope);
        if (Tree *inside = Interpreter::IsClosure(callee, &context))
        {
            scope = context->CurrentScope();
            callee = inside;
            DataScope(data, scope);
        }

        // Eliminate blocks on the callee side
        while (Block *blk = callee->AsBlock())
            callee = blk->child;

        // If we have an infix on the left, check if it's a single rewrite
        if (Infix *lifx = callee->AsInfix())
        {
            // Check if we have a function definition
            if (lifx->name == "is")
            {
                // If we have a single name on the left, like (X->X+1)
                // interpret that as a lambda function
                Tree      *result = NULL;
                if (Name *lfname = lifx->left->AsName())
                {
                    // Case like '(X->X+1) Arg':
                    // Bind arg in new context and evaluate body
                    context = new Context(context);
                    context->Define(lfname, arg);
                    result = xl_evaluate(context->CurrentScope(), lifx->right);
                }
                else
                {
                    // Otherwise, enter declaration and retry, e.g.
                    // '(X,Y->X+Y) (2,3)' should evaluate as 5
                    context = new Context(context);
                    context->Define(lifx->left, lifx->right);
                    result = xl_evaluate(context->CurrentScope(), arg);
                }

                DataResult(data, result);
                return success;
            }
        }

        // Try to lookup in the callee's context
        if (Tree *found = context->Bound(arg))
        {
            DataResult(data, found);
            return success;
        }

        // Other cases: go to 'fail' exit
        return fail;
    }
};


struct FormErrorOp : Op
// ----------------------------------------------------------------------------
//   When we fail with all candidates, report an error
// ----------------------------------------------------------------------------
{
    FormErrorOp(Tree *self): self(self) {}
    Tree_p self;
    virtual Op *        Run(Data data)
    {
        Ooops("No form matches $1", self);
        return success;
    }
    virtual kstring     OpID()  { return "error"; };
};


struct PrefixFormErrorOp : FormErrorOp
// ----------------------------------------------------------------------------
//   When we fail with all prefix candidates, report an error
// ----------------------------------------------------------------------------
{
    PrefixFormErrorOp(Tree *self): FormErrorOp(self) {}
    virtual Op *        Run(Data data)
    {
        Ooops("No prefix matches $1", self);
        return success;
    }
    virtual kstring     OpID()  { return "pfxerror"; };
};



// ============================================================================
//
//   Evaluating a code sequence
//
// ============================================================================

Code::Code(Context *ctx, Tree *self)
// ----------------------------------------------------------------------------
//    Create a new code from the given ops
// ----------------------------------------------------------------------------
    : context(ctx), self(self), ops(NULL), instrs()
{}


Code::Code(Context *context, Tree *self, Op *ops)
// ----------------------------------------------------------------------------
//    Create a new code from the given ops
// ----------------------------------------------------------------------------
    : context(context), self(self), ops(ops), instrs()
{
    for (Op *op = ops; op; op = op->success)
        instrs.push_back(op);
}


Code::~Code()
// ----------------------------------------------------------------------------
//    Delete the ops we own
// ----------------------------------------------------------------------------
{
    for (Ops::iterator o = instrs.begin(); o != instrs.end(); o++)
        delete *o;
    instrs.clear();
}


void Code::SetOps(Op **newOps, Ops *instrsToTakeOver, uint outId)
// ----------------------------------------------------------------------------
//    Take over the given code
// ----------------------------------------------------------------------------
{
    ops = *newOps;
    *newOps = NULL;
    XL_ASSERT(instrs.size() == 0);
    std::swap(instrs, *instrsToTakeOver);

    // A few post-generation optimizations:
    uint max = instrs.size();
    for (uint i = 0; i < max; i++)
    {
        Op *op = instrs[i];

        // Make sure calls all use the same out area
        if (CallOp *call = dynamic_cast<CallOp *>(op))
            call->outId = outId;

        // Remove labels, since they execute as no-ops
        while (LabelOp *label = dynamic_cast<LabelOp *>(op->success))
            op->success = label->success;
        if (FailOp *fop = dynamic_cast<FailOp *>(op))
            while (LabelOp *label = dynamic_cast<LabelOp *>(fop->fail))
                fop->fail = label->success;
    }
    for (uint i = 0; i < max; i++)
    {
        Op *op = instrs[i];
        if (LabelOp *label = dynamic_cast<LabelOp *>(op))
        {
            instrs.erase(instrs.begin() + i);
            --i;
            --max;
            delete label;
        }
    }
}


Op *Code::Run(Data data)
// ----------------------------------------------------------------------------
//   Run all instructions in the sequence
// ----------------------------------------------------------------------------
{
    // Running in-place in the same context
    Scope *scope = context->CurrentScope();
    data[0] = self;
    data[1] = scope;

    // Run all instructions we have in that code
    Op *op = ops;
    while (op)
        op = op->Run(data);

    // We were successful
    return success;
}


void Code::Dump(std::ostream &out)
// ----------------------------------------------------------------------------
//   Dump all the instructions
// ----------------------------------------------------------------------------
{
    out << OpID() << "\t" << (void *) (Op *) this
        << "\tentry\t" << (void *) ops
        << "\t" << self << "\n";
    out << "\talloc"
        << "\tI" << Inputs() << " L" << Locals() << "\n";
    Dump(out, ops, instrs);
}


static Ops *currentDump = NULL;

void Code::Dump(std::ostream &out, Op *ops, Ops &instrs)
// ----------------------------------------------------------------------------
//   Dump an instruction list
// ----------------------------------------------------------------------------
{
    Save<Ops *> saveCurrentDump(currentDump, &instrs);

    uint max = instrs.size();
    for (uint i = 0; i < max; i++)
    {
        Op *op = instrs[i];
        Op *fail = op->Fail();
        if (op == ops)
            out << i << "=>\t" << op;
        else
            out << i << "\t" << op;
        if (fail)
            out << Ref(fail, "\t", "fail", "nofail");
        if (i + 1 < max)
        {
            Op *next = instrs[i]->success;
            if (next != instrs[i+1])
                out << Ref(next, "\n\t", "goto", "return");
        }
        out << "\n";
    }
}


text Code::Ref(Op *op, text sep, text set, text null)
// ----------------------------------------------------------------------------
//   Return the reference for an op in the current list
// ----------------------------------------------------------------------------
{
    std::ostringstream out;

    bool found = false;
    uint max = currentDump ? currentDump->size() : 0;

    if (op == NULL)
        found = bool(out << sep << null);
    for (uint o = 0; o < max; o++)
        if ((*currentDump)[o] == op)
            found = bool(out << sep << set << "\t#" << o);
    if (!found)
        out << sep << set << "\t" << (void *) op;
    return out.str();
}


Procedure::Procedure(Context *context, Tree *self, uint nInputs, uint nLocals)
// ----------------------------------------------------------------------------
//   Create a proc
// ----------------------------------------------------------------------------
    : Code(context, self), nInputs(nInputs), nLocals(nLocals)
{}


Procedure::Procedure(Procedure *original, Data data, ParmOrder &capture)
// ----------------------------------------------------------------------------
//    Copy constructor used for closures
// ----------------------------------------------------------------------------
    : Code(original->context, original->self),
      nInputs(original->nInputs), nLocals(original->nLocals),
      captured()
{
    // We have no instrs, so we don't "own" the instructions
    ops = original->ops;

    // Copy data in the closure from current data
    uint max = capture.size();
    captured.reserve(max);
    for (uint p = 0; p < max; p++)
    {
        int id = capture[p];
        Tree *arg = data[id];
        captured.push_back(arg);
    }
}


Procedure::~Procedure()
// ----------------------------------------------------------------------------
//    Destructor for procs
// ----------------------------------------------------------------------------
{}


Op *Procedure::Run(Data data)
// ----------------------------------------------------------------------------
//   Create a new scope and run all instructions in the sequence
// ----------------------------------------------------------------------------
{
    Scope *scope     = context->CurrentScope();
    uint   frameSize = FrameSize();
    uint   offset    = OffsetSize();
    Data   frame     = new Tree_p[frameSize];
    Data   newData   = frame + offset;

    // Initialize self and scope
    newData[0] = self;
    newData[1] = scope;

    // Copy input arguments
    uint inputs   = Inputs();
    Data oarg = &newData[-1];
    Data iarg = &data[-1];
    for (uint a = 0; a < inputs; a++)
        *oarg-- = *iarg--;

    // Copy closure data if any
    uint closures = Closures();
    if (closures)
    {
        Data carg = ClosureData();
        for (uint c = 0; c < closures; c++)
            *oarg-- = *carg++;
    }

    // Execute the following instructions in the newly created data context
    Op *op = ops;
    while (op)
        op = op->Run(newData);

    // Copy result and current context to the old data
    Tree *result = DataResult(newData);
    DataResult(data, result);

    // Destroy the frame, we no longer need it
    delete[] frame;

    // Evaluate next instruction
    return success;
}


void Procedure::Dump(std::ostream &out)
// ----------------------------------------------------------------------------
//   Dump all the instructions
// ----------------------------------------------------------------------------
{
    out << OpID() << "\t" << (void *) (Op *) this
        << "\tentry\t" << (void *) ops
        << "\t" << self << "\n";
    out << "\talloc"
        << "\tI" << Inputs() << " L" << Locals() << " C" << Closures() << "\n";
    Code::Dump(out, ops, instrs);
}



// ============================================================================
//
//   Building a code sequence and variants
//
// ============================================================================

CodeBuilder::CodeBuilder(TreeList &captured)
// ----------------------------------------------------------------------------
//   Create a code builder
// ----------------------------------------------------------------------------
    : ops(NULL), lastOp(&ops),
      inputs(), values(), captured(captured),
      nEvals(0), nParms(0), candidates(0),
      test(NULL), resultType(NULL),
      context(NULL), parmsCtx(NULL), argsCtx(NULL),
      failOp(NULL), successOp(NULL),
      instrs(), subexprs(), parms(), defer(false)
{}


CodeBuilder::~CodeBuilder()
// ----------------------------------------------------------------------------
//    Delete all instructions if the list was not transferred
// ----------------------------------------------------------------------------
{
    for (Ops::iterator o = instrs.begin(); o != instrs.end(); o++)
        delete *o;
    instrs.clear();
}


CodeBuilder::depth CodeBuilder::ScopeDepth(Scope *scope)
// ----------------------------------------------------------------------------
//   Return true if the given scope is local to the current proc
// ----------------------------------------------------------------------------
{
    Scope *parmsScope = parmsCtx->CurrentScope();
    Scope *globalScope = MAIN->context.CurrentScope();
    uint count = 0;

    // Loop on scope, looking for either current parm scope or global scope
    while (scope)
    {
        if (scope == parmsScope)
            return count ? LOCAL : PARAMETER;
        if (scope == globalScope)
            return count ? ENCLOSING : GLOBAL;
        scope = ScopeParent(scope);
        count++;
    }

    // If we reached the end without seeing the current global scope,
    // this means this is a global scope (probably some other file)
    return GLOBAL;
}


void CodeBuilder::Add(Op *op)
// ----------------------------------------------------------------------------
//    Add an instruction in the generated code
// ----------------------------------------------------------------------------
{
    XL_ASSERT(count(instrs.begin(), instrs.end(), op) == 0);
    instrs.push_back(op);
    *lastOp = op;
    lastOp = &op->success;
    XL_ASSERT(!op->success && "Adding an instruction that has kids");
}


void CodeBuilder::AddEval(int id, Op *op)
// ----------------------------------------------------------------------------
//    Add an evaluation in the generated code, simplify if 'Quick'
// ----------------------------------------------------------------------------
{
    Add(new EvalOp(id, op, failOp));
}


void CodeBuilder::AddTypeCheck(Context *context, Tree *what, Tree *type)
// ----------------------------------------------------------------------------
//   Add type check if necessary
// ----------------------------------------------------------------------------
{
    if (type)
        if (Name *name = type->AsName())
            if (Tree *original = context->Bound(name))
                type = original;
    if (resultType)
        if (Name *name = resultType->AsName())
            if (Tree *original = context->Bound(name))
                resultType = original;

    // Unify form for 'catch-all' type
    if (!type)
        type = tree_type;
    if (!resultType)
        resultType = tree_type;
    if (resultType == tree_type)
        return;

    // Check if we have some static match
    if (what->IsConstant())
        if (xl_typecheck(context->CurrentScope(), type, what))
            return;

    // Check if types match statically
    if (type == resultType)
        return;

    // Otherwise, we need to generate a dynamic match
    int valueID = ValueID(what);
    int typeID = Evaluate(context, type);
    Add(new TypeCheckOp(valueID, typeID, failOp));
}


void CodeBuilder::Success()
// ----------------------------------------------------------------------------
//    Success at the end of a declaration
// ----------------------------------------------------------------------------
{
    XL_ASSERT(successOp && failOp);
    XL_ASSERT(count(instrs.begin(), instrs.end(), failOp) == 0);

    record(bytecode, "SUCCESS: %O FAIL: %O", successOp, failOp);

    // End current stream to the success exit, restart code gen at failure exit
    *lastOp = successOp;
    XL_ASSERT(failOp && "Success without a failure exit");

    lastOp = &failOp->success;
    instrs.push_back(failOp);
    failOp = NULL;

    XL_ASSERT(!*lastOp);
}


void CodeBuilder::InstructionsSuccess(uint neOld)
// ----------------------------------------------------------------------------
//    At end of an instruction, mark success by recording number of evals
// ----------------------------------------------------------------------------
{
    uint ne = values.size();
    if (nEvals < ne)
        nEvals = ne;
    if (ne > neOld)
        Add(new ClearOp(neOld+2, ne+1));
    if (defer)
        Add(new ClosureOp(context));
}



// ============================================================================
//
//    Compilation of the tree
//
// ============================================================================
//
//  The 'Instructions' function evaluates candidates in the symbol table.
//  Each specific declaration causes an invokation of compileLookup.
//
//  Consider the following code:
//
//      foo X:integer, Y:integer -> foo1
//      foo A:integer, B         -> foo2
//      foo U, V
//      write "Toto"
//
//  The generated code will look like this
//      ;; Evaluate foo1 candidate
//
//      ;; Match U against X:integer
//      Evaluate U              or goto fail1.1         (EvalOp)
//      Evaluate integer        or goto fail1.1         (EvalOp)
//      TypeCheck U : integer   or goto fail1.1         (TypeCheckOp)
//
//      ;; Match V against Y:integer
//      Evaluate V              or goto fail1.1         (EvalOp)
//      Evaluate integer        or goto fail1.1         (EvalOp - SaveLeft)
//      TypeCheck V : integer   or goto fail1.1         (TypeCheckOp)
//
//      ;; Call foo1
//      Call foo1 (id(U), id(V)) @ parmSize             (CallOp)
//
//      ;; Done, successful evaluation, goto is in 'success' field
//      goto success1
//
//    fail1.1:                                          (LabelOp)
//      Evaluate U              or goto fail1.2         (EvalOp)
//      Evaluate integer        or goto fail1.2         (EvalOp)
//      ...
//      Call foo2                                       (CallOp)
//      goto success1
//
//    fail1.2:                                          (LabelOp)
//      FormError                                       (FormErrorOp)
//
//    success1:                                         (LabelOp)
//      ;; Same thing for write "Toto"
//
//  The 'goto' in the above are implicit, marked by 'success' or 'fail' in Op.

static Tree *compileLookup(Scope *evalScope, Scope *declScope,
                           Tree *self, Infix *decl, void *cb)
// ----------------------------------------------------------------------------
//    Lookup a given declaration and generate code for it
// ----------------------------------------------------------------------------
{
    static uint  depth = 0;
    Save<uint>   saveDepth(depth, depth+1);

    CodeBuilder *builder = (CodeBuilder *) cb;
    uint         cindex = builder->candidates++;

    record(bytecode,
           "Compile %d:%d (%t) from %t",
           depth, cindex, self, decl->left);

    // Create the scope for evaluation
    Context_p    context = new Context(evalScope);
    Context_p    argsCtx = NULL;

    // Save current state of the builder
    Save<Tree_p>    saveSelf(builder->test, self);
    Save<Context_p> saveContext(builder->context, context);
    Save<Context_p> saveArgsCtx(builder->argsCtx, argsCtx);

    // Create the exit point for failed evaluation
    Op *         oldFailOp = builder->failOp;
    Op *         failOp = new LabelOp("fail");
    builder->failOp = failOp;
    builder->resultType = NULL;

    // We start with new parameters for each candidate
    ParmOrder           noParms;
    TreeIDs             empty;
    Save<ParmOrder>     saveParmOrder(builder->parms, noParms);
    Save<TreeIDs>       saveOutputs(builder->outputs, empty);

    // If we lookup a name or a number, just return it
    Tree *defined = RewriteDefined(decl->left);
    bool isLeaf = defined->IsLeaf();
    CodeBuilder::strength strength = CodeBuilder::ALWAYS;
    if (isLeaf)
    {
        if (defined != xl_self && !Tree::Equal(defined, self))
        {
            record(bytecode,
                   "Compile %d:%d (%t) from constant %t - MISMATCH",
                   depth, cindex, self, decl->left);
            builder->failOp = oldFailOp;
            delete failOp;
            return NULL;
        }
        argsCtx = context;
    }
    else
    {
        // Create the scope for binding the parameters
        argsCtx = new Context(declScope);
        argsCtx->CreateScope();
        builder->argsCtx = argsCtx;

        // Remember the old end in case we did not generate code
        Op **lastOp = builder->lastOp;
        uint lastInstrSize = builder->instrs.size();

        // Check bindings of arguments to declaration, exit if fails
        strength = decl->left->Do(builder);
        if (strength == CodeBuilder::NEVER)
        {
            record(bytecode, "Compile %d:%d (%t) from %t - MISMATCH",
                   depth, cindex, self, decl->left);

            // Remove the instructions that were added and the failed exit
            *lastOp = NULL;
            uint lastNow = builder->instrs.size();
            for (uint i = lastInstrSize; i < lastNow; i++)
                delete builder->instrs[i];
            builder->instrs.resize(lastInstrSize);
            builder->failOp = oldFailOp;
            builder->lastOp = lastOp;
            delete failOp;
            return NULL;
        }
        if (builder->resultType == tree_type)
            builder->resultType = NULL;
    }

    // Check if we have builtins (opcode or C bindings)
    if (decl->right == xl_self)
    {
        // If the right is "self", just return the input
        record(bytecode,
               "Compile %d:%d (%t) from %t SELF",
               depth, cindex, self, decl->left);
        builder->Add(new SelfOp);
    }
    else if (Opcode *opcode = Interpreter::OpcodeInfo(decl))
    {
        // Cached callback - Make a copy
        XL_ASSERT(!opcode->success);
        Opcode *clone = opcode->Clone();
        clone->SetParms(builder->parms);
        builder->Add(clone);
        record(bytecode,
               "Compile %d:%d (%t) OPCODE %O SELF",
               depth, cindex, self, (Op *) opcode);
    }
    else if (isLeaf)
    {
        // Assign an ID for names
        builder->Evaluate(context, defined);
    }
    else
    {
        // Normal case: evaluate body of the declaration in the new context
        CallOp *call = builder->Call(argsCtx, decl->right,
                                     builder->resultType,
                                     builder->outputs, builder->parms);
        builder->Add(call);
    }

    // Successful evaluation
    builder->Success();

    // Keep looking for other declarations
    record(bytecode, "Compile %d:%d (%t) SUCCESS",
           depth, cindex, self);

    if (strength == CodeBuilder::ALWAYS)
        return decl;
    return NULL;
}


Procedure *CodeBuilder::Compile(Context *ctx, Tree *what,
                               TreeIDs &callArgs, Tree *type)
// ----------------------------------------------------------------------------
//    Compile the tree
// ----------------------------------------------------------------------------
{
    // Check if we already compiled this particular tree (possibly recursive)
    Procedure *proc = what->GetInfo<Procedure>();
    if (proc)
    {
        captured = proc->captured;
        return proc;
    }

    // Does not exist yet, set it up
    Save<Context_p> saveParmsCtx(parmsCtx, ctx);
    uint nArgs = callArgs.size();
    Save<TreeIDs> saveInputs(inputs, callArgs);
    proc = new Procedure(ctx, what, nArgs, 0);
    what->SetInfo<Code>(proc);

    // Evaluate the input code
    bool result = true;
    Errors *errors = MAIN->errors;
    uint errCount = errors->Count();
    if (ctx->ProcessDeclarations(what) && errCount == errors->Count())
        result = Instructions(ctx, what);

    // Check if there is a result type, if so add a type check
    if (type)
        AddTypeCheck(ctx, what, type);

    // The generated code takes over the instructions in all cases
    proc->SetOps(&ops, &instrs, nEvals + nParms);
    if (result)
    {
        // Successful compilation - Return the code we created
        proc->nInputs = nArgs + captured.size();
        proc->nLocals = nEvals + nParms + 2;
        proc->captured = captured;

        record(bytecode_output, "Code %t: %O", what, (Op *) proc);

        return proc;
    }

    // We failed, delete the result and return
    what->Purge<Code>();
    return NULL;
}


Op *CodeBuilder::CompileInternal(Context *ctx, Tree *what, bool deferEval)
// ----------------------------------------------------------------------------
//   Compile an internal code sequence
// ----------------------------------------------------------------------------
{
    // Check if we already have translated expression in this builder
    Op *result = subexprs[what];
    if (result)
        return result;

    // Save the place where we insert instructions
    Save<Op *>      saveOps(ops, NULL);
    Save<Op **>     saveLastOp(lastOp, &ops);
    ParmOrder       noParms;
    Save<ParmOrder> saveParms(parms, noParms);
    Save<bool>      saveDefer(defer, deferEval);

    Errors *errors = MAIN->errors;
    uint errCount = errors->Count();
    if (ctx->ProcessDeclarations(what) && errCount == errors->Count())
        Instructions(ctx, what);
    result = ops;
    subexprs[what] = result;

    // Evals and parms are the max number for all subexpressions
    uint np = parms.size() + 2;
    if (nParms < np)
        nParms = np;

    return result;
}


bool CodeBuilder::Instructions(Context *ctx, Tree *what)
// ----------------------------------------------------------------------------
//    Compile an instruction or a sequence of instructions
// ----------------------------------------------------------------------------
{
    Save<Op *> saveSuccessOp(successOp, NULL);
    Save<Op *> saveFailOp(failOp, NULL);
    Scope_p    originalScope = ctx->CurrentScope();
    Context_p  gcContext     = ctx;
    TreeIDs    empty;

    while (what)
    {
        Save<TreeIDs> saveEvals(values, values);

        // Create new success exit for this expression
        Op *success = new LabelOp("success");
        successOp = success;

        // Lookup candidates (and count them)
        Save<uint> saveCandidates(candidates, 0);
        ctx->Lookup(what, compileLookup, this);

        if (candidates)
        {
            // We found candidates. Join the failOp to the successOp
            XL_ASSERT(!*lastOp && "Built code that is not NULL-terminated");

            Add(new FormErrorOp(what));
            *lastOp = success;

            lastOp = &success->success;
            instrs.push_back(success);

            InstructionsSuccess(saveEvals.saved.size());
            return true;
        }

        // In that case, the 'success' label was not used
        delete success;
        successOp = NULL;

        // Forms that we recognize directly and deal with here
        kind whatK = what->Kind();
        switch (whatK)
        {
        case INTEGER:
        case REAL:
        case TEXT:
        case NAME:
            // If not looked up, return the original
            Add(new ConstOp(what));
            InstructionsSuccess(saveEvals.saved.size());
            return true;

        case BLOCK:
        {
            // Evaluate child in a new context
            ctx->CreateScope();
            what = ((Block *) (Tree *) what)->child;
            bool hasInstructions = ctx->ProcessDeclarations(what);
            bool hasDecls = !ctx->IsEmpty();
            if (!hasDecls)
                ctx->PopScope();
            if (hasInstructions)
                continue;
            if (hasDecls)
                what = Interpreter::MakeClosure(ctx, what);
            Add(new ConstOp(what));
            if (hasDecls)
                ctx->PopScope();
            InstructionsSuccess(saveEvals.saved.size());
            return true;
        }

        case PREFIX:
        {
            // If we have a prefix on the left, check if it's a closure
            if (Tree *closed = Interpreter::IsClosure(what, &gcContext))
            {
                ctx = gcContext;
                what = closed;
                continue;
            }

            // If we have a name on the left, lookup name and start again
            Prefix *pfx = (Prefix *) (Tree *) what;
            Tree   *callee = pfx->left;
            Tree   *arg = pfx->right;

            while (Block *block = callee->AsBlock())
                callee = block->child;

            if (Name *name = callee->AsName())
            {
                // A few cases where we don't interpret the result
                if (name->value == "type"   ||
                    name->value == "extern" ||
                    name->value == "data")
                {
                    InstructionsSuccess(saveEvals.saved.size());
                    return true;
                }
            }

            // Create a call for forms like (X -> X+1) 31
            if (Infix *lifx = callee->AsInfix())
            {
                if (lifx->name == "is")
                {
                    TreeIDs   outs;
                    ParmOrder parms;

                    // If we have a single name on the left, like (X->X+1)
                    // interpret that as a lambda function
                    int argID = Evaluate(ctx, arg);
                    parms.push_back(argID);

                    // Context to declare the arguments of the lambda function
                    ctx = new Context(ctx);

                    if (Name *lfname = lifx->left->AsName())
                    {
                        // Case like '(X->X+1) Arg':
                        // Bind arg in new context and evaluate body
                        Rewrite *rw = ctx->Define(lfname, arg);
                        outs[rw->left] = -1;
                        CallOp *call = Call(ctx, lifx->right, NULL,
                                            outs, parms);
                        Add(call);
                        return true;
                    }
                    else
                    {
                        // Otherwise, map argument to declaration, e.g.
                        // '(X,Y->X+Y) (2,3)' should evaluate as 5
                        success = new LabelOp("lambda_s");
                        successOp = success;
                        Save<uint> saveCand(candidates, 0);
                        compileLookup(originalScope, originalScope,
                                      arg, lifx, this);
                        if (candidates)
                        {
                            Add(new FormErrorOp(what));
                            *lastOp = success;
                            lastOp = &success->success;
                            instrs.push_back(success);
                            InstructionsSuccess(saveEvals.saved.size());
                            return true;
                        }
                        delete success;
                        successOp = NULL;
                        Add(new FormErrorOp(what));
                        return true;
                    }
                }
            }

            // Evaluate callee
            int calleeID = Evaluate(ctx, callee);
            int argID = Evaluate(ctx, arg);

            // Lookup the argument in the callee's context
            if (!failOp)
            {
                failOp = new PrefixFormErrorOp(what);
                instrs.push_back(failOp);
            }
            Add (new IndexOp(calleeID, argID, failOp));
            InstructionsSuccess(saveEvals.saved.size());
            return true;
        }

        case POSTFIX:
        {
            // Check if there is a form that matches
            Ooops("No postfix matches $1", what);
            return false;
        }

        case INFIX:
        {
            Infix *infix = (Infix *) (Tree *) what;
            text name = infix->name;

            // Check sequences
            if (name == ";" || name == "\n")
            {
                // Sequences: evaluate left, then right
                if (!Instructions(ctx, infix->left))
                    return false;
                what = infix->right;
                continue;
            }

            // Check declarations
            if (name == "is")
            {
                // Declarations evaluate last non-declaration result, or self
                InstructionsSuccess(saveEvals.saved.size());
                return true;
            }

            // Check scoped reference
            if (name == ".")
            {
                if (!Instructions(ctx, infix->left))
                    return false;
                what = infix->right;
                continue;
            }

            // All other cases: return the input as is
            Add(new ConstOp(what));
            InstructionsSuccess(saveEvals.saved.size());
            return true;
        }
        }
    }
    return false;
}


int CodeBuilder::ValueID(Tree *self)
// ----------------------------------------------------------------------------
//    Return the evaluation ID for a given expression
// ----------------------------------------------------------------------------
{
    int id;

    // For names, use the rewrite as a unique ID
    Rewrite_p rw;
    if (context)
        if (Name *name = self->AsName())
            if (context->Bound(name, true, &rw, NULL))
                self = rw;

    TreeIDs::iterator found = values.find(self);
    if (found == values.end())
    {
        id = values.size() + 2;
        values[self] = id;
    }
    else
    {
        id = (*found).second;
    }
    return id;
}


int CodeBuilder::CaptureID(Tree *self)
// ----------------------------------------------------------------------------
//    Return the capture ID for a given expression
// ----------------------------------------------------------------------------
{
    int id;
    TreeIDs::iterator found = inputs.find(self);
    if (found == inputs.end())
    {
        id = ~inputs.size();
        outputs[self] = id;
    }
    else
    {
        id = (*found).second;
    }
    return id;
}


int CodeBuilder::Evaluate(Context *ctx, Tree *self, bool deferEval)
// ----------------------------------------------------------------------------
//   Evaluate the tree, and return its ID in the evals array
// ----------------------------------------------------------------------------
{
    int id = 0;

    // For constants, we can simply store it whenever we need it
    if (self->IsConstant())
    {
        Instructions(ctx, self);
        id = ValueID(self);
        Add(new StoreOp(id));
        return id;
    }

    // For names, look them up to check if we have them somewhere
    if (Name *name = self->AsName())
    {
        bool      evaluate = true;
        Rewrite_p rw;
        Scope_p   scope;

        if (Tree *value = ctx->Bound(name, true, &rw, &scope))
        {
            // Check if this is one of the input parameters
            depth d = ScopeDepth(scope);
            switch (d)
            {
            case PARAMETER:
            {
                TreeIDs::iterator found = inputs.find(rw);
                XL_ASSERT(found != inputs.end() && "Parameter not bound?");
                int inputId = (*found).second;

                // Don't evaluate if already evaluated during argument passing
                Tree *type = RewriteType(rw->left);
                evaluate = false;
                if (!type || type == tree_type)
                {
                    id = ValueID(rw);
                    Add(new ArgEvalOp(ctx, inputId, id, failOp));
                }
                else
                {
                    id = inputId;
                }
                break;
            }

            // Check if this is a local variable or function
            case LOCAL:
            {
                id = ValueID(rw);
                Op *code = CompileInternal(context, value, false);
                AddEval(id, code);
                break;
            }

            // Global entities can be evaluated directly in place
            case GLOBAL:
            {
                id = ValueID(rw);
                break;
            }

            // All other values are 'captured' from surrounding context
            case ENCLOSING:
            {
                id = CaptureID(rw);
                if (count(captured.begin(), captured.end(), rw) == 0)
                    captured.push_back(rw);
                evaluate = false;
                break;
            }
            } // switch

            // Evaluate code associated to name if we need to
            if (evaluate)
            {
                if (Opcode *opcode = value->GetInfo<Opcode>())
                {
                    Op *code = opcode->Clone();
                    AddEval(id, code);
                }
                else if (Code *code = value->GetInfo<Code>())
                {
                    AddEval(id, code);
                }
                else
                {
                    // No code yet: generate it
                    ParmOrder noParms;
                    TreeIDs   noParmIDs;
                    Tree *type = RewriteType(rw->left);
                    CallOp *call = Call(ctx, value, type, noParmIDs, noParms);
                    instrs.push_back(call);
                    AddEval(id, call);
                }
            }

            return id;
        }
    }

    // Other cases: generate code for the value, and evaluate it
    id = ValueID(self);
    Op *ops = CompileInternal(ctx, self, deferEval);
    AddEval(id, ops);

    // Return the allocated ID
    return id;
}


int CodeBuilder::EvaluationTemporary(Tree *self)
// ----------------------------------------------------------------------------
//    Create an evaluation temporary
// ----------------------------------------------------------------------------
{
    uint id = ValueID(self);
    Code *code = new Code(context, self, new ValueOp(id));
    self->SetInfo<Code>(code);
    return id;
}


CallOp *CodeBuilder::Call(Context *ctx, Tree *value, Tree *type,
                          TreeIDs &parmIDs, ParmOrder &parms)
// ----------------------------------------------------------------------------
//   Generate the code sequence for a call
// ----------------------------------------------------------------------------
{
    TreeList captured;
    Procedure *fn = Bytecode::Compile(ctx, value, type, parmIDs, captured);

    // Check if we captured values from the surrounding contexts
    if (uint csize = captured.size())
    {
        for (uint c = 0; c < csize; c++)
        {
            Tree *cap = captured[c];
            int id = ValueID(cap);
            parms.insert(parms.begin(), id);
        }
    }

    // Record the maximum parameter size
    uint np = parms.size() + 2;
    if (nParms < np)
        nParms = np;

    // Output slot where we will pass paramters and generate call
    CallOp *call = new CallOp(fn, ~0U, parms);
    return call;
}



// ============================================================================
//
//     Ops used during argument / parameter binding
//
// ============================================================================

template<class T>
struct MatchOp : FailOp
// ----------------------------------------------------------------------------
//   Check if the current result matches the integer/real/text value
// ----------------------------------------------------------------------------
{
    MatchOp(typename T::value_t ref, Op *fail): FailOp(fail), ref(ref) {}
    typename T::value_t ref;

    virtual Op *        Run(Data data)
    {
        Tree *test = DataResult(data);
        if (T *tval = test->As<T>())
            if (tval->value == ref)
                return success;
        return fail;
    }

    virtual kstring     OpID()  { return "match"; }
    virtual void        Dump(std::ostream &out)
    {
        out << OpID() << "\t" << ref;
    }
};

template<> kstring MatchOp<Integer>::OpID()     { return "match\tinteger"; }
template<> kstring MatchOp<Real>   ::OpID()     { return "match\treal"; }
template<> kstring MatchOp<Text>   ::OpID()     { return "match\ttext"; }


struct NameMatchOp : FailOp
// ----------------------------------------------------------------------------
//   Check if the current top of stack matches the name
// ----------------------------------------------------------------------------
{
    NameMatchOp(int testID, int nameID, Op *fail)
        : FailOp(fail), testID(testID), nameID(nameID) {}
    int testID;
    int nameID;

    virtual Op *        Run(Data data)
    {
        Tree *test = data[testID];
        Tree *ref  = data[nameID];
        if (Tree::Equal(ref, test))
            return success;
        return fail;
    }
    virtual kstring     OpID()  { return "match\tname"; }
    virtual void        Dump(std::ostream &out)
    {
        out << OpID() << "\t" << testID << "," << nameID;
    }
};


struct WhenClauseOp : FailOp
// ----------------------------------------------------------------------------
//   Check if the condition in a when clause is verified
// ----------------------------------------------------------------------------
{
    WhenClauseOp(int whenID, Op *fail): FailOp(fail), whenID(whenID) {}
    int whenID;

    virtual Op *        Run(Data data)
    {
        if (data[whenID] != xl_true)
            return fail;
        return success;
    }
    virtual kstring     OpID()  { return "when"; };
    virtual void        Dump(std::ostream &out)
    {
        out << OpID() << "\t" << whenID;
    }
};


struct InfixMatchOp : FailOp
// ----------------------------------------------------------------------------
//   Check if the result is an infix
// ----------------------------------------------------------------------------
{
    InfixMatchOp(text symbol, Op *fail, uint lid, uint rid)
        : FailOp(fail), symbol(symbol),
          lid(lid), rid(rid) {}
    text        symbol;
    uint        lid;
    uint        rid;

    virtual Op *        Run(Data data)
    {
        Tree *test = DataResult(data);
        Context_p ctx = NULL;
        if (Tree *inside = Interpreter::IsClosure(test, &ctx))
            test = inside;
        if (Infix *ifx = test->AsInfix())
        {
            if (ifx->name == symbol)
            {
                if (ctx)
                {
                    data[lid] = Interpreter::MakeClosure(ctx, ifx->left);
                    data[rid] = Interpreter::MakeClosure(ctx, ifx->right);
                }
                else
                {
                    data[lid] = ifx->left;
                    data[rid] = ifx->right;
                }
                return success;
            }
        }
        return fail;
    }

    virtual kstring     OpID()  { return "infix"; }
    virtual void        Dump(std::ostream &out)
    {
        out << OpID() << "\t" << lid << " " << symbol << " " << rid;
    }
};



// ============================================================================
//
//    Argument match
//
// ============================================================================

CodeBuilder::strength CodeBuilder::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   The pattern contains an integer: check we have the same
// ----------------------------------------------------------------------------
{
    if (Integer *ival = test->AsInteger())
        return ival->value == what->value ? ALWAYS : NEVER;
    if (test->IsConstant())
        return NEVER;
    Evaluate(context, test);
    Add(new MatchOp<Integer>(what->value, failOp));
    return SOMETIMES;
}


CodeBuilder::strength CodeBuilder::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    if (Real *rval = test->AsReal())
        return rval->value != what->value ? ALWAYS : NEVER;
    if (Integer *rval = test->AsInteger())
        return rval->value != what->value ? ALWAYS : NEVER;
    if (test->IsConstant())
        return NEVER;
    Evaluate(context, test);
    Add(new MatchOp<Real>(what->value, failOp));
    return SOMETIMES;
}


CodeBuilder::strength CodeBuilder::DoText(Text *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    if (Text *tval = test->AsText())
        return tval->value != what->value ? ALWAYS : NEVER;
    if (test->IsConstant())
        return NEVER;
    Evaluate(context, test);
    Add(new MatchOp<Text>(what->value, failOp));
    return SOMETIMES;
}


CodeBuilder::strength CodeBuilder::DoName(Name *what)
// ----------------------------------------------------------------------------
//   The pattern contains a name: bind it as a closure, no evaluation
// ----------------------------------------------------------------------------
{
    // If there is already a binding for that name, value must match
    // This covers both a pattern with 'pi' in it and things like 'X+X'
    if (Tree *bound = argsCtx->Bound(what))
    {
        if (bound->GetInfo<Opcode>())
        {
            // If this is some built-in name, we can do a static test
            if (Tree::Equal(bound, test))
                return ALWAYS;
            if (test->IsConstant())
                return NEVER;
        }

        // Do a dynamic test to check if the name value is the same
        int testID = Evaluate(context, test);
        int nameID = Evaluate(argsCtx, bound);
        Add(new NameMatchOp(testID, nameID, failOp));
        return SOMETIMES;
    }

    Evaluate(context, test, true);
    Bind(what, test);
    return ALWAYS;
}


CodeBuilder::strength CodeBuilder::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   The pattern contains a block: look inside
// ----------------------------------------------------------------------------
{
    if (Block *testBlock = test->AsBlock())
        if (testBlock->opening == what->opening &&
            testBlock->closing == what->closing)
            test = testBlock->child;

    return what->child->Do(this);
}



CodeBuilder::strength CodeBuilder::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   The pattern contains prefix: check that the left part matches
// ----------------------------------------------------------------------------
{
    // The test itself should be a prefix
    if (Prefix *pfx = test->AsPrefix())
    {
        // If we call 'sin X' and match 'sin 3', check if names match
        if (Name *name = what->left->AsName())
        {
            if (Name *testName = pfx->left->AsName())
            {
                if (name->value == testName->value)
                {
                    test = pfx->right;
                    return what->right->Do(this);
                }
                else
                {
                    Ooops("Prefix name $1 does not match $2", name, testName);
                    return NEVER;
                }
            }
        }

        // For other cases, we must go deep inside each prefix to check
        return DoLeftRight(what->left, what->right, pfx->left, pfx->right);
    }

    // All other cases are a mismatch
    Ooops("Prefix $1 does not match $2", what, test);
    return NEVER;
}


CodeBuilder::strength CodeBuilder::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//   The pattern contains posfix: check that the right part matches
// ----------------------------------------------------------------------------
{
    // The test itself should be a postfix
    if (Postfix *pfx = test->AsPostfix())
    {
        // If we call 'X!' and match '3!', check if names match
        if (Name *name = what->right->AsName())
        {
            if (Name *testName = pfx->right->AsName())
            {
                if (name->value == testName->value)
                {
                    test = pfx->left;
                    return what->left->Do(this);
                }
                else
                {
                    Ooops("Postfix name $1 does not match $2", name, testName);
                    return NEVER;
                }
            }
        }

        // For other cases, we must go deep inside each prefix to check
        return DoLeftRight(what->right, what->left, pfx->right, pfx->left);
    }

    // All other cases are a mismatch
    Ooops("Postfix $1 does not match $2", what, test);
    return NEVER;
}


CodeBuilder::strength CodeBuilder::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   The complicated case: various declarations
// ----------------------------------------------------------------------------
{
    Save<Context_p> saveContext(context, context);

    // Check if we have typed arguments, e.g. X:integer
    if (what->name == ":")
    {
        Name *name = what->left->AsName();
        if (!name)
        {
            Ooops("Invalid declaration, $1 is not a name", what->left);
            return NEVER;
        }

        // Typed name: evaluate type and check match
        Tree *namedType = NULL;
        Tree *type = what->right;
        if (Name *typeName = type->AsName())
            if (Tree *found = context->Bound(typeName))
                namedType = found;

        if (namedType)
        {
            if (namedType == tree_type)
            {
                Evaluate(context, test, true);
                Bind(name, test);
                return ALWAYS;
            }
            Scope *scope = context->CurrentScope();
            if (Tree *cast = xl_typecheck(scope, namedType, test))
            {
                test = cast;
                Evaluate(context, test);
                Bind(name, test, namedType);
                return ALWAYS;
            }

            // Check if this is a builtin type vs. a constant
            if (test->IsConstant())
                return NEVER;
        }

        // In all other cases, we need do perform dynamic evaluation to check
        Evaluate(context, test);
        AddTypeCheck(context, test, type);
        Bind(name, test, type);
        return SOMETIMES;
    }

    // Check if we have typed declarations, e.g. X+Y as integer
    if (what->name == "as")
    {
        if (resultType)
        {
            Ooops("Duplicate return type declaration $1", what);
            Ooops("Previously declared type was $1", resultType);
        }
        resultType = what->right;
        return what->left->Do(this);
    }

    // Check if we have a guard clause
    if (what->name == "when")
    {
        // It must pass the rest (need to bind values first)
        if (what->left->Do(this) == NEVER)
            return NEVER;

        // Here, we need to evaluate in the local context, not eval one
        int whenID = Evaluate(argsCtx, what->right);
        Add(new WhenClauseOp(whenID, failOp));
        return SOMETIMES;
    }

    // In all other cases, we need an infix with matching name
    Infix_p ifx = test->AsInfix();
    strength str = ALWAYS;
    if (!ifx || ifx->name != what->name)
    {
        if (test->IsConstant())
            return NEVER;
        TreePosition pos = test->Position();
        Name *l = new Name("left", pos);
        Name *r = new Name("right", pos);
        ifx = new Infix(what->name, l, r, pos);
        uint lid = EvaluationTemporary(l);
        uint rid = EvaluationTemporary(r);

        // Try to get an infix by evaluating what we have
        Evaluate(context, test);
        Add(new InfixMatchOp(what->name, failOp, lid, rid));
        str = SOMETIMES;
    }

    if (ifx && ifx->name == what->name)
    {
        strength st2 = DoLeftRight(what->left,what->right,ifx->left,ifx->right);
        if (st2 < str)
            return st2;
        return str;
    }

    // Mismatch
    Ooops("Infix $1 does not match $2", what, test);
    return NEVER;
}


CodeBuilder::strength CodeBuilder::DoLeftRight(Tree *wl, Tree *wr,
                                               Tree *l, Tree *r)
// ----------------------------------------------------------------------------
//    Combine left and right to get best result
// ----------------------------------------------------------------------------
{
    test = l;
    strength onLeft = wl->Do(this);
    if (onLeft == NEVER)
        return NEVER;
    test = r;
    strength onRight = wr->Do(this);
    if (onRight < onLeft)
        return onRight;
    return onLeft;
}


int CodeBuilder::Bind(Name *name, Tree *value, Tree *type)
// ----------------------------------------------------------------------------
//   Enter a new binding in the current context
// ----------------------------------------------------------------------------
{
    XL_ASSERT(inputs.find(name) == inputs.end() && "Binding name twice");

    Tree *defined = name;
    if (type)
        defined = new Infix(":", defined, type, name->Position());

    // Define the name in the parameters
    Rewrite *rw = argsCtx->Define(defined, value);

    // Generate the (negative) parameter ID for the given parameter
    int parmId = ~outputs.size();
    outputs[rw->left] = parmId;

    // Record parameter order for calls
    uint id = ValueID(value);
    parms.push_back(id);

    return parmId;
}

XL_END


XL::Op *xldebug(XL::Op *op)
// ----------------------------------------------------------------------------
//   Show an opcode alone
// ----------------------------------------------------------------------------
{
    std::cerr << op << "\n";;
    return op;
}


XL::Op &xldebug(XL::Op &op)
// ----------------------------------------------------------------------------
//   Show an opcode and all children
// ----------------------------------------------------------------------------
{
    std::cerr << op << "\n";
    return op;
}


XL::CodeBuilder *xldebug(XL::CodeBuilder *cb)
// ----------------------------------------------------------------------------
//   Show an opcode and all children as a listing
// ----------------------------------------------------------------------------
{
    std::cerr << cb->instrs << "\n";
    return cb;
}


XL::TreeIDs& xldebug(XL::TreeIDs &tids)
// ----------------------------------------------------------------------------
//   Show the contents of a tree IDs map
// ----------------------------------------------------------------------------
{
    for (XL::TreeIDs::iterator i = tids.begin(); i != tids.end(); i++)
        std::cerr << (*i).first << " @index " << (*i).second << "\n";
    return tids;
}
