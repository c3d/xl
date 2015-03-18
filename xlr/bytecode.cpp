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


#if 0
// ============================================================================
//
//   Evaluating a code sequence and variants
//
// ============================================================================

Code::Code(Scope *scope)
// ----------------------------------------------------------------------------
//   Build an empty list of instructions
// ----------------------------------------------------------------------------
    : scope(scope)
{}


Code::~Code()
// ----------------------------------------------------------------------------
//    Delete all instructions
// ----------------------------------------------------------------------------
{
    for (Ops::iterator o = ops.begin(); o != ops.end(); o++)
        (*o)->Delete();
}


bool Code::Run(Data &data)
// ----------------------------------------------------------------------------
//    Evaluate the code on the given data
// ----------------------------------------------------------------------------
{
    Data inner(data.locals);

    bool ok = true;
    Tree *self = data.Pop();
    Scope *scope = data.locals->CurrentScope();
    inner.Init(scope, self);
    
    for (Ops::iterator o = ops.begin(); ok && o != ops.end(); o++)
        ok = (*o)->Run(inner);

    if (ok)
    {
        Tree *result = inner.Pop();
        data.Push(result);
    }
    return ok;
}



// ============================================================================
// 
//    Data context
// 
// ============================================================================

void Data::Init(Scope *scope, Tree *src)
// ----------------------------------------------------------------------------
//   Initialize a data context to evaluate in the given scope
// ----------------------------------------------------------------------------
{
    locals = new Context(scope);
    locals->CreateScope();
    
    stack.clear();
    stack.push_back(src);
    self = src;
}


void Data::Push(Tree *t)
// ----------------------------------------------------------------------------
//    Push a data value on top of the stack
// ----------------------------------------------------------------------------
{
    stack.push_back(t);
}


Tree *Data::Pop()
// ----------------------------------------------------------------------------
//    Pop a value from the top of the stack
// ----------------------------------------------------------------------------
{
    XL_ASSERT(stack.size());
    Tree *result = stack.back();
    stack.pop_back();
    return result;
}


uint Data::Bind(Tree *t)
// ----------------------------------------------------------------------------
//    Add a variable to the locals, return its index
// ----------------------------------------------------------------------------
{
    uint index = vars.size();
    vars.push_back(t);
    return index;
}


Tree *Data::Self()
// ----------------------------------------------------------------------------
//   Put self on top of stack and return it
// ----------------------------------------------------------------------------
{
    stack.push_back(self);
    return self;
}


void Data::MustEvaluate(uint index, bool updateContext)
// ----------------------------------------------------------------------------
//   Check if we need to evaluate item at given index
// ----------------------------------------------------------------------------
{
    XL_ASSERT(index < evals.size());
    XL_ASSERT(stack.size());

    Tree *test = evals[index];
    if (!test)
    {
        test = stack.back();
        test = EvaluateClosure(context, test);
        evals[index] = test;
    }
    if (updateContext)
        if (Tree *inside = IsClosure(test, &context))
            test = inside;
    stack.back() = test;
}



// ============================================================================
// 
//   Operations
// 
// ============================================================================

struct DispatchOp : Code
// ----------------------------------------------------------------------------
//    Dispatch evaluation to the first op that matches
// ----------------------------------------------------------------------------
{
    virtual bool Run(Data &data)
    {
        Tree    *self = data.Pop();
        Context *context = data.context;
        Scope   *scope = data.locals->CurrentScope();
        
        for (Ops::iterator o = ops.begin(); o != ops.end(); o++)
        {
            Data inner(context);
            inner.Init(scope, self);
            if ((*o)->Run(inner))
            {
                Tree *result = inner.Pop();
                data.Push(result);
                return true;
            }
        }
        return false;
    }
};


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
        Tree * closed = MakeClosure(data.context, test);
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
        if (Tree *inside = IsClosure(result, &data.context))
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
        if (Tree *inside = IsClosure(result, NULL))
            result = inside;
        data.Push(result);
        return true;
        
    }
};


struct ConstOp : Op
// ----------------------------------------------------------------------------
//    Push a constant on the stack
// ----------------------------------------------------------------------------
{
    ConstOp(Tree *tree): tree(tree) {}
    Tree_p tree;

    bool Run(Data &data)
    {
        data.Push(tree);
        return true;
    }
};


struct VariableOp : Op
// ----------------------------------------------------------------------------
//    Push a variable on the stack
// ----------------------------------------------------------------------------
{
    VariableOp(Infix *decl): decl(decl) {}
    Infix_p decl;

    bool Run(Data &data)
    {
        data.Push(decl->right);
        return true;
    }
};


struct StoreOp : Op
// ----------------------------------------------------------------------------
//    Store top of stack in a variable
// ----------------------------------------------------------------------------
{
    StoreOp(Infix *decl): decl(decl) {}
    Infix_p decl;

    bool Run(Data &data)
    {
        Tree *value = data.Pop();
        decl->right = value;
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
        Tree *result = MakeClosure(data.locals, body);
        data.Push(result);
        return true;
    }
};


struct DebugOp : Op
{
    DebugOp(text t): message(t) {}
    text message;
    bool Run(Data &data)
    {
        std::cerr << message << data.stack << "\n";
        return true;           
    }
};

#endif
