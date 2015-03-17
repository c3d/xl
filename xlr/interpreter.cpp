// ****************************************************************************
//  interpreter.cpp                                                XLR project
// ****************************************************************************
//
//   File Description:
//
//    An interpreter for XLR that does not rely on LLVM at all
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

#include "interpreter.h"
#include "gc.h"
#include "info.h"
#include "errors.h"
#include "types.h"
#include "renderer.h"
#include "basics.h"

#include <cmath>


XL_BEGIN


// ============================================================================
//
//    Closure management (keeping scoping information with values)
//
// ============================================================================

static inline Tree *isClosure(Tree *tree, Context_p *context)
// ----------------------------------------------------------------------------
//   Check if something is a closure, if so set scope and/or context
// ----------------------------------------------------------------------------
{
    if (Scope *closure = tree->AsPrefix())
    {
        if (Scope *scope = ScopeParent(closure))
        {
            if (closure->GetInfo<ClosureInfo>())
            {
                // We normally have a scope on the left
                if (context)
                    *context = new Context(scope);
                return closure->right;
            }
        }
    }
    return NULL;
}


static inline Tree *makeClosure(Context_p context, Tree *value)
// ----------------------------------------------------------------------------
//   Create a closure encapsulating the current context
// ----------------------------------------------------------------------------
{
retry:
    kind valueKind = value->Kind();

    if (valueKind >= NAME || context->HasRewritesFor(valueKind))
    {
        if (valueKind == NAME)
        {
            if (Tree *bound = context->Bound(value))
            {
                if (Tree *inside = isClosure(bound, &context))
                {
                    if (value != inside)
                    {
                        value = inside;
                        goto retry;
                    }
                }
            }
        }

        if (valueKind != PREFIX || !value->GetInfo<ClosureInfo>())
        {
            Scope *scope = context->CurrentScope();
            value = new Prefix(scope, value);

            ClosureInfo *closureMarker = new ClosureInfo;
            value->SetInfo(closureMarker);
        }
    }
    return value;
}



// ============================================================================
//
//    Code and Data - Replaying the evaluation of a given expression
//
// ============================================================================

struct Op;
typedef std::vector<Op *>    Ops;


struct Code : Info
// ----------------------------------------------------------------------------
//    Sequence of ops to evaluate an expression
// ----------------------------------------------------------------------------
{
    Code(Scope *scope): scope(scope), fail(NULL) {}
    ~Code();

    Tree *              Run(Context *context, Tree *source);
    void                Add(Op *op)     { ops.push_back(op); }

public:
    Scope_p             scope;
    Ops                 ops;
    Code *              fail;
};


struct Data
// ----------------------------------------------------------------------------
//    Data storage for evaluating an expression
// ----------------------------------------------------------------------------
{
    Data(Context *ctx) : context(ctx), locals(NULL) {}

    void Reset(Scope *scope, Tree *src)
    {
        locals = new Context(scope);
        locals->CreateScope();

        stack.clear();
        stack.push_back(src);
    }

    void Push(Tree *t)
    {
        stack.push_back(t);
    }
    
    Tree *Pop()
    {
        XL_ASSERT(stack.size());
        Tree *result = stack.back();
        stack.pop_back();
        return result;
    }

    void Bind(Tree *t)
    {
        vars.push_back(t);
    }
    
    void MustEvaluate(uint index, bool updateContext)
    {
        XL_ASSERT(index < evals.size());
        Tree *test = evals[index];
        if (!test)
        {
            test = stack.back();
            test = EvaluateClosure(context, test);
            evals[index] = test;
        }
        if (updateContext)
            if (Tree *inside = isClosure(test, &context))
                test = inside;
        stack.back() = test;
    }

public:
    Context_p           context;
    Context_p           locals;
    TreeList            stack;
    TreeList            vars;
    TreeList            evals;
};



// ============================================================================
//
//    Op - Evaluation of a single operation
//
// ============================================================================

struct Op
// ----------------------------------------------------------------------------
//    Evaluate a single instruction in a 'Code' instance
// ----------------------------------------------------------------------------
{
    Op() {}
    virtual ~Op() {}
    virtual bool Run(Data &data) = 0;
};



// ============================================================================
//
//   Evaluating a code sequence and variants
//
// ============================================================================

Code::~Code()
// ----------------------------------------------------------------------------
//    Delete all instructions
// ----------------------------------------------------------------------------
{
    for (Ops::iterator o = ops.begin(); o != ops.end(); o++)
        delete *o;
}


