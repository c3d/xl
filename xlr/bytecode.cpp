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


XL_BEGIN

// ============================================================================
//
//   Evaluating a code sequence
//
// ============================================================================

Code::Code()
// ----------------------------------------------------------------------------
//    Create a new code from the given ops
// ----------------------------------------------------------------------------
    : Op("code", runCode), ops(NULL)
{}


Code::Code(Op *ops)
// ----------------------------------------------------------------------------
//    Create a new code from the given ops
// ----------------------------------------------------------------------------
    : Op("code", runCode), ops(ops)
{}


Code::~Code()
// ----------------------------------------------------------------------------
//    Delete the ops we own
// ----------------------------------------------------------------------------
{
    Op::Delete(ops);
}


void Code::SetOps(Op **newOps)
// ----------------------------------------------------------------------------
//    Take over the given code
// ----------------------------------------------------------------------------
{
    ops = *newOps;
    *newOps = NULL;
}


Op *Code::runCode(Op *codeOp, Data &data)
// ----------------------------------------------------------------------------
//   Run all instructions in the sequence
// ----------------------------------------------------------------------------
{
    Code *code = (Code *) codeOp;
    Op *op = code->ops;
    while (op)
        op = op->Run(data);
    return codeOp->next;
}


void Code::Dump(std::ostream &out)
// ----------------------------------------------------------------------------
//   Dump all the instructions
// ----------------------------------------------------------------------------
{
    out << name << "\t" << (void *) this << "\tbegin\n"
        << *ops
        << name << "\t" << (void *) this << "\tend\n";
}



// ============================================================================
//
//    Opcodes we use in this translation
//
// ============================================================================

struct LabelOp : Op
// ----------------------------------------------------------------------------
//    The target of a jump
// ----------------------------------------------------------------------------
{
    LabelOp(kstring name = "label"): Op(name, label), id(currentId++) {}
    uint id;
    static uint currentId;
    static Op *label(Op *op, Data &data)
    {
        // Do nothing
        return op->next;
    }
    virtual void Dump(std::ostream &out) { out << name << "\t" << id << "\n"; }
};
uint LabelOp::currentId = 0;


struct ConstOp : Op
// ----------------------------------------------------------------------------
//   Evaluates a constant
// ----------------------------------------------------------------------------
{
    ConstOp(Tree *value): Op("const", returnConst), value(value) {}
    Tree *      value;
    static Tree *returnConst(Op *op)     { return ((ConstOp *) op)->value; }
    virtual void Dump(std::ostream &out) { out << name << "\t"<<value<<"\n"; }
};


struct SelfOp : Op
// ----------------------------------------------------------------------------
//   Evaluate self
// ----------------------------------------------------------------------------
{
    SelfOp(): Op("self", returnSelf) {}
    static Tree *returnSelf(Data &data) { return data.self; }
};


struct EvalOp : Op
// ----------------------------------------------------------------------------
//    Evaluate the given tree
// ----------------------------------------------------------------------------
{
    EvalOp(uint id, Op *ops, Op *fail, bool saveLeft = false)
        : Op("eval", saveLeft ? evalSaveLeft : eval),
          id(id), ops(ops), fail(fail) {}
    ~EvalOp() { Delete(ops); }

    uint id;
    Op * ops;
    Op * fail;

    static Op *eval(Op *evalOp, Data &data)
    {
        EvalOp *ev = (EvalOp *) evalOp;
        uint id = ev->id;
        Tree *result = data.Value(id);
        if (result)
        {
            data.result = result;
            return ev->next;
        }

        // Complete evaluation of the bytecode we were given
        Op *op = ev->ops;
        while (op)
            op = op->Run(data);

        // Save the result if evaluation was successful
        if (data.result)
        {
            result = data.Value(id, data.result);

            // Return the next operation to execute
            return ev->next;
        }

        // Otherwise, go to the fail bytecode
        return ev->fail;
    }

    static Op *evalSaveLeft(Op *op, Data &data)
    {
        // Save the 'left' in data because we want to reuse it
        Save<Tree_p> saveLeft(data.left, data.left);

        // Evaluate normally
        return eval(op, data);
    }

    virtual Op *Fail()
    {
        return fail;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << "\tbegin\t" << id << "\t" << fail << "\n"
            << *ops
            << name << "\tend\t" << id << "\n";
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
        return vop->next;
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
        return ld->next;
    }

    virtual void Dump(std::ostream &out) { out<<name<<"\t"<<argId<<"\n"; }
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
        return vop->next;
    }
    
    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << varId << "\t" << decl->left << "\n";
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
        return ld->next;
    }
    
    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" <<varId << "\n";
    }
};


