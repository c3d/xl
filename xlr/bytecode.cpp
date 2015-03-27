// ****************************************************************************
//  bytecode.cpp                                                   Tao project
// ****************************************************************************
//
//   File Description:
//
//
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

#include "bytecode.h"
#include "interpreter.h"
#include "save.h"
#include "errors.h"
#include "basics.h"

#include <algorithm>
#include <sstream>

XL_BEGIN

// ============================================================================
//
//    Main entry point
//
// ============================================================================

Tree *EvaluateWithBytecode(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//   Compile bytecode and then evaluate it
// ----------------------------------------------------------------------------
{
    Code *code = CompileToBytecode(context, what);
    Tree_p result = what;
    if (code)
    {
        TreeList noargs;
        Data     data(context, what, noargs);

        data.result = what;
        Op *op = code->Run(data);
        while(op)
            op = op->Run(data);
        result = data.result;
    }
    return result;
}


Code *CompileToBytecode(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//    Compile a tree to bytecode
// ----------------------------------------------------------------------------
{
    CodeBuilder  builder;
    Code *code = builder.Compile(context, what);
    return code;
}


Code *CompileToBytecode(Context *context, Tree *what, TreeIndices &parms)
// ----------------------------------------------------------------------------
//    Compile a tree to bytecode
// ----------------------------------------------------------------------------
{
    CodeBuilder  builder;
    Code *code = builder.Compile(context, what, parms);
    return code;
}



// ============================================================================
//
//   Evaluating a code sequence
//
// ============================================================================

Code::Code(Context *context, Tree *self, uint nArgs)
// ----------------------------------------------------------------------------
//    Create a new code from the given ops
// ----------------------------------------------------------------------------
    : Op("code", runCode), context(context), self(self),
      ops(NULL),
      nArgs(nArgs), nVars(0), nEvals(0), nParms(0),
      instrs()
{}


Code::Code(Context *context, Tree *self, Op *ops, uint nArgs)
// ----------------------------------------------------------------------------
//    Create a new code from the given ops
// ----------------------------------------------------------------------------
    : Op("code", runCode), context(context), self(self),
      ops(ops),
      nArgs(nArgs), nVars(0), nEvals(0), nParms(0),
      instrs()
{}


Code::~Code()
// ----------------------------------------------------------------------------
//    Delete the ops we own
// ----------------------------------------------------------------------------
{
    for (Ops::iterator o = instrs.begin(); o != instrs.end(); o++)
        delete *o;
    instrs.clear();
}


void Code::SetOps(Op **newOps, Ops *instrsToTakeOver)
// ----------------------------------------------------------------------------
//    Take over the given code
// ----------------------------------------------------------------------------
{
    ops = *newOps;
    *newOps = NULL;
    instrs = *instrsToTakeOver;
    instrsToTakeOver->clear();
}


Op *Code::runCode(Op *codeOp, Data &data)
// ----------------------------------------------------------------------------
//   Run all instructions in the sequence
// ----------------------------------------------------------------------------
{
    Code *code = (Code *) codeOp;

    // Running in-place in the same context
    Save<Context_p> setContext(data.context, code->context);
    Save<Tree_p>    setSelf(data.self, code->self);
    Op *op = code->ops;
    while (op)
        op = op->Run(data);

    return codeOp->success;
}


Op *Code::runCodeWithScope(Op *codeOp, Data &data)
// ----------------------------------------------------------------------------
//   Run all instructions in the sequence
// ----------------------------------------------------------------------------
{
    Code *code = (Code *) codeOp;
    Data newData(code->context, code->self, data.parms, code->nArgs);
    newData.Allocate(code->nVars, code->nEvals, code->nParms);

    // Execute the following instructions in the newly created data
    Op *op = code->ops;
    while (op)
        op = op->Run(newData);

    // Copy result and left to the old data
    data.result = newData.result;
    data.left = newData.left;

    return codeOp->success;
}


void Code::Dump(std::ostream &out)
// ----------------------------------------------------------------------------
//   Dump all the instructions
// ----------------------------------------------------------------------------
{
    out << name << "\t" << (void *) (Op *) this
        << "\tentry\t" << (void *) ops
        << "\t" << self << "\n";
    out << "\talloc"
        << " A" << nArgs << " V" << nVars
        << " E" << nEvals << " P" << nParms << "\n";
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
        found = (out << sep << null);
    for (uint o = 0; o < max; o++)
        if ((*currentDump)[o] == op)
            found = (out << sep << set << "\t#" << o);
    if (!found)
        out << sep << set << "\t" << (void *) op;
    return out.str();
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
    FailOp(kstring n, arity_none_fn fn, Op *fx): Op(n, fn), fail(fx) {}
    FailOp(kstring n, arity_data_fn fn, Op *fx): Op(n, fn), fail(fx) {}
    FailOp(kstring n, arity_opdata_fn fn, Op *fx): Op(n, fn), fail(fx) {}
    Op *fail;
    virtual Op *Fail()  { return fail; }
};


struct LabelOp : Op
// ----------------------------------------------------------------------------
//    The target of a jump
// ----------------------------------------------------------------------------
{
    LabelOp(kstring name = "label"): Op(name, label) {}
    static Op *label(Op *op, Data &data)
    {
        // Do nothing
        return op->success;
    }
    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << (void *) this;
    }
};


struct ConstOp : Op
// ----------------------------------------------------------------------------
//   Evaluates a constant
// ----------------------------------------------------------------------------
{
    ConstOp(Tree *value): Op("const", returnConst), value(value) {}
    Tree *      value;
    static Tree *returnConst(Op *op)     { return ((ConstOp *) op)->value; }
    virtual void Dump(std::ostream &out)
    {
        kstring kinds[] = { "integer", "real", "text", "name",
                            "block", "prefix", "postfix", "infix" };
        out << name << "\t" << kinds[value->Kind()] << "\t" << value;
    }
};


struct SelfOp : Op
// ----------------------------------------------------------------------------
//   Evaluate self
// ----------------------------------------------------------------------------
{
    SelfOp(): Op("self", returnSelf) {}
    static Tree *returnSelf(Data &data) { return data.self; }
};


struct EvalOp : FailOp
// ----------------------------------------------------------------------------
//    Evaluate the given tree once and only once
// ----------------------------------------------------------------------------
{
    EvalOp(uint id, Op *ops, Op *fail, bool saveLeft = false)
        : FailOp(saveLeft ? "eval2nd"    : "eval",
                 saveLeft ? evalSaveLeft : eval,
                 fail),
          ops(ops), id(id) {}
    Op * ops;
    uint id;

    static Op *eval(Op *evalOp, Data &data)
    {
        EvalOp *ev = (EvalOp *) evalOp;
        uint id = ev->id;
        Tree *result = data.Value(id);
        if (result)
        {
            data.result = result;
            return ev->success;
        }

        // Complete evaluation of the bytecode we were given
        Op *op = ev->ops;
        while (op)
            op = op->Run(data);

        // Save the result if evaluation was successful
        if (data.result)
        {
            data.Value(id, data.result);

            // Return the next operation to execute
            return ev->success;
        }

        // Otherwise, go to the fail bytecode
        return ev->fail;
    }

    static Op *evalSaveLeft(Op *op, Data &data)
    {
        // Save the 'left' in data because we want to reuse it
        Tree_p left = data.result;
        Op *nextOp = eval(op, data);
        data.left = left;
        return nextOp;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << id << Code::Ref(ops, "\t", "code", "null");
    }
};


struct EvalClearOp : Op
// ----------------------------------------------------------------------------
//    Clear a range of eval entries after a complete evaluation
// ----------------------------------------------------------------------------
{
    EvalClearOp(uint lo, uint hi): Op("eclear", eclear), lo(lo), hi(hi) {}
    uint lo, hi;
    static Op *eclear(Op *op, Data &data)
    {
        EvalClearOp *ec = (EvalClearOp *) op;
        for (uint v = ec->lo; v < ec->hi; v++)
            data.Value(v, NULL);
        return ec->success;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << lo << ".." << hi;
    }
};


struct ValueOp : Op
// ----------------------------------------------------------------------------
//    Return a tree that we know was already evaluated
// ----------------------------------------------------------------------------
{
    ValueOp(uint id): Op("value", value), id(id) {}
    uint id;
    static Op *value(Op *op, Data &data)
    {
        ValueOp *vop = (ValueOp *) op;
        data.result = data.Value(vop->id);
        return vop->success;
    }
    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << id;
    }
};


