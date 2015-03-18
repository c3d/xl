#ifndef INTERPRETER_H
#define INTERPRETER_H
// ****************************************************************************
//  interpreter.h                                                  XLR project
// ****************************************************************************
//
//   File Description:
//
//     A fully interpreted mode for XLR, that does not rely on LLVM at all
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


XL_BEGIN

// ============================================================================
// 
//    Main entry points
// 
// ============================================================================

Tree *Evaluate(Context *context, Tree *code);
Tree *EvaluateClosure(Context *context, Tree *code);
Tree *TypeCheck(Context *scope, Tree *type, Tree *value);



// ============================================================================
// 
//    Closure management
// 
// ============================================================================

Tree *IsClosure(Tree *value, Context_p *context);
Tree *MakeClosure(Context *context, Tree *value);



// ============================================================================
// 
//   Interpreter operations
// 
// ============================================================================

struct Op;                      // Individual operation
struct Code;                    // Info encapsulating code for a tree
struct Data;                    // Data associated with code evaluation

struct Op
// ----------------------------------------------------------------------------
//    Evaluate a single instruction in a 'Code' instance
// ----------------------------------------------------------------------------
{
                        Op()                    {}
    virtual             ~Op()                   {}
    virtual void        Delete()                { delete this; }
    virtual bool        Run(Data &data)         = 0;
};
typedef std::vector<Op *> Ops;


struct Code : Info, Op
// ----------------------------------------------------------------------------
//    Sequence of ops to evaluate an expression
// ----------------------------------------------------------------------------
{
                        Code(Scope *scope);
                        ~Code();

    virtual bool        Run(Data &data);
    void                Add(Op *op)     { ops.push_back(op); }

public:
    Scope_p             scope;  // Scope of declaration for the code
    Ops                 ops;    // Sequence of operations
};


struct Data
// ----------------------------------------------------------------------------
//    Data storage for evaluating an expression
// ----------------------------------------------------------------------------
{
                        Data(Context *ctx) : context(ctx), locals(NULL) {}

    void                Init(Scope *scope, Tree *src);
    void                Push(Tree *t);
    Tree *              Pop();
    uint                Bind(Tree *t);
    Tree *              Self();
    void                MustEvaluate(uint index, bool updateContext);

public:
    Tree_p              self;    // The tree being evaluated
    Context_p           context; // Evaluation context
    Context_p           locals;  // Local declarations
    TreeList            stack;   // Evaluation stack
    TreeList            vars;    // Local variables, beginning with args
    TreeList            evals;   // Partial evaluations
};



// ============================================================================
//
//    Closure management (keeping scoping information with values)
//
// ============================================================================

struct ClosureInfo : Info
// ----------------------------------------------------------------------------
//   Mark a given Prefix as a closure
// ----------------------------------------------------------------------------
{};


inline Tree *IsClosure(Tree *tree, Context_p *context)
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


inline Tree *MakeClosure(Context *ctx, Tree *value)
// ----------------------------------------------------------------------------
//   Create a closure encapsulating the current context
// ----------------------------------------------------------------------------
{
    Context_p context = ctx;

retry:
    kind valueKind = value->Kind();

    if (valueKind >= NAME || context->HasRewritesFor(valueKind))
    {
        if (valueKind == NAME)
        {
            if (Tree *bound = context->Bound(value))
            {
                if (Tree *inside = IsClosure(bound, &context))
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
//    Inline implementations for main entry points
// 
// ============================================================================

inline Tree *Evaluate(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//    Evaluate 'what', finding the final, non-closure result
// ----------------------------------------------------------------------------
{
    Tree *result = EvaluateClosure(context, what);
    if (Tree *inside = IsClosure(result, NULL))
        result = inside;
    return result;
}


XL_END

#endif // INTERPRETER_H