Tree *Code::Run(Context *ctx, Tree *src)
// ----------------------------------------------------------------------------
//     Run the recorded code in the given context on the given tree
// ----------------------------------------------------------------------------
{
    Data data(ctx);

    for (Code *code = this; code; code = code->fail)
    {
        bool ok = true;
        data.Reset(code->scope, src);
        for (Ops::iterator o=code->ops.begin(); ok && o!=code->ops.end(); o++)
            ok = (*o)->Run(data);
        if (ok)
            return data.Pop();
    }

    return NULL;
}


template<class T>
struct MatchOp : Op
// ----------------------------------------------------------------------------
//   Check if the current top of stack matches the integer/real/text value
// ----------------------------------------------------------------------------
{
    MatchOp(T *ref): ref(ref) {}
    GCPtr<T>    ref;

    bool Run(Data &data)
    {
        Tree *test = data.Pop();
        if (T *tval = test->As<T>())
            if (tval->value == ref->value)
                return true;
        return false;
    }
};


struct NameMatchOp : Op
// ----------------------------------------------------------------------------
//   Check if the current top of stack matches the name
// ----------------------------------------------------------------------------
{
    NameMatchOp(Name *ref): ref(ref) {}
    Name_p ref;

    bool Run(Data &data)
    {
        Tree *test = data.Pop();
        if(Tree *bound = data.locals->Bound(ref))
            if (Tree::Equal(bound, test))
                return true;
        return false;
    }
};


struct BindOp : Op
// ----------------------------------------------------------------------------
//   Bind the top of stack to the given name
// ----------------------------------------------------------------------------
{
    BindOp(Name *name) : name(name) {}
    Name_p name;

    bool Run(Data &data)
    {
        Tree *test = data.Pop();
        data.locals->Define(name, test);
        data.Bind(test);
        return true;
    }
};


struct BindClosureOp : Op
// ----------------------------------------------------------------------------
//    Bind the top of stack to given name with a closure
// ----------------------------------------------------------------------------
{
    BindClosureOp(Name *name) : name(name) {}
    Name_p name;

    bool Run(Data &data)
    {
        Tree * test = data.Pop();
        Tree * closed = makeClosure(data.context, test);
        data.locals->Define(name, closed);
        data.Bind(closed);
        return true;
    }
};


struct BlockChildOp : Op
// ----------------------------------------------------------------------------
//    Replace the top of the stack with a block child
// ----------------------------------------------------------------------------
{
    BlockChildOp(Block *block) : open(block->opening), close(block->closing) {}
    text open, close;

    bool Run(Data &data)
    {
        Tree *test = data.Pop();
        if (Block *tblk = test->AsBlock())
            if (tblk->opening == open && tblk->closing == close)
                test = tblk->child;
        data.Push(test);
        return true;
    }
};


struct PrefixMatchOp : Op
// ----------------------------------------------------------------------------
//   Check if the top of stack is a prefix with given name
// ----------------------------------------------------------------------------
{
    PrefixMatchOp(text name): name(name) {}
    text name;

    bool Run(Data &data)
    {
        Tree *test = data.Pop();
        if (Prefix *pfx = test->AsPrefix())
        {
            if (Name *tname = pfx->left->AsName())
            {
                if (tname->value == name)
                {
                    data.Push(pfx->right);
                    return true;
                }
            }
        }
        return false;
    }
};


struct PrefixLeftRightMatchOp : Op
// ----------------------------------------------------------------------------
//   Validates the left and right of a prefix 
// ----------------------------------------------------------------------------
{
    bool Run(Data &data)
    {
        Tree *test = data.Pop();
        if (Prefix *pfx = test->AsPrefix())
        {
            data.Push(pfx->right);
            data.Push(pfx->left);
            return true;
        }
        return false;
    }
};
    

struct PostfixMatchOp : Op
// ----------------------------------------------------------------------------
//   Check if the top of stack is a postfix with given name
// ----------------------------------------------------------------------------
{
    PostfixMatchOp(text name): name(name) {}
    text name;

    bool Run(Data &data)
    {
        Tree *test = data.Pop();
        if (Postfix *pfx = test->AsPostfix())
        {
            if (Name *tname = pfx->right->AsName())
            {
                if (tname->value == name)
                {
                    data.Push(pfx->left);
                    return true;
                }
            }
        }
        return false;
    }
};


struct PostfixRightLeftMatchOp : Op
// ----------------------------------------------------------------------------
//   Validates the left and right of a prefix 
// ----------------------------------------------------------------------------
{
    bool Run(Data &data)
    {
        Tree *test = data.Pop();
        if (Postfix *pfx = test->AsPostfix())
        {
            data.Push(pfx->left);
            data.Push(pfx->right);
            return true;
        }
        return false;
    }
};