struct StoreOp : Op
// ----------------------------------------------------------------------------
//    Store a value in "permanent" storage
// ----------------------------------------------------------------------------
{
    StoreOp(uint id): Op("store", store), id(id) {}
    uint id;
    static Op *store(Op *op, Data &data)
    {
        StoreOp *sop = (StoreOp *) op;
        data.Value(sop->id, data.result);
        return sop->success;
    }
    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << id;
    }
};


struct EnterOp : Op
// ----------------------------------------------------------------------------
//   The 'Enter' key on RPN calculators, pushes result to left
// ----------------------------------------------------------------------------
{
    EnterOp(): Op("enter", enter) {}
    static Tree *enter(Data &data)
    {
        data.left = data.result;
        return data.result;
    }
};


struct BinaryOp : Op
// ----------------------------------------------------------------------------
//    Enter two arguments for use in ops that take two input arguments
// ----------------------------------------------------------------------------
{
    BinaryOp(uint left, uint right)
        : Op("binary", binary), left(left), right(right) {}
    uint left, right;
    static Op *binary(Op *op, Data &data)
    {
        BinaryOp *bop = (BinaryOp *) op;
        data.left = data.Value(bop->left);
        data.result = data.Value(bop->right);
        return bop->success;
    }
    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << left << ", " << right;
    }
};


