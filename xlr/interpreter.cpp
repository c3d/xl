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

XL_BEGIN

typedef std::map<Tree_p, Tree_p>        EvalCache;


struct Bindings
// ----------------------------------------------------------------------------
//   Structure used to record bindings
// ----------------------------------------------------------------------------
{
    typedef bool value_type;

    Bindings(Context *context, Context *locals, Tree *test, EvalCache &cache)
        : context(context), locals(locals),
          test(test), cache(cache),
          left(xl_nil), right(xl_nil), resultType(NULL) {}

    // Tree::Do interface
    bool DoInteger(Integer *what);
    bool DoReal(Real *what);
    bool DoText(Text *what);
    bool DoName(Name *what);
    bool DoPrefix(Prefix *what);
    bool DoPostfix(Postfix *what);
    bool DoInfix(Infix *what);
    bool DoBlock(Block *what);

    // Evaluation and binding of values
    Tree *MustEvaluate(Tree *tval);
    void  Bind(Name *name, Tree *value);
    void  BindClosure(Name *name, Tree *value);


private:
    Context   *context;
    Context   *locals;
    Tree      *test;
    EvalCache &cache;

public:
    Tree      *left;
    Tree      *right;
    Tree      *resultType;
};


inline bool Bindings::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   The pattern contains an integer: check we have the same
// ----------------------------------------------------------------------------
{
    test = MustEvaluate(what);
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
    test = MustEvaluate(what);
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
    test = MustEvaluate(what);
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
    // If there is already a binding for that name, value must match
    // This covers both a pattern with 'pi' in it and things like 'X+X'
    if (Tree *bound = context->Bound(what))
        return Tree::Equal(bound, test);

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
        Tree *type = Evaluate(context, what->right);
        Tree *value = MustEvaluate(test);
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
        resultType = Evaluate(context, what->right);
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
    if (Infix *ifx = test->AsInfix())
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
    if (left == xl_nil)
        left = value;
    else if (right == xl_nil)
        right = value;
    locals->Define(name, value);
}


struct ClosureInfo : Info
// ----------------------------------------------------------------------------
//   Mark a given Prefix as a closure
// ----------------------------------------------------------------------------
{
    // We use a shared ClosureInfo marker for everybody, don't delete it
    virtual void Delete() {}
};