struct TypedBindingOp : Op
// ----------------------------------------------------------------------------
//    Type check the two top elements of the stack and bind to name
// ----------------------------------------------------------------------------
{
    TypedBindingOp(Name *name): name(name) {}
    Name_p name;

    bool Run(Data &data)
    {
        Tree *test = data.Pop();
        Tree *type = data.Pop();
        Tree *checked = TypeCheck(data.context, type, test);
        if (checked)
        {
            data.locals->Define(name, checked);
            data.Bind(checked);
            return true;
        }
        return false;
    }
};
    

struct TypeCheckOp : Op
// ----------------------------------------------------------------------------
//    Perform a type check between the two top elements of the stack
// ----------------------------------------------------------------------------
{
    bool Run(Data &data)
    {
        Tree *test = data.Pop();
        Tree *type = data.Pop();
        Tree *checked = TypeCheck(data.context, type, test);
        if (checked)
            return true;
        return false;
    }
};


struct WhenClauseOp : Op
// ----------------------------------------------------------------------------
//   Check the given when clause
// ----------------------------------------------------------------------------
{
    bool Run(Data &data)
    {
        Tree *test = data.Pop();
        if (test == xl_true)
            return true;
        if (test != xl_false)
            Ooops("Invalid guard clause, $1 is not a boolean", test);
        return false;
    }
};


struct InfixMatchOp : Op
// ----------------------------------------------------------------------------
//   Match a given infix
// ----------------------------------------------------------------------------
{
    InfixMatchOp(uint index, text name) : index(index), name(name) {}
    uint index;
    text name;
    

    bool Run(Data &data)
    {
        Tree *test = data.Pop();
        Infix *ifx = test->AsInfix();
        if (!ifx || ifx->name != name)
        {
            data.Push(test);
            data.MustEvaluate(index, true);
            test = data.Pop();
            ifx = test->AsInfix();
        }
        if (ifx)
        {
            data.Push(ifx->right);
            data.Push(ifx->left);
        }
        return false;
    }
};


struct EvaluateOp : Op
// ----------------------------------------------------------------------------
//   Evaluate a tree
// ----------------------------------------------------------------------------
{
    EvaluateOp(uint index): index(index) {}
    uint index;

    bool Run(Data &data)
    {
        XL_ASSERT(index < data.evals.size());
        Tree *test = data.Pop();
        Tree *result = data.evals[index];
        if (!result)
        {
            result = Evaluate(data.context, test);
            data.evals[index] = result;
        }
        data.Push(result);
        return true;
    }
};


struct EvaluateUpdateOp : Op
// ----------------------------------------------------------------------------
//   Evaluate a tree
// ----------------------------------------------------------------------------
{
    EvaluateUpdateOp(uint index): index(index) {}
    uint index;

    bool Run(Data &data)
    {
        XL_ASSERT(index < data.evals.size());
        Tree *test = data.Pop();
        Tree *result = data.evals[index];
        if (!result)
        {
            result = Evaluate(data.context, test);
            data.evals[index] = result;
        }
        if (Tree *inside = isClosure(result, &data.context))
            result = inside;
        data.Push(result);
        return true;
    }
};


struct EvaluateTreeOp : Op
// ----------------------------------------------------------------------------
//   Evaluate a specific tree in the context
// ----------------------------------------------------------------------------
{
    EvaluateTreeOp(uint index, Tree *tree, bool locals)
        : tree(tree), index(index), locals(locals) {}
    Tree_p tree;
    uint   index;
    bool   locals;

    bool Run(Data &data)
    {
        XL_ASSERT(index < data.evals.size());
        Tree *result = data.evals[index];
        if (!result)
        {
            result = Evaluate(locals ? data.locals : data.context, tree);
            data.evals[index] = result;
        }
        if (Tree *inside = isClosure(result, NULL))
            result = inside;
        data.Push(result);
        return true;
        
    }
};


struct EvaluateOpcodeOp : Op
// ----------------------------------------------------------------------------
//    Evaluate an opcode
// ----------------------------------------------------------------------------
{
    EvaluateOpcodeOp(Opcode *opcode, Tree *self): opcode(opcode), self(self) {}
    Opcode *opcode;
    Tree_p  self;

    bool Run(Data &data)
    {
        Tree *result = opcode->Evaluate(data.context, self, data.vars);
        data.Push(result);
        return true;
    }
};


struct BodyOp : Op
// ----------------------------------------------------------------------------
//    Evaluate a body
// ----------------------------------------------------------------------------
{
    BodyOp(Tree *body): body(body) {}
    Tree_p  body;

    bool Run(Data &data)
    {
        Tree *result = makeClosure(data.locals, body);
        data.Push(result);
        return true;
    }
};




// ============================================================================
//
//   Evaluation cache - Recording what has been evaluated and how
//
// ============================================================================