struct ArgOp : Op
// ----------------------------------------------------------------------------
//    Reload an argument value
// ----------------------------------------------------------------------------
{
    ArgOp(uint argId): Op("arg", load), argId(argId) {}
    uint    argId;

    static Op *load(Op *op, Data &data)
    {
        ArgOp *ld = (ArgOp *) op;
        Tree *result = data.Arg(ld->argId);
        if (Infix *infix = result->AsInfix())
            if (infix->name == "->")
                result = infix->right;
        data.result = result;
        return ld->success;
    }

    virtual void Dump(std::ostream &out) { out << name << "\t" << argId; }
};


struct VarOp : Op
// ----------------------------------------------------------------------------
//    First reference to a variable stores the declaration in locals
// ----------------------------------------------------------------------------
{
    VarOp(uint varId, Infix *decl)
        : Op("var", varInit), varId(varId), decl(decl) {}
    uint        varId;
    Infix_p     decl;
    static Op *varInit(Op *op, Data &data)
    {
        VarOp *vop = (VarOp *) op;
        data.Var(vop->varId, vop->decl);
        data.result = vop->decl->right;
        return vop->success;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << varId << "\t" << decl->left;
    }
};


struct LoadOp : Op
// ----------------------------------------------------------------------------
//    Reference a variable from the locals
// ----------------------------------------------------------------------------
{
    LoadOp(uint varId): Op("load", load), varId(varId) {}
    uint        varId;
    static Op *load(Op *op, Data &data)
    {
        LoadOp *ld = (LoadOp *) op;
        Infix *decl = (Infix *) data.Var(ld->varId);
        XL_ASSERT(decl && decl->Kind() == INFIX);
        data.result = decl->right;
        return ld->success;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << varId;
    }
};


struct ParmOp : Op
// ----------------------------------------------------------------------------
//    Save an output parameter
// ----------------------------------------------------------------------------
{
    ParmOp(uint parmId, uint valId)
        : Op("parm", store), parmId(parmId), valId(valId) {}
    uint parmId, valId;
    static Op *store(Op *op, Data &data)
    {
        ParmOp *st = (ParmOp *) op;
        data.Parm(st->parmId, data.Value(st->valId));
        return st->success;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << parmId << "=" << valId;
    }
};


struct TypeCheckOp : FailOp
// ----------------------------------------------------------------------------
//   Check if a type matches the given type
// ----------------------------------------------------------------------------
{
    TypeCheckOp(Op *fail)
        : FailOp("typecheck", typecheck, fail) {}
    static Tree *typecheck(Data &data)
    {
        Tree *cast = TypeCheck(data.context, data.result, data.left);
        return cast;
    }
};


struct ClosureOp : Op
// ----------------------------------------------------------------------------
//    Create a closure
// ----------------------------------------------------------------------------
{
    ClosureOp(Scope *scope): Op("closure", closure), scope(scope) {}
    Scope_p scope;
    static Op *closure(Op *op, Data &data)
    {
        ClosureOp *cls = (ClosureOp *) op;
        if (cls->scope != data.context->CurrentScope())
        {
            Context *context = new Context(cls->scope);
            data.result = MakeClosure(context, data.result);
        }
        return cls->success;
    }
    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << (void *) scope;
    }
};


struct DiscloseOp : Op
// ----------------------------------------------------------------------------
//    If the value is a closure, update the data context
// ----------------------------------------------------------------------------
{
    DiscloseOp(): Op("disclose", disclose) {}
    static Tree *disclose(Data &data)
    {
        if (Tree *inside = IsClosure(data.result, &data.context))
            data.result = inside;
        return data.result;
    }
};


struct CallOp : Op
// ----------------------------------------------------------------------------
//    Call a body - Parms are supposed to have been written first
// ----------------------------------------------------------------------------
{
    CallOp(Op *ops): Op("call", call), ops(ops) {}
    Op *ops;
    static Op  *call(Op *op, Data &data)
    {
        CallOp *call = (CallOp *) op;
        op = call->ops;
        while (op)
            op = op->Run(data);
        return call->success;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << Code::Ref(ops, "\t", "code", "null");
    }
};


struct FormErrorOp : Op
// ----------------------------------------------------------------------------
//   When we fail with all candidates, report an error
// ----------------------------------------------------------------------------
{
    FormErrorOp(Tree *self): Op("error", error), self(self) {}
    Tree_p self;
    static Op *error(Op *op, Data &data)
    {
        FormErrorOp *fe = (FormErrorOp *) op;
        Ooops("No form matches $1", fe->self);
        return fe->success;
    }
};



