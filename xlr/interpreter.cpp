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

typedef std::map<Tree_p, Tree_p>        EvalCache;


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
    decl->SetInfo<Opcode>(opcode);
    return opcode;
}


static inline Opcode *opcodeInfo(Infix *decl)
// ----------------------------------------------------------------------------
//    Check if we have an opcode in the definition
// ----------------------------------------------------------------------------
{
    Opcode *info = decl->GetInfo<Opcode>();
    if (info)
        return info;

    // Check if the declaration is something like 'X -> opcode Foo'
    // If so, lookup 'Foo' in the opcode table the first time to record it
    if (Prefix *prefix = decl->right->AsPrefix())
    {
        if (Name *name = prefix->left->AsName())
        {
            if (name->value == "opcode")
            {
                if (Name *opcodeName = prefix->right->AsName())
                    if (Opcode *opcode = Opcode::Find(opcodeName->value))
                        return setInfo(decl, opcode);
                Ooops("Invalid primitive $1", prefix->right);
            }
        }
    }
        

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
             Tree *test, EvalCache &cache,
             Opcode *opcode)
        : context(context), locals(locals),
          test(test), cache(cache), opcode(opcode), resultType(NULL) {}

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
    Tree *MustEvaluate(Tree *tval);
    void  Bind(Name *name, Tree *value);
    void  BindClosure(Name *name, Tree *value);


private:
    Context    *context;
    Context    *locals;
    Tree       *test;
    EvalCache  &cache;

public:
    Opcode *    opcode;
    TreeList    args;
    Tree *      resultType;
};


inline bool Bindings::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   The pattern contains an integer: check we have the same
// ----------------------------------------------------------------------------
{
    test = MustEvaluate(test);
    if (Integer *ival = test->AsInteger())
        if (ival->value == what->value)
            return true;
    return false;
}


inline bool Bindings::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    test = MustEvaluate(test);
    if (Real *rval = test->AsReal())
        if (rval->value == what->value)
            return true;
    return false;
}


inline bool Bindings::DoText(Text *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    test = MustEvaluate(test);
    if (Text *tval = test->AsText())
        if (tval->value == what->value)         // Do delimiters matter?
            return true;
    return false;
}


inline bool Bindings::DoName(Name *what)
// ----------------------------------------------------------------------------
//   The pattern contains a name: bind it as a closure, no evaluation
// ----------------------------------------------------------------------------
{
    // The test value may have been evaluated
    EvalCache::iterator found = cache.find(test);
    if (found != cache.end())
        test = (*found).second;

    // If there is already a binding for that name, value must match
    // This covers both a pattern with 'pi' in it and things like 'X+X'
    if (Tree *bound = locals->Bound(what))
    {
        Tree *eval = MustEvaluate(test);
        bool result = Tree::Equal(bound, eval);
        IFTRACE(eval)
            std::cerr << "  ARGCHECK: "
                      << test << "=" << eval
                      << " vs " << bound
                      << (result ? " MATCH" : " FAILED")
                      << "\n";
        return result;
    }

    IFTRACE(eval)
        std::cerr << "  CLOSURE " << what << "=" << test << "\n";
    BindClosure(what, test);
    return true;
}


bool Bindings::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   The pattern contains a block: look inside
// ----------------------------------------------------------------------------
{
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
                    test = pfx->right;
                    return what->right->Do(this);
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
                if (name->value == testName->value)
                {
                    test = pfx->left;
                    return what->left->Do(this);
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
    return false;
}


bool Bindings::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   The complicated case: various declarations
// ----------------------------------------------------------------------------
{
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
        Tree *type = MustEvaluate(what->right);
        Tree *value = type == tree_type ? test : MustEvaluate(test);
        Tree *checked = TypeCheck(context, type, value);
        if (checked)
        {
            Bind(name, value);
            return true;
        }

        // Type mismatch
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
        resultType = MustEvaluate(what->right);
        return what->left->Do(this);
    }

    // Check if we have a guard clause
    if (what->name == "when")
    {
        // It must pass the rest (need to bind values first)
        if (!what->left->Do(this))
            return false;

        // Here, we need to evaluate in the local context, not eval one
        Tree *check = Evaluate(locals, what->right);
        if (check == xl_true)
            return true;
        else if (check != xl_false)
            Ooops ("Invalid guard clause, $1 is not a boolean", check);
        return false;
    }

    // In all other cases, we need an infix with matching name
    Infix *ifx = test->AsInfix();
    if (!ifx)
    {
        test = MustEvaluate(test);
        ifx = test->AsInfix();
    }
    if (ifx)
    {
        if (ifx->name != what->name)
            return false;

        test = ifx->left;
        if (!what->left->Do(this))
            return false;
        test = ifx->right;
        if (what->right->Do(this))
            return true;
    }

    // Mismatch
    return false;
}