struct EvalCache: std::map<Tree_p, uint>
// ----------------------------------------------------------------------------
//    Recording evaluations and other operations
// ----------------------------------------------------------------------------
{
    EvalCache(): code(NULL) {}

public:
    Code *              code;
    TreeList            computed;
};



// ============================================================================
//
//    Primitive cache for 'opcode' and 'C' bindings
//
// ============================================================================

static inline Opcode *setInfo(Infix *decl, Opcode *opcode)
// ----------------------------------------------------------------------------
//    Create a new info for the given callback
// ----------------------------------------------------------------------------
{
    decl->right->SetInfo<Opcode>(opcode);
    return opcode;
}


static inline Opcode *opcodeInfo(Infix *decl)
// ----------------------------------------------------------------------------
//    Check if we have an opcode in the definition
// ----------------------------------------------------------------------------
{
    Tree *right = decl->right;
    Opcode *info = right->GetInfo<Opcode>();
    if (info)
        return info;

    // Check if the declaration is something like 'X -> opcode Foo'
    // If so, lookup 'Foo' in the opcode table the first time to record it
    if (Prefix *prefix = right->AsPrefix())
        if (Name *name = prefix->left->AsName())
            if (name->value == "opcode")
                if (Name *opcodeName = prefix->right->AsName())
                    if (Opcode *opcode = Opcode::Find(opcodeName->value))
                        return setInfo(decl, opcode);

    return NULL;
}



// ============================================================================
//
//    Parameter binding
//
// ============================================================================

struct Bindings
// ----------------------------------------------------------------------------
//   Structure used to record bindings
// ----------------------------------------------------------------------------
{
    typedef bool value_type;

    Bindings(Context *context, Context *locals,
             Tree *test, EvalCache &cache, TreeList &args)
        : context(context), locals(locals),
          test(test), cache(cache), resultType(NULL), args(args) {}

    // Tree::Do interface
    bool  DoInteger(Integer *what);
    bool  DoReal(Real *what);
    bool  DoText(Text *what);
    bool  DoName(Name *what);
    bool  DoPrefix(Prefix *what);
    bool  DoPostfix(Postfix *what);
    bool  DoInfix(Infix *what);
    bool  DoBlock(Block *what);

    // Evaluation and binding of values
    uint  EvaluationIndex(Tree *expr);
    void  MustEvaluate(bool updateContext = false);
    Tree *MustEvaluate(Context *context, Tree *what);
    void  Bind(Name *name, Tree *value);
    void  BindClosure(Name *name, Tree *value);

private:
    Context_p  context;
    Context_p  locals;
    Tree_p     test;
    EvalCache  &cache;

public:
    Tree_p      resultType;
    TreeList   &args;
};


#define ADD_OP(X)                                                       \
/* ------------------------------------------------------------ */      \
/*  Add an operation to the current code                        */      \
/* ------------------------------------------------------------ */      \
    do { if (cache.code) cache.code->Add(new X); } while (0)


inline bool Bindings::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   The pattern contains an integer: check we have the same
// ----------------------------------------------------------------------------
{
    MustEvaluate();
    ADD_OP(MatchOp<Integer>(what));
    if (Integer *ival = test->AsInteger())
        if (ival->value == what->value)
            return true;
    Ooops("Integer $1 does not match $2", what, test);
    return false;
}


inline bool Bindings::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    MustEvaluate();
    ADD_OP(MatchOp<Real>(what));
    if (Real *rval = test->AsReal())
        if (rval->value == what->value)
            return true;
    Ooops("Real $1 does not match $2", what, test);
    return false;
}


inline bool Bindings::DoText(Text *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    MustEvaluate();
    ADD_OP(MatchOp<Text>(what));
    if (Text *tval = test->AsText())
        if (tval->value == what->value)         // Do delimiters matter?
            return true;
    Ooops("Text $1 does not match $2", what, test);
    return false;
}


inline bool Bindings::DoName(Name *what)
// ----------------------------------------------------------------------------
//   The pattern contains a name: bind it as a closure, no evaluation
// ----------------------------------------------------------------------------
{
    Save<Context_p> saveContext(context, context);

    // The test value may have been evaluated
    EvalCache::iterator found = cache.find(test);
    if (found != cache.end())
    {
        uint index = (*found).second;
        XL_ASSERT(index <= cache.computed.size());
        if (Tree *evaluated = cache.computed[index-1])
            test = evaluated;
    }

    // If there is already a binding for that name, value must match
    // This covers both a pattern with 'pi' in it and things like 'X+X'
    if (Tree *bound = locals->Bound(what))
    {
        MustEvaluate(true);
        ADD_OP(NameMatchOp(what));
        bool result = Tree::Equal(bound, test);
        IFTRACE(eval)
            std::cerr << "  ARGCHECK: "
                      << test << " vs " << bound
                      << (result ? " MATCH" : " FAILED")
                      << "\n";
        if (!result)
            Ooops("Name $1 does not match $2", bound, test);
        return result;
    }

    IFTRACE(eval)
        std::cerr << "  CLOSURE " << ContextStack(context->CurrentScope())
                  << what << "=" << test << "\n";
    BindClosure(what, test);
    ADD_OP(BindClosureOp(what));
    return true;
}