struct ParmOp : Op
// ----------------------------------------------------------------------------
//    Save an output parameter
// ----------------------------------------------------------------------------
{
    ParmOp(uint parmId): Op("parm", store), parmId(parmId) {}
    uint parmId;
    static Op *store(Op *op, Data &data)
    {
        ParmOp *st = (ParmOp *) op;
        data.Parm(st->parmId, data.result);
        return st->next;
    }
    
    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << parmId << "\n";
    }
};


struct TypeCheckOp : Op
// ----------------------------------------------------------------------------
//   Check if a type matches the given type
// ----------------------------------------------------------------------------
{
    TypeCheckOp(Op *fail)
        : Op("typecheck", typecheck), fail(fail) {}
    Op *fail;
    static Tree *typecheck(Data &data)
    {
        Tree *cast = TypeCheck(data.context, data.result, data.left);
        return cast;
    }
    virtual Op *Fail()
    {
        return fail;
    }
    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << fail << "\n";
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
        return cls->next;
    }
    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << (void *) scope << "\n";
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


struct ScopeOp : Op
// ----------------------------------------------------------------------------
//    Evaluate a body in a new data environment
// ----------------------------------------------------------------------------
{
    ScopeOp(Context *locals, Tree *self,
            uint nArgs, uint nVars, uint nEvals, uint nParms)
        : Op("scope", scope), locals(locals), self(self),
          nArgs(nArgs), nVars(nVars), nEvals(nEvals), nParms(nParms) {}
    Context_p   locals;
    Tree_p      self;
    uint        nArgs;
    uint        nVars;
    uint        nEvals;
    uint        nParms;

    static Op *scope(Op *scopeOp, Data &data)
    {
        ScopeOp *sc = (ScopeOp *) scopeOp;
        Data newData(sc->locals, sc->self, data.parms, sc->nArgs);
        if (sc->nVars || sc->nEvals || sc->nParms)
            newData.Allocate(sc->nVars, sc->nEvals, sc->nParms);

        // Execute the following instructions in the newly created data
        Op *op = sc->next;
        while (op)
            op = op->Run(newData);

        return NULL;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << "\t"
            << (void *) locals << "\t" << (void *) this
            << "\n\t" << ShortTreeForm(self, 20)
            << "\n\tA" << nArgs
            << " L" << nVars
            << " E" << nEvals
            << " P" << nParms
            <<"\n";
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
        return call->next;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << (void *) ops << "\n";
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
    : ops(NULL), lastOp(&ops)
{}


CodeBuilder::~CodeBuilder()
// ----------------------------------------------------------------------------
//    Delete all instructions if the list was not transferred
// ----------------------------------------------------------------------------
{
    Op::Delete(ops);
}


void CodeBuilder::Add(Op *op)
// ----------------------------------------------------------------------------
//    Add an instruction in the generated code
// ----------------------------------------------------------------------------
{
    *lastOp = op;
    lastOp = &op->next;
}


void CodeBuilder::Success()
// ----------------------------------------------------------------------------
//    Success at the end of a declaration
// ----------------------------------------------------------------------------
{
    // End current stream to the success exit, restart code gen at failure exit
    *lastOp = successOp;
    lastOp = &failOp->next;
}



// ============================================================================
//
//    Compilation of the tree
//
// ============================================================================

static Tree *compileLookup(Scope *evalScope, Scope *declScope,
                           Tree *self, Infix *decl, void *cb)
// ----------------------------------------------------------------------------
//    Lookup a given declaration and generate code for it
// ----------------------------------------------------------------------------
{
    static uint depth = 0;
    Save<uint> saveDepth(depth, depth+1);

    CodeBuilder *code = (CodeBuilder *) cb;
    uint         cindex = code->candidates++;

    // Create the scope for evaluation
    Context_p   context = new Context(evalScope);
    Context_p   locals  = NULL;

    // Create the exit point for failed evaluation
    Op *        failOp = new LabelOp("fail");
    Save<Op *>  saveFailOp(code->failOp, failOp);

    // If we lookup a name or a number, just return it
    Tree *defined = RewriteDefined(decl->left);
    Tree *resultType = tree_type;
    if (defined->IsLeaf())
    {
        if (!Tree::Equal(defined, self))
        {
            IFTRACE(compile)
                std::cerr << "COMPILE" << depth << ":" << cindex
                          << "(" << self << ") from constant "
                          << decl->left << " MISMATCH\n";
            delete failOp;
            return NULL;
        }
        locals = context;

        // Assign an ID for names
        code->Reference(defined, decl);
    }
    else
    {
        // Create the scope for binding the parameters
        locals = new Context(declScope);
        locals->CreateScope();

        // Remember the old end in case we did not generate code
        Op **lastOp = code->lastOp;

        // Check bindings of arguments to declaration, exit if fails
        Save<Tree_p>    saveSelf(code->test, self);
        Save<Context_p> saveContext(code->context, context);
        Save<Context_p> saveLocals(code->locals, locals);
        Save<Tree_p>    saveResultType(code->resultType, NULL);
        if (!decl->left->Do(code))
        {
            IFTRACE(compile)
                std::cerr << "COMPILE" << depth << ":" << cindex
                          << "(" << self << ") from " << decl->left
                          << " MISMATCH\n";

            // Remove the instructions that were added and the failed exit
            Op *added = *lastOp;
            *lastOp = NULL;
            Op::Delete(added);
            delete failOp;
            return NULL;
        }
        if (code->resultType)
            resultType = code->resultType;

    }

    // Check if the right is "self"
    if (decl->right == xl_self)
    {
        IFTRACE(compile)
            std::cerr << "COMPILE" << depth << ":" << cindex
                      << "(" << self << ") from " << decl->left
                      << " SELF\n";
        code->Add(new SelfOp);
    }

    // Check if we have builtins (opcode or C bindings)
    else if (Opcode *opcode = OpcodeInfo(decl))
    {
        // Cached callback - Make a copy
        XL_ASSERT(opcode->arity <= Op::SELF);
        XL_ASSERT(!opcode->next);
        code->Add(new Op(*opcode));
        IFTRACE(compile)
            std::cerr << "COMPILE" << depth << ":" << cindex
                      << "(" << self << ") OPCODE " << opcode->name
                      << "\n";
    }
    else
    {
        // Normal case: evaluate body of the declaration in the new context
        uint nVars  = code->variables.size();
        uint nEvals = code->evals.size();
        uint nParms = code->parms.size();
        Op *body = code->Compile(locals, decl->right, nVars, nEvals, nParms);
        code->Add(new CallOp(body));
    }

    // Check if there is a result type, if so add a type check
    if (resultType != tree_type)
    {
        code->Add(new EnterOp);       // Save value in 'left'
        code->Evaluate(context, resultType, true);
        code->Add(new TypeCheckOp(code->failOp));
    }

    // Enclose the result if necessary
    code->Add(new ClosureOp(locals->CurrentScope()));

    // Successful evaluation
    code->Success();

    // Keep looking for other declarations
    IFTRACE(compile)
        std::cerr << "COMPILE" << depth << ":" << cindex
                  << "(" << self << ") SUCCCESS\n";
    return NULL;
}


Op *CodeBuilder::Compile(Context *context, Tree *what,
                         uint nArgs, uint nVars, uint nEvals, uint nParms)
// ----------------------------------------------------------------------------
//    Compile the tree
// ----------------------------------------------------------------------------
{
    // Check if we already compiled this particular tree (possibly recursive)
    Code *code = what->GetInfo<Code>();
    if (code)
        return code;

    // Does not exist yet, set it up
    code = new Code;
    what->SetInfo<Code>(code);

    // Save the place where we insert instructions
    Save<Op *>  saveOps(ops, NULL);
    Save<Op **> saveLastOp(lastOp, &ops);
    Save<Op *>  saveSuccessOp(successOp, NULL);

    // We start with a clean slate for this code
    TreeIndices empty;
    Save<TreeIndices>   saveParms(parms,     empty);
    Save<TreeIndices>   saveVars (variables, empty);
    Save<TreeIndices>   saveEvals(evals,     empty);

    // Check if we need to create a frame
    if (nArgs || nVars || nEvals || nParms)
        Add(new ScopeOp(context, what, nArgs, nVars, nEvals, nParms));

    // Evaluate the input code
    bool result = Instructions(context, what);

    // The generated code takes over the instructions in all cases
    code->SetOps(&ops);
    if (result)
    {
        // Successful compilation - Return the code we created
        return code;
    }

    // We failed, delete the result and return
    delete code;
    return NULL;
}


bool CodeBuilder::Instructions(Context *ctx, Tree *what)
// ----------------------------------------------------------------------------
//    Compile an instruction or a sequence of instructions
// ----------------------------------------------------------------------------
{
    // Check if we have instructions or only a declaration
    if (!context->ProcessDeclarations(what))
    {
        // Only a declaration - return self
        Add(new SelfOp);
        return true;
    }

    Scope_p   originalScope = ctx->CurrentScope();
    Context_p context = ctx;
    while (what)
    {
        // Create new success exit for this expression
        Op *success = new LabelOp("success");
        if (successOp)
            successOp->next = success;
        if (failOp)
            failOp->next = success;
        successOp = success;
        failOp = NULL;

        // Lookup candidates (and count them)
        candidates = 0;
        context->Lookup(what, compileLookup, this);

        if (candidates)
        {
            // We found candidates. Join the failOp to the successOp
            if (failOp)
                failOp->next = successOp;
            lastOp = &successOp->next;
            return true;
        }

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

            // Check if we had something like '(X->X+1) 31' as closure
            Context_p calleeContext = NULL;
            if (Tree *inside = IsClosure(callee, &calleeContext))
                callee = inside;

            if (Name *name = callee->AsName())
                // A few cases where we don't interpret the result
                if (name->value == "type"   ||
                    name->value == "extern" ||
                    name->value == "data")
                    return true;

            // This variable records if we evaluated the callee
            Tree *newCallee = NULL;
            Tree *arg = pfx->right;

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
            if (!newCallee)
            {
                Context_p newContext = new Context(context);
                Evaluate(newContext, callee);
                Add(new EnterOp);
            }

            if (newCallee != callee)
            {
                // We need to evaluate argument in current context
                if (Instructions(context, arg))
                {
                    // We built a new context if left was a block
                    TreePosition pos = pfx->Position();
                    if (Tree *inside = IsClosure(newCallee, &context))
                    {
                        what = arg;
                        // Check if we have a single definition on the left
                        if (Infix *ifx = inside->AsInfix())
                            if (ifx->name == "->")
                                what = new Prefix(newCallee, arg, pos);
                    }
                    else
                    {
                        // Other more regular cases
                        what = new Prefix(newCallee, arg, pos);
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

            // All other cases: failure
            Ooops("No infix matches $1", what);
            return false;
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


uint CodeBuilder::Evaluate(Context *context, Tree *self, bool saveLeft)
// ----------------------------------------------------------------------------
//   Evaluate the tree, and return its ID in the evals array
// ----------------------------------------------------------------------------
{
    uint id = EvaluationID(self);

    // Compile the code for the input
    Op *ops = Compile(context, self);

    // Add an evaluation opcode
    Add(new EvalOp(id, ops, failOp, saveLeft));

    // Return the allocated ID
    return id;
}


uint CodeBuilder::EvaluationTemporary(Tree *self)
// ----------------------------------------------------------------------------
//    Create an evaluation temporary
// ----------------------------------------------------------------------------
{
    uint id = EvaluationID(self);
    Code *code = new Code(new ValueOp(id));
    self->SetInfo<Code>(code);
    return id;
}




// ============================================================================
//
//     Ops used during argument / parameter binding
//
// ============================================================================

template<class T>
struct MatchOp : Op
// ----------------------------------------------------------------------------
//   Check if the current result matches the integer/real/text value
// ----------------------------------------------------------------------------
{
    MatchOp(T *ref, Op *fail): Op("match", match), ref(ref), fail(fail) {}
    GCPtr<T>    ref;
    Op *        fail;

    static Op *match(Op *op, Data &data)
    {
        MatchOp *mo = (MatchOp *) op;
        Tree *test = data.result;
        if (T *tval = test->As<T>())
            if (tval->value == mo->ref->value)
                return mo->next;
        return mo->fail;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << ref << "\n";
    }
};

template<> void MatchOp<Integer>::Dump(std::ostream &out)
{
    out << name << "\tinteger\t" << ref << "\n";
}
template<> void MatchOp<Real>::Dump(std::ostream &out)
{
    out << name << "\treal\t" << ref << "\n";
}
template<> void MatchOp<Text>::Dump(std::ostream &out)
{
    out << name << "\ttext\t" << ref << "\n";
}


struct NameMatchOp : Op
// ----------------------------------------------------------------------------
//   Check if the current top of stack matches the name
// ----------------------------------------------------------------------------
{
    NameMatchOp(Op *fail)
        : Op("name_match", match), fail(fail) {}
    Op *        fail;

    static Op *match(Op *op, Data &data)
    {
        NameMatchOp *nmo = (NameMatchOp *) op;
        Tree *test = data.result;
        Tree *ref  = data.left;
        if (Tree::Equal(ref, test))
            return nmo->next;
        return nmo->fail;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << "\t";
        fail->Dump(out);
    }
};


struct WhenClauseOp : Op
// ----------------------------------------------------------------------------
//   Check if the condition in a when clause is verified
// ----------------------------------------------------------------------------
{
    WhenClauseOp(Op *fail): Op("when", when), fail(fail) {}
    Op *fail;

    static Op *when(Op *op, Data &data)
    {
        WhenClauseOp *wc = (WhenClauseOp *) op;
        if (data.result != xl_true)
            return wc->fail;
        data.result = data.left;
        return wc->next;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << "\t";
        fail->Dump(out);
    }
};


struct InfixMatchOp : Op
// ----------------------------------------------------------------------------
//   Check if the result is an infix
// ----------------------------------------------------------------------------
{
    InfixMatchOp(text symbol, Op *fail, uint lid, uint rid)
        : Op("infix", infix), symbol(symbol), fail(fail), lid(lid), rid(rid) {}
    text        symbol;
    Op *        fail;
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
                return im->next;
            }
        }
        return im->fail;
    }

    virtual void Dump(std::ostream &out)
    {
        out << name << "\t" << symbol << "\t";
        fail->Dump(out);
    }
};



// ============================================================================
//
//    Argument match
//
// ============================================================================

inline bool CodeBuilder::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   The pattern contains an integer: check we have the same
// ----------------------------------------------------------------------------
{
    Evaluate(context, test);
    if (Integer *ival = test->AsInteger())
        if (ival->value != what->value)
            return false;
    Add(new MatchOp<Integer>(what, failOp));
    return true;
}


inline bool CodeBuilder::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    Evaluate(context, test);
    if (Real *rval = test->AsReal())
        if (rval->value != what->value)
            return false;
    Add(new MatchOp<Real>(what, failOp));
    return true;
}


inline bool CodeBuilder::DoText(Text *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    Evaluate(context, test);
    if (Text *tval = test->AsText())
        if (tval->value != what->value)
            return false;
    Add(new MatchOp<Text>(what, failOp));
    return true;
}


inline bool CodeBuilder::DoName(Name *what)
// ----------------------------------------------------------------------------
//   The pattern contains a name: bind it as a closure, no evaluation
// ----------------------------------------------------------------------------
{
    // If there is already a binding for that name, value must match
    // This covers both a pattern with 'pi' in it and things like 'X+X'
    if (Tree *bound = locals->Bound(what))
    {
        Evaluate(locals, test);
        Add(new EnterOp);
        Evaluate(locals, bound, true);
        Add(new NameMatchOp(failOp));
        return true;
    }

    Bind(what, test, true);
    return true;
}


bool CodeBuilder::DoBlock(Block *what)
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


bool CodeBuilder::DoPrefix(Prefix *what)
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
                    return false;
                }
            }
        }

        // For other cases, we must go deep inside each prefix to check
        test = pfx->left;
        if (!what->left->Do(this))
            return false;
        test = pfx->right;
        if (what->right->Do(this))
            return true;
    }

    // All other cases are a mismatch
    Ooops("Prefix $1 does not match $2", what, test);
    return false;
}