void Bindings::BindClosure(Name *name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a new binding in the current context, preserving scope
// ----------------------------------------------------------------------------
{
    if (context->HasRewritesFor(value->Kind()))
    {
        static ClosureInfo closureMarker;
        Scope *scope = context->CurrentScope();
        Prefix *closure = new Prefix(scope, value);
        closure->SetInfo(&closureMarker);
        value = closure;
    }
    Bind(name, value);
}




// ============================================================================
//
//    Primitive cache
//
// ============================================================================

struct OpcodeInfo : Info
// ----------------------------------------------------------------------------
//    Store the opcode info for the builtins
// ----------------------------------------------------------------------------
{
    typedef Tree *(*callback_fn) (Tree*left, Tree *right);
    OpcodeInfo(callback_fn cb): Invoke(cb) {}
    callback_fn Invoke;
};


// Create the opcode tables
#define BINARY(Name, ResTy, LeftTy, RightTy, Code)                      \
    static ResTy *opcode##Name(LeftTy *left, RightTy *right) { return Code; }

#define UNARY(Name, ResTy, LeftTy, Code)                        \
    static ResTy *opcode##Name(LeftTy *left) { return Code; }

#include "interpreter.tbl"


struct OpcodeCallback
// ----------------------------------------------------------------------------
//   The callback for a given opcode
// ----------------------------------------------------------------------------
{
    kstring                     name;
    OpcodeInfo::callback_fn     callback;
};


static OpcodeCallback OpcodeCallbacks[] =
// ----------------------------------------------------------------------------
//    The list of callbacks
// ----------------------------------------------------------------------------
{
#define BINARY(Name, ResTy, LeftTy, RightTy, Code) \
    { #Name, (OpcodeInfo::callback_fn) opcode##Name },
#define UNARY(Name, ResTy, LeftTy, Code)           \
    { #Name, (OpcodeInfo::callback_fn) opcode##Name },
#include "interpreter.tbl"
    { NULL, NULL }
};




// ============================================================================
//
//   Main evaluation loop for the interpreter
//
// ============================================================================

static Tree *evalLookup(Scope *scope,
                        Tree *form, Infix *decl, void *ec)
// ----------------------------------------------------------------------------
//   Calllback function to check if the candidate matches
// ----------------------------------------------------------------------------
{
    // Retrieve the evaluation cache for arguments
    EvalCache *cache = (EvalCache *) ec;

    // Create the scope for evaluation and local bindings
    Context_p context = new Context(scope);
    Context_p locals = new Context(context);

    // Check bindings of arguments to declaration, exit if fails
    Bindings  bindings(context, locals, form, *cache);
    if (!decl->left->Do(bindings))
        return NULL;

    // Check if the right is "self"
    if (decl->right == xl_self)
        return form;

    // Check if we have builtins (opcode or C bindings)
    if (OpcodeInfo *info = decl->GetInfo<OpcodeInfo>())
    {
        // Cached callback
        return info->Invoke(bindings.left, bindings.right);
    }
    else if (Prefix *prefix = decl->right->AsPrefix())
    {
        if (Name *name = prefix->left->AsName())
        {
            if (name->value == "opcode")
            {
                if (Name *opcode = prefix->right->AsName())
                {
                    for (OpcodeCallback *cb = OpcodeCallbacks; cb->name; cb++)
                    {
                        if (cb->name == opcode->value)
                        {
                            // First use, cache the result in decl
                            OpcodeInfo *info = new OpcodeInfo(cb->callback);
                            decl->SetInfo<OpcodeInfo>(info);
                            return cb->callback(bindings.left, bindings.right);
                        }
                    }
                }
            }
        }
    }

    // Normal case: evaluate body of the declaration in the new context
    Tree *result = Evaluate(locals, decl->right);

    // If the bindings had a return type, check it
    if (bindings.resultType)
        if (!TypeCheck(context, result, bindings.resultType))
            Ooops("Result $1 does not have expected type $2",
                  result, bindings.resultType);

    return result;
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
                if (Tree *found = context->Bound(what))
                    return found;
            return what;

        case BLOCK:
        {
            // Check if there is a block form
            EvalCache cache;
            if (Tree *eval = context->Lookup(what, evalLookup, &cache))
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
            EvalCache cache;
            if (Tree *eval = context->Lookup(what, evalLookup, &cache))
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
                if (lpfx->GetInfo<ClosureInfo>())
                {
                    // We normally have a scope on the right
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
            EvalCache cache;
            if (Tree *eval = context->Lookup(what, evalLookup, &cache))
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

            // Check assignments
            if (name == ":=")
            {
                Tree *target = Instructions(context, infix->left);
                Tree *value = Instructions(context, infix->right);
                Tree *assigned = context->Assign(target, value);
                return assigned;
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
            EvalCache cache;
            if (Tree *eval = context->Lookup(what, evalLookup, &cache))
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
    GarbageCollector::Collect();
    return result;
}


struct TypeCheckInfo : Info
// ----------------------------------------------------------------------------
//    A structure to quickly do the most common type checks
// ----------------------------------------------------------------------------
{
    TypeCheckInfo(kind k): expected(k) {}
    virtual bool Check(Tree *what)      { return what->Kind() == expected; }
    kind expected;
};


#define TYPE_CHECK(x, k, Code)                  \
struct x##TypeCheckInfo : TypeCheckInfo         \
{                                               \
    x##TypeCheckInfo(): TypeCheckInfo(k) {}     \
    virtual bool Check(Tree *what)              \
    {                                           \
        if (!TypeCheckInfo::Check(what))        \
            return false;                       \
        Code;                                   \
    }                                           \
}

TYPE_CHECK(boolean,     NAME,  return ((Name *)  what)->IsBoolean());
TYPE_CHECK(character,   TEXT,  return ((Text *)  what)->IsCharacter());
TYPE_CHECK(text,        TEXT,  return ((Text *)  what)->IsText());
TYPE_CHECK(symbol,      NAME,  return ((Name *)  what)->IsName());
TYPE_CHECK(operator,    NAME,  return ((Name *)  what)->IsOperator());
TYPE_CHECK(declaration, INFIX, return ((Infix *) what)->IsDeclaration());


Tree *TypeCheck(Context *scope, Tree *value, Tree *type)
// ----------------------------------------------------------------------------
//   Check if 'value' matches 'type' in the given context
// ----------------------------------------------------------------------------
{
    static bool inited = false;
    if (!inited)
    {
#define BASIC_TYPE_CHECK(type, kind)                                    \
        type##_type->SetInfo<TypeCheckInfo>(new TypeCheckInfo(kind))
#define EXTENDED_TYPE_CHECK(type)                                       \
        type##_type->SetInfo<type##TypeCheckInfo>(new type##TypeCheckInfo)

        BASIC_TYPE_CHECK(integer, INTEGER);
        BASIC_TYPE_CHECK(real,    REAL);
        BASIC_TYPE_CHECK(text,    TEXT);
        BASIC_TYPE_CHECK(name,    NAME);
        BASIC_TYPE_CHECK(block,   BLOCK);
        BASIC_TYPE_CHECK(prefix,  PREFIX);
        BASIC_TYPE_CHECK(postfix, POSTFIX);
        BASIC_TYPE_CHECK(infix,   INFIX);

        EXTENDED_TYPE_CHECK(boolean);
        EXTENDED_TYPE_CHECK(character);
        EXTENDED_TYPE_CHECK(text);
        EXTENDED_TYPE_CHECK(symbol);
        EXTENDED_TYPE_CHECK(operator);
        EXTENDED_TYPE_CHECK(declaration);

        inited = true;
    }

    // Accelerated type check for the builtin types
    if (TypeCheckInfo *builtin = type->GetInfo<TypeCheckInfo>())
    {
        // If this is marked as builtin, check if the test passes
        if (builtin->Check(value))
            return value;
    }

    // Check if the expression "Type contains Value" works
    Infix *typeCheck = new Infix("contains", type, value, value->Position());
    Tree *converted = Evaluate(scope, typeCheck);
    if (converted != value)
        return converted;

    // No direct or converted match, end of game
    return NULL;
}

XL_END