// ============================================================================
//
//   Building a code sequence and variants
//
// ============================================================================

CodeBuilder::CodeBuilder()
// ----------------------------------------------------------------------------
//   Create a code builder
// ----------------------------------------------------------------------------
    : ops(NULL), lastOp(&ops),
      variables(), evals(), parms(),
      nEvals(0), nParms(0),
      candidates(0), test(NULL), resultType(NULL),
      context(NULL), locals(NULL),
      failOp(NULL), successOp(NULL)
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


void CodeBuilder::Success()
// ----------------------------------------------------------------------------
//    Success at the end of a declaration
// ----------------------------------------------------------------------------
{
    XL_ASSERT(successOp && failOp);
    XL_ASSERT(count(instrs.begin(), instrs.end(), failOp) == 0);

    IFTRACE(compile)
        std::cerr << "SUCCESS:\t" << successOp << "\n"
                  << "FAIL:\t"    << failOp << "\n";

    // End current stream to the success exit, restart code gen at failure exit
    *lastOp = successOp;
    XL_ASSERT(failOp && "Success without a failure exit");

    lastOp = &failOp->success;
    instrs.push_back(failOp);
    failOp = NULL;

    XL_ASSERT(!*lastOp);
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
//      Evaluate integer        or goto fail1.1         (EvalOp - SaveLeft)
//      TypeCheck U : integer   or goto fail1.1         (TypeCheckOp)
//      Bind U to X                                     (ParmOp 0)
//
//      ;; Match V against Y:integer
//      Evaluate V              or goto fail1.1         (EvalOp)
//      Evaluate integer        or goto fail1.1         (EvalOp - SaveLeft)
//      TypeCheck V : integer   or goto fail1.1         (TypeCheckOp)
//      Bind V to Y                                     (ParmOp 1)
//
//      ;; Call foo1
//      Call foo1                                       (CallOp)
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
//  In general, the evaluation context has a two-deep stack, containing
//  'result' as the last result, and 'left' as the next element.
//  The 'EvalOp' has two variants, one of which saves the current result
//  in the 'data.left' field (for two-argument functions)
//  One-operand operations, e.g. 'sin', read 'result' and write into it.
//  Two-operand operations, e.g. TypeCheck, use 'left' and 'result'.
//  Argument passing for user-defined functions uses 'parms', an array
//  in the Data structure.
//

static Tree *compileLookup(Scope *evalScope, Scope *declScope,
                           Tree *self, Infix *decl, void *cb)
// ----------------------------------------------------------------------------
//    Lookup a given declaration and generate code for it
// ----------------------------------------------------------------------------
{
    static uint  depth = 0;
    Save<uint>   saveDepth(depth, depth+1);

    CodeBuilder *code = (CodeBuilder *) cb;
    uint         cindex = code->candidates++;

    IFTRACE(compile)
        std::cerr << "COMPILE" << depth << ":" << cindex
                  << "(" << self << ") from "
                  << decl->left << "\n";

    // Create the scope for evaluation
    Context_p    context = new Context(evalScope);
    Context_p    locals  = NULL;

    // Create the exit point for failed evaluation
    Op *         oldFailOp = code->failOp;
    Op *         failOp = new LabelOp("fail");
    code->failOp = failOp;

    // We start with new parameters for each candidate
    TreeIndices         empty;
    Save<TreeIndices>   saveParms(code->parms, empty);
    ParmOrder           noParmOrder;
    Save<ParmOrder>     saveParmOrder(code->parmOrder, noParmOrder);

    // If we lookup a name or a number, just return it
    Tree *defined = RewriteDefined(decl->left);
    Tree *resultType = tree_type;
    bool isLeaf = defined->IsLeaf();
    CodeBuilder::strength strength = CodeBuilder::ALWAYS;
    if (isLeaf)
    {
        if (!Tree::Equal(defined, self))
        {
            IFTRACE(compile)
                std::cerr << "COMPILE" << depth << ":" << cindex
                          << "(" << self << ") from constant "
                          << decl->left << " MISMATCH\n";
            code->failOp = oldFailOp;
            delete failOp;
            return NULL;
        }
        locals = context;
    }
    else
    {
        // Create the scope for binding the parameters
        locals = new Context(declScope);
        locals->CreateScope();

        // Remember the old end in case we did not generate code
        Op **lastOp = code->lastOp;
        uint lastInstrSize = code->instrs.size();

        // Check bindings of arguments to declaration, exit if fails
        Save<Tree_p>    saveSelf(code->test, self);
        Save<Context_p> saveContext(code->context, context);
        Save<Context_p> saveLocals(code->locals, locals);
        Save<Tree_p>    saveResultType(code->resultType, NULL);

        strength = decl->left->Do(code);
        if (strength == CodeBuilder::NEVER)
        {
            IFTRACE(compile)
                std::cerr << "COMPILE" << depth << ":" << cindex
                          << "(" << self << ") from " << decl->left
                          << " MISMATCH\n";

            // Remove the instructions that were added and the failed exit
            *lastOp = NULL;
            uint lastNow = code->instrs.size();
            for (uint i = lastInstrSize; i < lastNow; i++)
                delete code->instrs[i];
            code->instrs.resize(lastInstrSize);
            code->failOp = oldFailOp;
            code->lastOp = lastOp;
            delete failOp;
            return NULL;
        }
        if (code->resultType)
            resultType = code->resultType;
    }

    // Check if we have builtins (opcode or C bindings)
    if (decl->right == xl_self)
    {
        // If the right is "self", just return the input
        IFTRACE(compile)
            std::cerr << "COMPILE" << depth << ":" << cindex
                      << "(" << self << ") from " << decl->left
                      << " SELF\n";
        code->Add(new SelfOp);
    }
    else if (Opcode *opcode = OpcodeInfo(decl))
    {
        // Cached callback - Make a copy
        XL_ASSERT(opcode->arity <= Op::ARITY_SELF);
        XL_ASSERT(!opcode->success);
        if (opcode->arity == Op::ARITY_TWO ||
            opcode->arity == Op::ARITY_CONTEXT_TWO)
        {
            uint lfID = code->parmOrder[0];
            uint rtID = code->parmOrder[1];
            code->Add(new BinaryOp(lfID, rtID));
        }
        if (opcode->arity < Op::ARITY_SELF)
            code->Add(new Op(*opcode));
        else
            code->Add(new ConstOp(defined));
        IFTRACE(compile)
            std::cerr << "COMPILE" << depth << ":" << cindex
                      << "(" << self << ") OPCODE " << opcode->name
                      << "\n";
    }
    else if (isLeaf)
    {
        // Assign an ID for names
        code->Reference(defined, decl);
    }
    else
    {
        // Normal case: evaluate body of the declaration in the new context
        Code *body = CompileToBytecode(locals, decl->right, code->parms);
        for (uint p = 0; p < code->parmOrder.size(); p++)
            code->Add(new ParmOp(p, code->parmOrder[p]));
        code->Add(new CallOp(body));
    }

    // Check if there is a result type, if so add a type check
    if (resultType != tree_type)
    {
        code->Evaluate(context, resultType, true);
        code->Add(new TypeCheckOp(code->failOp));
    }

    // Successful evaluation
    code->Success();

    // Record the maximum parameter size
    uint np = code->parms.size();
    if (code->nParms < np)
        code->nParms = np;

    // Keep looking for other declarations
    IFTRACE(compile)
        std::cerr << "COMPILE" << depth << ":" << cindex
                  << "(" << self << ") SUCCCESS\n";

    if (strength == CodeBuilder::ALWAYS)
        return decl;
    return NULL;
}


Code *CodeBuilder::Compile(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//    Compile a top-level declaration (no parameters)
// ----------------------------------------------------------------------------
{
    TreeIndices noparms;
    return Compile(context, what, parms);
}


Code *CodeBuilder::Compile(Context *context, Tree *what, TreeIndices &callArgs)
// ----------------------------------------------------------------------------
//    Compile the tree
// ----------------------------------------------------------------------------
{
    // Check if we already compiled this particular tree (possibly recursive)
    Code *code = what->GetInfo<Code>();
    if (code)
        return code;

    // Does not exist yet, set it up
    uint nArgs = callArgs.size();
    Save<TreeIndices> saveArgs(args, callArgs);
    code = new Code(context, what, nArgs);
    what->SetInfo<Code>(code);

    // Evaluate the input code
    bool result = true;
    Errors *errors = MAIN->errors;
    uint errCount = errors->Count();
    if (context->ProcessDeclarations(what) && errCount == errors->Count())
        result = Instructions(context, what);

    // The generated code takes over the instructions in all cases
    code->SetOps(&ops, &instrs);
    if (result)
    {
        // Successful compilation - Return the code we created
        code->nVars  = variables.size();
        code->nEvals = nEvals;
        code->nParms = nParms;
        if (code->nVars || code->nEvals || code->nParms)
            code->arity_opdata = code->runCodeWithScope;

        IFTRACE(ucode)
            std::cerr << "CODE " << what << "\n"
                      << code << "\n";

        return code;
    }

    // We failed, delete the result and return
    what->Purge<Code>();
    return NULL;
}


Op *CodeBuilder::CompileInternal(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//   Compile an internal code sequence
// ----------------------------------------------------------------------------
{
    // Check if we already have translated expression in this builder
    Op *result = subexprs[what];
    if (result)
        return result;

    // Save the place where we insert instructions
    Save<Op *>          saveOps(ops, NULL);
    Save<Op **>         saveLastOp(lastOp, &ops);
    TreeIndices         empty;
    Save<TreeIndices>   saveParms(parms, empty);

    Errors *errors = MAIN->errors;
    uint errCount = errors->Count();
    if (context->ProcessDeclarations(what) && errCount == errors->Count())
        Instructions(context, what);
    result = ops;
    subexprs[what] = result;

    // Evals and parms are the max number for all subexpressions
    uint np = parms.size();
    if (nParms < np)
        nParms = np;

    return result;
}


bool CodeBuilder::Instructions(Context *ctx, Tree *what)
// ----------------------------------------------------------------------------
//    Compile an instruction or a sequence of instructions
// ----------------------------------------------------------------------------
{
    Save<Op *>  saveSuccessOp(successOp, NULL);
    Save<Op *>  saveFailOp(failOp, NULL);
    Scope_p     originalScope = ctx->CurrentScope();
    Context_p   context = ctx;
    TreeIndices empty;

    while (what)
    {
        Save<TreeIndices>   saveEvals(evals, evals);

        // Create new success exit for this expression
        Op *success = new LabelOp("success");
        successOp = success;

        // Lookup candidates (and count them)
        Save<uint> saveCandidates(candidates, 0);
        context->Lookup(what, compileLookup, this);

        if (candidates)
        {
            // We found candidates. Join the failOp to the successOp
            XL_ASSERT(!*lastOp && "Built code that is not NULL-terminated");

            Add(new FormErrorOp(what));
            *lastOp = success;

            lastOp = &success->success;
            instrs.push_back(success);

            uint ne = evals.size();
            if (nEvals < ne)
                nEvals = ne;
            uint neOld = saveEvals.saved.size();
            if (ne > neOld)
                Add(new EvalClearOp(neOld, ne));

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
            return true;

        case BLOCK:
        {
            // Evaluate child in a new context
            context->CreateScope();
            what = ((Block *) (Tree *) what)->child;
            bool hasInstructions = context->ProcessDeclarations(what);
            if (context->IsEmpty())
                context->PopScope();
            if (hasInstructions)
                continue;
            Add(new ConstOp(what));
            return true;
        }

        case PREFIX:
        {
            // If we have a prefix on the left, check if it's a closure
            if (Tree *closed = IsClosure(what, &context))
            {
                what = closed;
                continue;
            }

            // If we have a name on the left, lookup name and start again
            Prefix *pfx = (Prefix *) (Tree *) what;
            Tree   *callee = pfx->left;
            Tree   *originalCallee = callee;


            // Check if we had something like '(X->X+1) 31' as closure
            if (Tree *inside = IsClosure(callee, &context))
                callee = inside;

            if (Name *name = callee->AsName())
            {
                // A few cases where we don't interpret the result
                if (name->value == "type"   ||
                    name->value == "extern" ||
                    name->value == "data")
                    return true;

                Scope_p scope;
                if (Tree *bound = context->Bound(name, true, NULL, &scope))
                {
                    context = new Context(scope);
                    callee = bound;
                }
            }

            // This variable records if we evaluated the callee
            Tree *arg = pfx->right;

            // Eliminate blocks on the callee side
            while (Block *blk = callee->AsBlock())
                callee = blk->child;

            // If we have an infix on the left, check if it's a single rewrite
            if (Infix *lifx = callee->AsInfix())
            {
                // Check if we have a function definition
                if (lifx->name == "->")
                {
                    // If we have a single name on the left, like (X->X+1)
                    // interpret that as a lambda function
                    if (Name *lfname = lifx->left->AsName())
                    {
                        // Case like '(X->X+1) Arg':
                        // Bind arg in new context and evaluate body
                        context = new Context(context);
                        context->Define(lfname, arg);
                        what = lifx->right;
                        continue;
                    }

                    // Otherwise, enter declaration and retry, e.g.
                    // '(X,Y->X+Y) (2,3)' should evaluate as 5
                    context = new Context(context);
                    context->Define(lifx->left, lifx->right);
                    what = arg;
                    continue;
                }
            }

            // Other cases: evaluate the callee, and if it changed, retry
            if (callee != originalCallee)
            {
                // We need to evaluate argument in current context
                if (Instructions(context, arg))
                {
                    // We built a new context if left was a block
                    TreePosition pos = pfx->Position();
                    if (Tree *inside = IsClosure(callee, &context))
                    {
                        what = arg;
                        // Check if we have a single definition on the left
                        if (Infix *ifx = inside->AsInfix())
                            if (ifx->name == "->")
                                what = new Prefix(callee, arg, pos);
                    }
                    else
                    {
                        // Other more regular cases
                        what = new Prefix(callee, arg, pos);
                    }
                    continue;
                }
            }

            // If we get there, we didn't find anything interesting to do
            Ooops("No prefix matches $1", what);
            return false;
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
                Context *leftContext = context;
                if (!Instructions(leftContext, infix->left))
                    return false;
                what = infix->right;
                continue;
            }

            // Check declarations
            if (name == "->")
            {
                // Declarations evaluate last non-declaration result, or self
                return true;
            }

            // Check scoped reference
            if (name == ".")
            {
                if (!Instructions(context, infix->left))
                    return false;
                Add(new DiscloseOp);
                what = infix->right;
                continue;
            }

            // All other cases: return the input as is
            Add(new ConstOp(what));
            return true;
        }
        }
    }
    return false;
}


uint CodeBuilder::EvaluationID(Tree *self)
// ----------------------------------------------------------------------------
//    Return the evaluation ID for a given expression
// ----------------------------------------------------------------------------
{
    uint id = evals.size();
    TreeIndices::iterator found = evals.find(self);
    if (found == evals.end())
        evals[self] = id;
    else
        id = (*found).second;
    return id;
}


uint CodeBuilder::Evaluate(Context *ctx, Tree *self, bool saveLeft)
// ----------------------------------------------------------------------------
//   Evaluate the tree, and return its ID in the evals array
// ----------------------------------------------------------------------------
{
    uint id = EvaluationID(self);
    bool computed = false;

    // For constants, we can simply evaluate in line
    if (self->IsConstant())
    {
        if (saveLeft)
            Add(new EnterOp);
        Instructions(ctx, self);
        computed = true;
    }

    else if (Name *name = self->AsName())
    {
        // Check if that's one of the input parameters. If so, emit an 'Arg'
        Rewrite_p rw;
        Scope_p   scope;
        Context * inner = ctx;
        if (context)
            inner = context;
        if (Tree *arg = inner->Bound(name, true, &rw, &scope))
        {
            TreeIndices::iterator found = args.find(rw->left);
            if (found != args.end())
            {
                uint localId = (*found).second;
                if (saveLeft)
                    Add(new EnterOp);
                Add(new ArgOp(localId));
                computed = true;
            }

            else if (Opcode *op = arg->GetInfo<Opcode>())
            {
                if (saveLeft)
                    Add(new EnterOp);
                if (op->arity < Op::ARITY_SELF)
                    Add(new Op(*op));
                else
                    Add(new ConstOp(arg));
                computed = true;
            }
        }
    }

    if (computed)
    {
        // Store the value we just computed
        Add(new StoreOp(id));
    }
    else
    {
        // Compile the code for the input
        Op *op = CompileInternal(ctx, self);
        
        // Add an evaluation opcode
        Add(new EvalOp(id, op, failOp, saveLeft));
    }
    
    // Return the allocated ID
    return id;
}


uint CodeBuilder::EvaluationTemporary(Tree *self)
// ----------------------------------------------------------------------------
//    Create an evaluation temporary
// ----------------------------------------------------------------------------
{
    uint id = EvaluationID(self);
    Code *code = new Code(context, self, new ValueOp(id));
    self->SetInfo<Code>(code);
    return id;
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
    MatchOp(T *ref, Op *fail): FailOp("match", match, fail), ref(ref) {}
    GCPtr<T>    ref;

    static Op *match(Op *op, Data &data)
    {
        MatchOp *mo = (MatchOp *) op;
        Tree *test = data.result;
        if (T *tval = test->As<T>())
            if (tval->value == mo->ref->value)
                return mo->success;
        return mo->fail;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << ref;
    }
};

template<> void MatchOp<Integer>::Dump(std::ostream &out)
{
    out << name << "\tinteger\t" << ref;
}
template<> void MatchOp<Real>::Dump(std::ostream &out)
{
    out << name << "\treal\t" << ref;
}
template<> void MatchOp<Text>::Dump(std::ostream &out)
{
    out << name << "\ttext\t" << ref;
}


struct NameMatchOp : FailOp
// ----------------------------------------------------------------------------
//   Check if the current top of stack matches the name
// ----------------------------------------------------------------------------
{
    NameMatchOp(Op *fail)
        : FailOp("name_match", match, fail) {}

    static Op *match(Op *op, Data &data)
    {
        NameMatchOp *nmo = (NameMatchOp *) op;
        Tree *test = data.result;
        Tree *ref  = data.left;
        if (Tree::Equal(ref, test))
            return nmo->success;
        return nmo->fail;
    }
};


struct WhenClauseOp : FailOp
// ----------------------------------------------------------------------------
//   Check if the condition in a when clause is verified
// ----------------------------------------------------------------------------
{
    WhenClauseOp(Op *fail): FailOp("when", when, fail) {}

    static Op *when(Op *op, Data &data)
    {
        WhenClauseOp *wc = (WhenClauseOp *) op;
        if (data.result != xl_true)
            return wc->fail;
        data.result = data.left;
        return wc->success;
    }
};


struct InfixMatchOp : FailOp
// ----------------------------------------------------------------------------
//   Check if the result is an infix
// ----------------------------------------------------------------------------
{
    InfixMatchOp(text symbol, Op *fail, uint lid, uint rid)
        : FailOp("infix", infix, fail), symbol(symbol),
          lid(lid), rid(rid) {}
    text        symbol;
    uint        lid;
    uint        rid;

    static Op *infix(Op *op, Data &data)
    {
        InfixMatchOp *im = (InfixMatchOp *) op;
        if (Infix *ifx = data.result->AsInfix())
        {
            if (ifx->name == im->symbol)
            {
                data.Value(im->lid, ifx->left);
                data.Value(im->rid, ifx->right);
                return im->success;
            }
        }
        return im->fail;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << symbol;
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
    Evaluate(context, test);
    Add(new MatchOp<Integer>(what, failOp));
    return SOMETIMES;
}


CodeBuilder::strength CodeBuilder::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    if (Real *rval = test->AsReal())
        return rval->value != what->value ? ALWAYS : NEVER;
    Evaluate(context, test);
    Add(new MatchOp<Real>(what, failOp));
    return SOMETIMES;
}


CodeBuilder::strength CodeBuilder::DoText(Text *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    if (Text *tval = test->AsText())
        return tval->value != what->value ? ALWAYS : NEVER;
    Evaluate(context, test);
    Add(new MatchOp<Text>(what, failOp));
    return SOMETIMES;
}


CodeBuilder::strength CodeBuilder::DoName(Name *what)
// ----------------------------------------------------------------------------
//   The pattern contains a name: bind it as a closure, no evaluation
// ----------------------------------------------------------------------------
{
    // If there is already a binding for that name, value must match
    // This covers both a pattern with 'pi' in it and things like 'X+X'
    if (Tree *bound = locals->Bound(what))
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
        Evaluate(locals, test);
        Evaluate(locals, bound, true);
        Add(new NameMatchOp(failOp));
        return SOMETIMES;
    }

    Bind(what, test, true);
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

        // Check if this is a builtin type vs. a constant
        if (test->IsConstant())
        {
            if (Name *typeName = what->right->AsName())
            {
                if (Tree *bound = context->Bound(typeName))
                {
                    if (!TypeCheck(context, bound, test))
                        return NEVER;
                    Bind(name, test, false);
                    return ALWAYS;
                }
            }
        }

        // Typed name: evaluate type and check match
        Evaluate(context, test);
        Evaluate(context, what->right, true);
        Add(new TypeCheckOp(failOp));
        Bind(name, test, false);
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
        Evaluate(locals, what->right, true);
        Add(new WhenClauseOp(failOp));
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


uint CodeBuilder::Bind(Name *name, Tree *value, bool closure)
// ----------------------------------------------------------------------------
//   Enter a new binding in the current context, remember left and right
// ----------------------------------------------------------------------------
{
    XL_ASSERT(parms.find(name) == parms.end() && "Binding name twice");

    // Define the name in the locals
    locals->Define(name, value);

    // Generate the parameter ID for the given parameter
    uint parmId = parms.size();
    parms[name] = parmId;

    // Evaluate the value and store it in the parameter
    uint id = Evaluate(context, value);
    if (closure && (int) id >= 0)
        Add(new ClosureOp(context->CurrentScope()));

    // Save the value used for binding
    parmOrder.push_back(id);

    return parmId;
}


uint CodeBuilder::Reference(Tree *name, Infix *decl)
// ----------------------------------------------------------------------------
//   Make a reference to a local or outer variable
// ----------------------------------------------------------------------------
{
    // Check if that's one of the input parameters. If so, emit an 'Arg'
    TreeIndices::iterator found = args.find(name);
    if (found != args.end())
    {
        uint localId = (*found).second;
        Add(new ArgOp(localId));
        return ~localId;
    }

    // Otherwise, allocate a variable with the decl
    uint varId = variables.size();
    found = variables.find(name);
    if (found == variables.end())
    {
        variables[name] = varId;
        Add(new VarOp(varId, decl));
    }
    else
    {
        varId = (*found).second;
        Add(new LoadOp(varId));
    }

    return varId;
}

XL_END


extern "C" void debugo(XL::Op *op)
// ----------------------------------------------------------------------------
//   Show an opcode alone
// ----------------------------------------------------------------------------
{
    std::cerr << op << "\n";;
}


extern "C" void debugop(XL::Op *op)
// ----------------------------------------------------------------------------
//   Show an opcode and all children
// ----------------------------------------------------------------------------
{
    std::cerr << *op << "\n";
}


extern "C" void debugob(XL::CodeBuilder *cb)
// ----------------------------------------------------------------------------
//   Show an opcode and all children as a listing
// ----------------------------------------------------------------------------
{
    std::cerr << cb->instrs << "\n";
}