Tree *Bindings::MustEvaluate(Tree *tval)
// ------------------------------------------------------------------------
//   Ensure that each bound arg is evaluated at most once
// ------------------------------------------------------------------------
{
    Tree *evaluated = cache[tval];
    if (!evaluated)
    {
        evaluated = Evaluate(context, tval);
        cache[tval] = evaluated;
    }
    return evaluated;
}


void Bindings::Bind(Name *name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a new binding in the current context, remember left and right
// ----------------------------------------------------------------------------
{
    IFTRACE(eval)
        std::cerr << "  BIND " << name << "=" << ShortTreeForm(value) <<"\n";
    if (opcode)
        args.push_back(value);
    locals->Define(name, value);
}


void Bindings::BindClosure(Name *name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a new binding in the current context, preserving its environment
// ----------------------------------------------------------------------------
{
    if (context->HasRewritesFor(value->Kind()))
    {
        Scope *scope = context->CurrentScope();
        value = new Prefix(scope, value);

        ClosureInfo *closureMarker = new ClosureInfo;
        value->SetInfo(closureMarker);
    }
    Bind(name, value);
}




// ============================================================================
//
//   Main evaluation loop for the interpreter
//
// ============================================================================

static Tree *evalLookup(Scope *evalScope, Scope *declScope,
                        Tree *self, Infix *decl, void *ec)
// ----------------------------------------------------------------------------
//   Calllback function to check if the candidate matches
// ----------------------------------------------------------------------------
{
    static uint id = 0;
    IFTRACE(eval)
        std::cerr << "EVAL" << ++id << "(" << self
                  << ") from " << decl->left << "\n";

    // Retrieve the evaluation cache for arguments
    EvalCache *cache = (EvalCache *) ec;

    // Create the scope for evaluation and local bindings
    Context_p context = new Context(evalScope);
    Context_p locals = new Context(declScope);
    locals->CreateScope();

    // Check if the decl is an opcode or C binding
    Opcode *opcode = opcodeInfo(decl);
    
    // Check bindings of arguments to declaration, exit if fails
    Bindings  bindings(context, locals, self, *cache, opcode);
    if (!decl->left->Do(bindings))
    {
        IFTRACE(eval)
            std::cerr << "EVAL" << id-- << "(" << self
                      << ") from " << decl->left
                      << " MISMATCH\n";
        return NULL;
    }

    // Check if the right is "self"
    if (decl->right == xl_self)
    {
        IFTRACE(eval)
            std::cerr << "EVAL" << id-- << "(" << self
                      << ") from " << decl->left
                      << " SELF\n";
        return self;
    }

    // Check if we have builtins (opcode or C bindings)
    if (opcode)
    {
        // Cached callback
        Tree *result = opcode->Invoke(locals, self, bindings.args);
        IFTRACE(eval)
            std::cerr << "EVAL" << id-- << "(" << self
                      << ") OPCODE " << opcode->name
                      << "(" << bindings.args << ") = "
                      << result << "\n";
        return result;
    }

    // Normal case: evaluate body of the declaration in the new context
    Tree *result = Evaluate(locals, decl->right);
    IFTRACE(eval)
        std::cerr << "EVAL" << id-- << "(" << self
                  << ") = (" << result << ")\n";

    // If the bindings had a return type, check it
    if (bindings.resultType)
        if (!TypeCheck(context, bindings.resultType, result))
            Ooops("Result $1 does not have expected type $2",
                  result, bindings.resultType);

    return result;
}


inline Tree *lookup(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//   lookup for evaluation
// ----------------------------------------------------------------------------
{
    EvalCache cache;
    return context->Lookup(what, evalLookup, &cache);
}


static Tree *Instructions(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//   Evaluate the input tree once declarations have been processed
// ----------------------------------------------------------------------------
{
    Tree_p result = what;

    // Loop to avoid recursion for a few common cases, e.g. sequences, blocks
    while (what)
    {
        // Make sure garbage collections doesn't destroy key objects
        Tree_p    gcWhat    = what;
        Context_p gcContext = context;

        kind whatK = what->Kind();
        switch (whatK)
        {
        case INTEGER:
        case REAL:
        case TEXT:
        case NAME:
            // Check if there is a specific rewrite in current scope
            if (context->HasRewritesFor(whatK))
            {
                Rewrite_p rw;
                Scope_p   scope;

                if (Tree *found = context->Bound(what, true, &rw, &scope))
                {
                    // If the declaration has an opcode, evaluate it
                    if (Opcode *opcode = found->GetInfo<Opcode>())
                    {
                        TreeList noArgs;
                        Tree *result = opcode->Invoke(context, found, noArgs);
                        return result;
                    }

                    // Check if we need to keep evaluating
                    if (found->Kind() > NAME)
                    {
                        // If lookup returned a closure, need to evaluate it
                        if (Prefix *pfx = found->AsPrefix())
                        {
                            if (Prefix *lpfx = pfx->left->AsPrefix())
                            {
                                if (pfx->GetInfo<ClosureInfo>())
                                {
                                    // We normally have a scope on the left
                                    scope = lpfx;
                                    context = new Context(scope);
                                    what = pfx->right;
                                    continue;
                                }
                            }
                        }

                        // Keep evaluating in the scope where it was defined
                        context = new Context(scope);
                        what = found;
                        continue;
                    }

                    // Otherwise, we are done here
                    return found;
                }
            }
            return what;

        case BLOCK:
        {
            // Check if there is a block form
            if (Tree *eval = lookup(context, what))
                return eval;

            // Otherwise, evaluate child in a new context
            context = new Context(context);
            what = ((Block *) what)->child;
            context->ProcessDeclarations(what);
            continue;
        }

        case PREFIX:
        {
            // Check if there is a form that matches
            if (Tree *eval = lookup(context, what))
                return eval;

            // Calling with an expression or scope on the left
            Prefix *pfx = (Prefix *) what;
            Tree *callee = pfx->left;
            Tree *arg = pfx->right;

            // Strip away blocks
            while (Block *block = callee->AsBlock())
                callee = block->child;

            // If we have a name on the left, lookup name and start again
            if (Name *name = callee->AsName())
            {
                // A few cases where we don't interpret the result
                if (name->value == "type"   ||
                    name->value == "extern" ||
                    name->value == "data")
                    return what;

                if (Tree *found = context->Bound(name))
                {
                    if (found != name)
                    {
                        what = new Prefix(found, arg, pfx->Position());
                        continue;
                    }
                }
            }

            // If we have a prefix on the left, check if it's a closure
            if (Prefix *lpfx = callee->AsPrefix())
            {
                if (pfx->GetInfo<ClosureInfo>())
                {
                    // We normally have a scope on the left
                    Scope *scope = lpfx;
                    context = new Context(scope);
                    what = arg;
                    continue;
                }
            }

            // This variable records if we evaluated the callee
            Tree *newCallee = NULL;

            // If we have an infix on the left, can be a function or sequence
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

                if (lifx->name == ";" || lifx->name == "\n")
                {
                    Context *newContext = new Context(context);
                    if (!newContext->ProcessDeclarations(lifx))
                    {
                        // No instructions on left: evaluate in that context
                        context = newContext;
                        what = arg;
                        continue;
                    }

                    // If we had instructions in the callee, evaluate the body
                    newCallee = Instructions(newContext, callee);
                }
            }

            // Other cases: evaluate the callee, and if it changed, retry
            if (!newCallee)
            {
                Context *newContext = new Context(context);
                newCallee = Evaluate(newContext, callee);
            }

            if (newCallee != callee)
            {
                what = new Prefix(newCallee, arg, pfx->Position());
                continue;
            }

            // If we get there, we didn't find anything interesting to do
            return what;
        }

        case POSTFIX:
        {
            // Check if there is a form that matches
            if (Tree *eval = lookup(context, what))
                return eval;
            return what;
        }

        case INFIX:
        {
            Infix *infix = (Infix *) what;
            text name = infix->name;

            // Check sequences
            if (name == ";" || name == "\n")
            {
                // Sequences: evaluate left, then right
                Tree *left = Instructions(context, infix->left);
                if (left != infix->left)
                    result = left;
                what = infix->right;
                continue;
            }

            // Check declarations
            if (name == "->")
            {
                // Declarations evaluate last non-declaration result, or self
                return result;
            }

            // Check scoped reference
            if (name == ".")
            {
                Tree *target = Instructions(context, infix->left);
                Context *newContext = new Context(context);
                if (!newContext->ProcessDeclarations(target))
                {
                    // No instructions on left: evaluate in that context
                    context = newContext;
                    what = infix->right;
                    continue;
                }

                // If we had instructions in the left side, evaluate the body
                Instructions(newContext, infix->left);
                what = infix->right;
                continue;
            }

            // All other cases: evaluate
            if (Tree *eval = lookup(context, what))
                return eval;
            return what;
        }
        } // switch
    }// while(what)

    return result;
}


Tree *Evaluate(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//    Process declarations, then evaluate instructions
// ----------------------------------------------------------------------------
{
    // Create scope for declarations, and evaluate in this context
    Tree_p result = what;
    if (context->ProcessDeclarations(what))
        result = Instructions(context, what);

    // At end of evaluation, check if need to cleanup
    // GarbageCollector::Collect();
    return result;
}



// ============================================================================
// 
//     Type checking
// 
// ============================================================================

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