bool Bindings::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   The pattern contains a block: look inside
// ----------------------------------------------------------------------------
{
    ADD_OP(BlockChildOp(what));
    if (Block *testBlock = test->AsBlock())
        if (testBlock->opening == what->opening &&
            testBlock->closing == what->closing)
            test = testBlock->child;

    return what->child->Do(this);
}


bool Bindings::DoPrefix(Prefix *what)
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
                    ADD_OP(PrefixMatchOp(name->value));
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
        ADD_OP(PrefixLeftRightMatchOp());
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


bool Bindings::DoPostfix(Postfix *what)
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
                ADD_OP(PostfixMatchOp(name->value));
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
        ADD_OP(PostfixRightLeftMatchOp());
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


bool Bindings::DoInfix(Infix *what)
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
            // I don't think this is possible if entered normally
            // This is coding defensively, most likely dead code
            Ooops("Invalid declaration, $1 is not a name", what->left);
            return false;
        }

        // Typed name: evaluate type and check match
        Tree *type = MustEvaluate(context, what->right);
        Tree *checked = TypeCheck(context, type, test);
        if (!checked || type == XL::value_type)
        {
            MustEvaluate(type != XL::value_type);
            checked = TypeCheck(context, type, test);
        }
        if (checked)
        {
            ADD_OP(TypedBindingOp(name));
            Bind(name, checked);
            return true;
        }

        // Type mismatch
        Ooops("Type $1 does not contain $2", type, test);
        return false;
    }

    // Check if we have typed declarations, e.g. X+Y as integer
    if (what->name == "as")
    {
        if (resultType)
        {
            Ooops("Duplicate return type declaration $1", what);
            Ooops("Previously declared type was $1", resultType);
        }
        resultType = MustEvaluate(context, what->right);
        ADD_OP(TypeCheckOp);
        return what->left->Do(this);
    }

    // Check if we have a guard clause
    if (what->name == "when")
    {
        // It must pass the rest (need to bind values first)
        if (!what->left->Do(this))
            return false;

        // Here, we need to evaluate in the local context, not eval one
        Tree *check = MustEvaluate(locals, what->right);
        ADD_OP(WhenClauseOp);
        if (check == xl_true)
            return true;
        else if (check != xl_false)
            Ooops ("Invalid guard clause, $1 is not a boolean", check);
        else
            Ooops("Guard clause $1 is not verified", what->right);
        return false;
    }

    // In all other cases, we need an infix with matching name
    uint index = EvaluationIndex(test);
    ADD_OP(InfixMatchOp(index, what->name));
    Infix *ifx = test->AsInfix();
    if (!ifx)
    {
        // Try to get an infix by evaluating what we have
        MustEvaluate(true);
        ifx = test->AsInfix();
    }
    if (ifx)
    {
        if (ifx->name != what->name)
        {
            Ooops("Infix names $1 and $2 don't match", what->Position())
                .Arg(ifx->name).Arg(what->name);
            return false;
        }
        
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


uint Bindings::EvaluationIndex(Tree *tree)
// ----------------------------------------------------------------------------
//   Return or allocate the evaluation index for the tree
// ----------------------------------------------------------------------------
{
    uint idx = cache[tree];
    if (idx)
        return idx-1;

    cache.computed.push_back(NULL);
    idx = cache.computed.size();
    cache[tree] = idx;
    return idx-1;
}


void Bindings::MustEvaluate(bool updateContext)
// ----------------------------------------------------------------------------
//   Evaluate 'test', ensuring that each bound arg is evaluated at most once
// ----------------------------------------------------------------------------
{
    uint idx = EvaluationIndex(test);
    if (updateContext)
        ADD_OP(EvaluateUpdateOp(idx));
    else
        ADD_OP(EvaluateOp(idx));
        
    Tree *evaluated = cache.computed[idx];
    if (!evaluated)
    {
        evaluated = EvaluateClosure(context, test);
        IFTRACE(eval)
            std::cerr << "  TEST(" << test << ") = "
                      << "NEW(" << evaluated << ")\n";
        cache.computed[idx] = evaluated;
    }
    else
    {
        IFTRACE(eval)
            std::cerr << "  TEST(" << test << ") = "
                      << "OLD(" << evaluated << ")\n";
    }
    
    test = evaluated;
    if (updateContext)
        if (Tree *inside = isClosure(test, &context))
            test = inside;

}


Tree *Bindings::MustEvaluate(Context *context, Tree *tval)
// ----------------------------------------------------------------------------
//   Ensure that each bound arg is evaluated at most once
// ----------------------------------------------------------------------------
{
    uint idx = EvaluationIndex(tval);
    ADD_OP(EvaluateTreeOp(idx, tval, context == locals));
    Tree *evaluated = cache.computed[idx];
    if (!evaluated)
    {
        evaluated = EvaluateClosure(context, tval);
        cache.computed[idx] = evaluated;
        IFTRACE(eval)
            std::cerr << "  NEED(" << tval << ") = "
                      << "NEW(" << evaluated << ")\n";
    }
    else
    {
        IFTRACE(eval)
            std::cerr << "  NEED(" << tval << ") = "
                      << "OLD(" << evaluated << ")\n";
    }
    if (Tree *inside = isClosure(evaluated, NULL))
        evaluated = inside;
    return evaluated;
}


void Bindings::Bind(Name *name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a new binding in the current context, remember left and right
// ----------------------------------------------------------------------------
{
    IFTRACE(eval)
        std::cerr << "  BIND " << name << "=" << ShortTreeForm(value) <<"\n";
    args.push_back(value);
    locals->Define(name, value);
}


void Bindings::BindClosure(Name *name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a new binding in the current context, preserving its environment
// ----------------------------------------------------------------------------
{
    value = makeClosure(context, value);
    Bind(name, value);
}



// ============================================================================
//
//   Main evaluation loop for the interpreter
//
// ============================================================================

static Tree *error_result = NULL;


static Tree *evalLookup(Scope *evalScope, Scope *declScope,
                        Tree *self, Infix *decl, void *ec)
// ----------------------------------------------------------------------------
//   Calllback function to check if the candidate matches
// ----------------------------------------------------------------------------
{
    Errors errors(" Candidate $1 does not match", decl);

    static uint depth = 0;
    Save<uint> saveDepth(depth, depth+1);
    IFTRACE(eval)
        std::cerr << "EVAL" << depth << "(" << self
                  << ") from " << decl->left << "\n";
    if (depth > MAIN->options.stack_depth)
    {
        Ooops("Stack depth exceeded evaluating $1", self);
        return error_result = xl_error;
    }
    else if (error_result)
    {
        return error_result;
    }

    // Create the scope for evaluation
    Context_p context = new Context(evalScope);
    Context_p locals  = NULL;
    Tree *result = decl->right;

    // Check if the decl is an opcode or C binding
    Opcode *opcode = opcodeInfo(decl);

    // If we lookup a name or a number, just return it
    Tree *defined = RewriteDefined(decl->left);
    TreeList args;
    Tree *resultType = tree_type;
    if (defined->IsLeaf())
    {
        // Must match literally, or we don't have a candidate
        if (!Tree::Equal(defined, self))
        {
            IFTRACE(eval)
                std::cerr << "EVAL" << depth << "(" << self
                          << ") from constant " << decl->left
                          << " MISMATCH\n";
            return NULL;
        }
        locals = context;
    }
    else
    {
        // Retrieve the evaluation cache for arguments
        EvalCache *cache = (EvalCache *) ec;

        // Create the scope for evaluation and local bindings
        locals = new Context(declScope);
        locals->CreateScope();

        // Check bindings of arguments to declaration, exit if fails
        Bindings  bindings(context, locals, self, *cache, args);
        if (!decl->left->Do(bindings))
        {
            IFTRACE(eval)
                std::cerr << "EVAL" << depth << "(" << self
                          << ") from " << decl->left
                          << " MISMATCH\n";
            return NULL;
        }
        if (bindings.resultType)
            resultType = bindings.resultType;
    }

    // Check if the right is "self"
    if (result == xl_self)
    {
        IFTRACE(eval)
            std::cerr << "EVAL" << depth << "(" << self
                      << ") from " << decl->left
                      << " SELF\n";
        return self;
    }

    // Check if we have builtins (opcode or C bindings)
    if (opcode)
    {
        // Cached callback
        result = opcode->Evaluate(context, result, args);
        IFTRACE(eval)
            std::cerr << "EVAL" << depth << "(" << self
                      << ") OPCODE " << opcode->name
                      << "(" << args << ") = "
                      << result << "\n";
        return result;
    }

    // Normal case: evaluate body of the declaration in the new context
    if (resultType != tree_type)
        result = new Infix("as", result, resultType, self->Position());

    result = makeClosure(locals, result);
    IFTRACE(eval)
        std::cerr << "EVAL" << depth << " BINDINGS: "
                  << ContextStack(locals->CurrentScope())
                  << "\n" << locals << "\n"
                  << "EVAL(" << self
                  << ") = (" << result << ")\n";
    return result;
}


inline Tree *encloseResult(Context *context, Scope *old, Tree *what)
// ----------------------------------------------------------------------------
//   Encapsulate result with a closure if context is not evaluation context
// ----------------------------------------------------------------------------
{
    if (context->CurrentScope() != old)
        what = makeClosure(context, what);
    return what;
}


static Tree *Instructions(Context_p context, Tree_p what)
// ----------------------------------------------------------------------------
//   Evaluate the input tree once declarations have been processed
// ----------------------------------------------------------------------------
{
    Tree_p      result = what;
    Scope_p     originalScope = context->CurrentScope();

    // Loop to avoid recursion for a few common cases, e.g. sequences, blocks
    while (what)
    {
        // First attempt to look things up
        EvalCache cache;
        if (Tree *eval = context->Lookup(what, evalLookup, &cache))
        {
            if (eval == xl_error)
                return eval;
            MAIN->errors->Clear();
            result = eval;
            if (Tree *inside = isClosure(eval, &context))
            {
                what = inside;
                continue;
            }
            return encloseResult(context, originalScope, eval);
        }

        kind whatK = what->Kind();
        switch (whatK)
        {
        case INTEGER:
        case REAL:
        case TEXT:
            return what;

        case NAME:
            Ooops("No name matches $1", what);
            return encloseResult(context, originalScope, what);

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
            return encloseResult(context, originalScope, what);
        }

        case PREFIX:
        {
            // If we have a prefix on the left, check if it's a closure
            if (Tree *closed = isClosure(what, &context))
            {
                what = closed;
                continue;
            }

            // If we have a name on the left, lookup name and start again
            Prefix *pfx = (Prefix *) (Tree *) what;
            Tree   *callee = pfx->left;

            // Check if we had something like '(X->X+1) 31' as closure
            Context_p calleeContext = NULL;
            if (Tree *inside = isClosure(callee, &calleeContext))
                callee = inside;

            if (Name *name = callee->AsName())
                // A few cases where we don't interpret the result
                if (name->value == "type"   ||
                    name->value == "extern" ||
                    name->value == "data")
                    return what;

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
                newCallee = EvaluateClosure(newContext, callee);
            }

            if (newCallee != callee)
            {
                // We need to evaluate argument in current context
                arg = Instructions(context, arg);

                // We built a new context if left was a block
                if (Tree *inside = isClosure(newCallee, &context))
                {
                    what = arg;
                    // Check if we have a single definition on the left
                    if (Infix *ifx = inside->AsInfix())
                        if (ifx->name == "->")
                            what = new Prefix(newCallee, arg, pfx->Position());
                }
                else
                {
                    // Other more regular cases
                    what = new Prefix(newCallee, arg, pfx->Position());
                }
                continue;
            }

            // If we get there, we didn't find anything interesting to do
            Ooops("No prefix matches $1", what);
            return encloseResult(context, originalScope, what);
        }

        case POSTFIX:
        {
            // Check if there is a form that matches
            Ooops("No postifx matches $1", what);
            return encloseResult(context, originalScope, what);
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
                Tree *left = Instructions(leftContext, infix->left);
                if (left != infix->left)
                    result = left;
                what = infix->right;
                continue;
            }

            // Check declarations
            if (name == "->")
            {
                // Declarations evaluate last non-declaration result, or self
                return encloseResult(context, originalScope, result);
            }

            // Check type matching
            if (name == "as")
            {
                result = TypeCheck(context, infix->right, infix->left);
                if (!result)
                {
                    Ooops("Value $1 does not match type $2",
                          infix->left, infix->right);
                    result = infix->left;
                }
                return encloseResult(context, originalScope, result);
            }

            // Check scoped reference
            if (name == ".")
            {
                Tree *left = Instructions(context, infix->left);
                isClosure(left, &context);
                what = infix->right;
                continue;
            }

            // All other cases: failure
            Ooops("No infix matches $1", what);
            return encloseResult(context, originalScope, what);
        }
        } // switch
    }// while(what)

    return encloseResult(context, originalScope, result);
}