bool CodeBuilder::DoPostfix(Postfix *what)
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
                    return false;
                }
            }
        }

        // For other cases, we must go deep inside each prefix to check
        test = pfx->right;
        if (!what->right->Do(this))
            return false;
        test = pfx->left;
        if (what->left->Do(this))
            return true;
    }

    // All other cases are a mismatch
    Ooops("Postfix $1 does not match $2", what, test);
    return false;
}


bool CodeBuilder::DoInfix(Infix *what)
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
            return false;
        }

        // Typed name: evaluate type and check match
        Evaluate(context, test);
        Add(new EnterOp);
        Evaluate(context, what->right);
        Add(new TypeCheckOp(failOp));
        Bind(name, test, false);
        return true;
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
        if (!what->left->Do(this))
            return false;

        // Here, we need to evaluate in the local context, not eval one
        Add(new EnterOp);
        Evaluate(locals, what->right, true);
        Add(new WhenClauseOp(failOp));
        return true;
    }

    // In all other cases, we need an infix with matching name
    Infix_p ifx = test->AsInfix();
    if (!ifx || ifx->name != what->name)
    {
        TreePosition pos = test->Position();
        Name *l = new Name("left", pos);
        Name *r = new Name("right", pos);
        ifx = new Infix(what->name, l, r, pos);
        uint lid = EvaluationTemporary(l);
        uint rid = EvaluationTemporary(r);

        // Try to get an infix by evaluating what we have
        Evaluate(context, test);
        Add(new InfixMatchOp(what->name, failOp, lid, rid));
    }

    if (ifx && ifx->name == what->name)
    {
        test = ifx->left;
        if (!what->left->Do(this))
            return false;
        test = ifx->right;
        if (what->right->Do(this))
            return true;
    }

    // Mismatch
    Ooops("Infix $1 does not match $2", what, test);
    return false;
}


uint CodeBuilder::Bind(Name *name, Tree *value, bool closure)
// ----------------------------------------------------------------------------
//   Enter a new binding in the current context, remember left and right
// ----------------------------------------------------------------------------
{
    XL_ASSERT(parms.find(names) == parms.end() && "Binding name twice");

    // Define the name in the locals
    locals->Define(name, value);

    // Generate the parameter ID for the given parameter
    uint parmId = parms.size();
    parms[name] = parmId;

    // Evaluate the value and store it in the parameter
    Evaluate(context, value);
    if (closure)
        Add(new ClosureOp(context->CurrentScope()));
    Add(new ParmOp(parmId));

    return parmId;
}


uint CodeBuilder::Reference(Tree *name, Infix *decl)
// ----------------------------------------------------------------------------
//   Make a reference to a local or outer variable
// ----------------------------------------------------------------------------
{
    // Check if that's one of the input parameters. If so, emit an 'Arg'
    TreeIndices::iterator found = parms.find(name);
    if (found != parms.end())
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