Tree *EvaluateClosure(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//    Evaluate 'what', possibly returned as a closure in case not in 'context'
// ----------------------------------------------------------------------------
{
    // Create scope for declarations, and evaluate in this context
    Tree_p result = what;
    if (context->ProcessDeclarations(what))
    {
        Errors errors(" Cannot evaluate $1", what);
        result = Instructions(context, what);
    }

    // This is a safe point for checking collection status
    GarbageCollector::SafePoint();

    return result;
}


Tree *Evaluate(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//    Evaluate 'what', finding the final, non-closure result
// ----------------------------------------------------------------------------
{
    Tree *result = EvaluateClosure(context, what);
    if (Tree *inside = isClosure(result, NULL))
        result = inside;
    return result;
}


Tree *IsClosure(Tree *value, Context_p *context)
// ----------------------------------------------------------------------------
//   Externally usable variant of isClosure
// ----------------------------------------------------------------------------
{
    return isClosure(value, context);
}



// ============================================================================
//
//     Type checking
//
// ============================================================================

struct Expansion
// ----------------------------------------------------------------------------
//   A structure to expand a type-matched structure
// ----------------------------------------------------------------------------
{
    Expansion(Context *context): context(context) {}

    typedef Tree *value_type;

    Tree *  DoInteger(Integer *what)
    {
        return what;
    }
    Tree *  DoReal(Real *what)
    {
        return what;
    }
    Tree *  DoText(Text *what)
    {
        return what;
    }
    Tree *  DoName(Name *what)
    {
        if (Tree *bound = context->Bound(what))
        {
            if (Tree *eval = isClosure(bound, NULL))
                bound = eval;
            return bound;
        }
        return what;
    }
    Tree *  DoPrefix(Prefix *what)
    {
        Tree *left  = what->left->Do(this);
        Tree *right = what->right->Do(this);
        if (left != what->left || right != what->right)
            return new Prefix(left, right, what->Position());
        return what;
    }
    Tree *  DoPostfix(Postfix *what)
    {
        Tree *left  = what->left->Do(this);
        Tree *right = what->right->Do(this);
        if (left != what->left || right != what->right)
            return new Postfix(left, right, what->Position());
        return what;

    }
    Tree *  DoInfix(Infix *what)
    {
        if (what->name == ":" || what->name == "as" || what->name == "when")
            return what->left->Do(this);
        Tree *left  = what->left->Do(this);
        Tree *right = what->right->Do(this);
        if (left != what->left || right != what->right)
            return new Infix(what->name, left, right, what->Position());
        return what;
    }

    Tree *  DoBlock(Block *what)
    {
        Tree *chld = what->child->Do(this);
        if (chld != what->child)
            return new Block(chld,what->opening,what->closing,what->Position());
        return what;
    }

    Context_p context;
};


static Tree *formTypeCheck(Context *context, Tree *shape, Tree *value)
// ----------------------------------------------------------------------------
//    Check a value against a type shape
// ----------------------------------------------------------------------------
{
    // Strip outermost block if there is one
    if (Block *block = shape->AsBlock())
        shape = block->child;

    // Check if the shape matches
    Context_p locals = new Context(context);
    EvalCache cache;
    TreeList  args;
    Bindings  bindings(context, locals, value, cache, args);
    if (!shape->Do(bindings))
    {
        IFTRACE(eval)
            std::cerr << "TYPECHECK: shape mismatch for " << value << "\n";
        return NULL;
    }

    // Reconstruct the resulting value from the shape
    Expansion expand(locals);
    value = shape->Do(expand);

    // The value is associated to the symbols we extracted
    IFTRACE(eval)
        std::cerr << "TYPECHECK: shape match for " << value << "\n";
    return makeClosure(locals, value);
}


Tree *TypeCheck(Context *scope, Tree *type, Tree *value)
// ----------------------------------------------------------------------------
//   Check if 'value' matches 'type' in the given context
// ----------------------------------------------------------------------------
{
    IFTRACE(eval)
        std::cerr << "TYPECHECK " << value << " against " << type << "\n";

    // Accelerated type check for the builtin or constructed types
    if (TypeCheckOpcode *builtin = type->GetInfo<TypeCheckOpcode>())
    {
        // If this is marked as builtin, check if the test passes
        if (Tree *converted = builtin->Check(scope, value))
        {
            IFTRACE(eval)
                std::cerr << "TYPECHECK " << value
                          << " as " << converted << "\n";
            return converted;
        }
    }
    else
    {
        // Check a type like 'type (X, Y)'
        if (Prefix *ptype = type->AsPrefix())
            if (Name *ptypename = ptype->left->AsName())
                if (ptypename->value == "type")
                    return formTypeCheck(scope, ptype->right, value);

        IFTRACE(eval)
            std::cerr << "TYPECHECK: no code for " << type
                      << " opcode is " << type->GetInfo<Opcode>()
                      << "\n";
    }


    // No direct or converted match, end of game
    IFTRACE(eval)
        std::cerr << "TYPECHECK " << value << " FAILED\n";
    return NULL;
}



// ============================================================================
//
//    Include the opcodes
//
// ============================================================================

#include "opcodes.h"
#include "interpreter.tbl"

XL_END
    
