// *****************************************************************************
// interpreter.cpp                                                    XL project
// *****************************************************************************
//
// File description:
//
//    An interpreter for XL that does not rely on LLVM at all
//
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
// XL is free software: you can redistribute it and/or modify
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

#include "interpreter.h"
#include "gc.h"
#include "info.h"
#include "errors.h"
#include "renderer.h"
#include "basics.h"
#include "runtime.h"

#include <cmath>
#include <algorithm>

RECORDER(interpreter, 128, "Interpreted evaluation of XL code");
RECORDER(interpreter_lazy, 64, "Interpreter lazy evaluation");
RECORDER(interpreter_eval, 128, "Primary evaluation entry point");
RECORDER(interpreter_typecheck, 64, "Type checks");

XL_BEGIN
// ============================================================================
//
//   Options
//
// ============================================================================

namespace Opt
{
IntegerOption   stackDepth("stack_depth",
                           "Maximum stack depth for interpreter",
                           1000, 25, 25000);
}



// ============================================================================
//
//   Evaluation cache - Recording what has been evaluated and how
//
// ============================================================================

typedef std::map<Tree_p, Tree_p> EvalCache;




// ============================================================================
//
//   Interpeter main entry points
//
// ============================================================================

Interpreter::Interpreter()
// ----------------------------------------------------------------------------
//   Constructor for interpreter
// ----------------------------------------------------------------------------
{
    record(interpreter, "Created interpreter %p", this);
}


Interpreter::~Interpreter()
// ----------------------------------------------------------------------------
//   Destructor for interpreter
// ----------------------------------------------------------------------------
{
    record(interpreter, "Destroyed interpreter %p", this);
}


Tree *Interpreter::Evaluate(Scope *scope, Tree *what)
// ----------------------------------------------------------------------------
//    Evaluate 'what', finding the final, non-closure result
// ----------------------------------------------------------------------------
{
    Context_p context = new Context(scope);
    Tree *result = EvaluateClosure(context, what);
    if (Tree *inside = IsClosure(result, nullptr))
        result = inside;
    return result;
}


bool Interpreter::TypeAnalysis(Scope *scope, Tree *source)
// ----------------------------------------------------------------------------
//    No type analysis for the interpreter
// ----------------------------------------------------------------------------
{
    return true;
}



// ============================================================================
//
//    Primitive cache for 'opcode' and 'C' bindings
//
// ============================================================================

Opcode *Interpreter::SetInfo(Infix *decl, Opcode *opcode)
// ----------------------------------------------------------------------------
//    Create a new info for the given callback
// ----------------------------------------------------------------------------
{
    decl->right->SetInfo<Opcode>(opcode);
    return opcode;
}


Opcode *Interpreter::OpcodeInfo(Infix *decl)
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
            if (name->value == "builtin")
                if (Name *opName = prefix->right->AsName())
                    if (Opcode *opcode = Opcode::Find(prefix, opName->value))
                        return SetInfo(decl, opcode->Clone());

    return nullptr;
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
          test(test), cache(cache), resultType(nullptr), args(args) {}

    // Tree::Do interface
    bool  Do(Integer *what);
    bool  Do(Real *what);
    bool  Do(Text *what);
    bool  Do(Name *what);
    bool  Do(Prefix *what);
    bool  Do(Postfix *what);
    bool  Do(Infix *what);
    bool  Do(Block *what);

    // Evaluation and binding of values
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


inline bool Bindings::Do(Integer *what)
// ----------------------------------------------------------------------------
//   The pattern contains an integer: check we have the same
// ----------------------------------------------------------------------------
{
    MustEvaluate();
    if (Integer *ival = test->AsInteger())
        if (ival->value == what->value)
            return true;
    Ooops("Integer $1 does not match $2", what, test);
    return false;
}


inline bool Bindings::Do(Real *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    MustEvaluate();
    if (Real *rval = test->AsReal())
        if (rval->value == what->value)
            return true;
    Ooops("Real $1 does not match $2", what, test);
    return false;
}


inline bool Bindings::Do(Text *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    MustEvaluate();
    if (Text *tval = test->AsText())
        if (tval->value == what->value)         // Do delimiters matter?
            return true;
    Ooops("Text $1 does not match $2", what, test);
    return false;
}


inline bool Bindings::Do(Name *what)
// ----------------------------------------------------------------------------
//   The pattern contains a name: bind it as a closure, no evaluation
// ----------------------------------------------------------------------------
{
    Save<Context_p> saveContext(context, context);

    // The test value may have been evaluated
    EvalCache::iterator found = cache.find(test);
    if (found != cache.end())
        test = (*found).second;

    // If there is already a binding for that name, value must match
    // This covers both a pattern with 'pi' in it and things like 'X+X'
    if (Tree *bound = locals->Bound(what))
    {
        MustEvaluate(true);
        bool result = Tree::Equal(bound, test);
        record(interpreter, "Arg check %t vs %t: %+s",
               test, bound, result ? "match" : "failed");
        if (!result)
            Ooops("Name $1 does not match $2", bound, test);
        return result;
    }

    record(interpreter, "In closure %t: %t = %t",
           context->Symbols(), what, test);
    BindClosure(what, test);
    return true;
}


bool Bindings::Do(Block *what)
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


bool Bindings::Do(Prefix *what)
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


bool Bindings::Do(Postfix *what)
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


bool Bindings::Do(Infix *what)
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
        Scope *scope = context->Symbols();
        Tree *type = MustEvaluate(context, what->right);
        Tree *checked = xl_typecheck(scope, type, test);
        if (!checked || type == XL::value_type)
        {
            MustEvaluate(type != XL::value_type);
            checked = xl_typecheck(scope, type, test);
        }
        if (checked)
        {
            Bind(name, checked);
            return true;
        }

        // Type mismatch
        Ooops("Type $1 does not contain $2", type, test);
        return false;
    }

    // Check if we have typed declarations, e.g. X+Y as integer
    if (IsTypeAnnotation(what))
    {
        if (resultType)
        {
            Ooops("Duplicate return type declaration $1", what);
            Ooops("Previously declared type was $1", resultType);
        }
        resultType = MustEvaluate(context, what->right);
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
        if (check == xl_true)
            return true;
        else if (check != xl_false)
            Ooops ("Invalid guard clause, $1 is not a boolean", check);
        else
            Ooops("Guard clause $1 is not verified", what->right);
        return false;
    }

    // In all other cases, we need an infix with matching name
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


void Bindings::MustEvaluate(bool updateContext)
// ----------------------------------------------------------------------------
//   Evaluate 'test', ensuring that each bound arg is evaluated at most once
// ----------------------------------------------------------------------------
{
    Tree *evaluated = cache[test];
    if (!evaluated)
    {
        evaluated = Interpreter::EvaluateClosure(context, test);
        cache[test] = evaluated;
        record(interpreter_lazy, "Test %t = new %t", test, evaluated);
    }
    else
    {
        record(interpreter_lazy, "Test %t = old %t", test, evaluated);
    }

    test = evaluated;
    if (updateContext)
    {
        if (Tree *inside = Interpreter::IsClosure(test, &context))
        {
            record(interpreter_lazy, "Encapsulate %t in closure %t",
                   test, inside);
            test = inside;
        }
    }
}


Tree *Bindings::MustEvaluate(Context *context, Tree *tval)
// ----------------------------------------------------------------------------
//   Ensure that each bound arg is evaluated at most once
// ----------------------------------------------------------------------------
{
    Tree *evaluated = cache[tval];
    if (!evaluated)
    {
        evaluated = Interpreter::EvaluateClosure(context, tval);
        cache[tval] = evaluated;
        record(interpreter_lazy, "Evaluate %t in context %t is new %t",
               tval, context, evaluated);
    }
    else
    {
        record(interpreter_lazy, "Evaluate %t in context %t is old %t",
               tval, context, evaluated);
    }
    if (Tree *inside = Interpreter::IsClosure(evaluated, nullptr))
    {
        record(interpreter_lazy, "Encapsulate %t in closure %t",
               evaluated, inside);
        evaluated = inside;
    }
    return evaluated;
}


RECORDER(bind, 64, "Bind values to names");
void Bindings::Bind(Name *name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a new binding in the current context, remember left and right
// ----------------------------------------------------------------------------
{
    record(bind, "Bind %t = %t", name, value);
    args.push_back(value);
    locals->Define(name, value);
}


void Bindings::BindClosure(Name *name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a new binding in the current context, preserving its environment
// ----------------------------------------------------------------------------
{
    value = Interpreter::MakeClosure(context, value);
    Bind(name, value);
}



// ============================================================================
//
//   Main evaluation loop for the interpreter
//
// ============================================================================

static Tree *error_result = nullptr;


static Tree *evalLookup(Scope *evalScope, Scope *declScope,
                        Tree *self, Infix *decl, void *ec)
// ----------------------------------------------------------------------------
//   Calllback function to check if the candidate matches
// ----------------------------------------------------------------------------
{
    static uint depth = 0;
    Save<uint> saveDepth(depth, depth+1);
    record(interpreter_eval, "Eval%u %t from %t", depth, self, decl->left);
    if (depth > Opt::stackDepth)
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
    Context_p locals  = nullptr;
    Tree *result = nullptr;

    // Check if the decl is an opcode or C binding
    Errors *errors = MAIN->errors;
    uint errCount = errors->Count();
    Opcode *opcode = Interpreter::OpcodeInfo(decl);
    if (errors->Count() != errCount)
        return nullptr;

    // If we lookup a name or a number, just return it
    Tree *defined = RewriteDefined(decl->left);
    Tree *resultType = tree_type;
    TreeList args;
    if (defined->IsLeaf())
    {
        // Must match literally, or we don't have a candidate
        if (!Tree::Equal(defined, self))
        {
            record(interpreter_eval, "Eval%u %t from constant %t: mismatch",
                   depth, self, decl->left);
            return nullptr;
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
            record(interpreter_eval, "Eval%u %t from %t: mismatch",
                   depth, self, decl->left);
            return nullptr;
        }
        if (bindings.resultType)
            resultType = bindings.resultType;
    }

    // Check if the right is "self"
    if (result == xl_self)
    {
        record(interpreter_eval, "Eval%u %t from %t is self",
               depth, self, decl->left);
        return self;
    }

    // Check if we have builtins (opcode or C bindings)
    if (opcode)
    {
        // Cached callback
        uint offset = args.size();
        std::reverse(args.begin(), args.end());
        args.push_back(decl->right);
        args.push_back(context->Symbols());
        Data data = &args[offset];
        opcode->Run(data);
        result = DataResult(data);
        record(interpreter_eval, "Eval%u %t opcode %+s, result %t",
               depth, self, opcode->OpID(), result);
        return result;
    }

    // Normal case: evaluate body of the declaration in the new context
    result = decl->right;
    if (resultType != tree_type)
        result = new Infix("as", result, resultType, self->Position());

    result = Interpreter::MakeClosure(locals, result);
    record(interpreter_eval, "Eval%u %t in context %t = %t",
           depth, locals->Symbols(), self, result);
    return result;
}


inline Tree *encloseResult(Context *context, Scope *old, Tree *what)
// ----------------------------------------------------------------------------
//   Encapsulate result with a closure if context is not evaluation context
// ----------------------------------------------------------------------------
{
    if (context->Symbols() != old)
        what = Interpreter::MakeClosure(context, what);
    return what;
}


Tree *Interpreter::Instructions(Context_p context, Tree_p what)
// ----------------------------------------------------------------------------
//   Evaluate the input tree once declarations have been processed
// ----------------------------------------------------------------------------
{
    Tree_p      result = what;
    Scope_p     originalScope = context->Symbols();

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
            if (Tree *inside = IsClosure(eval, &context))
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
            if (Tree *closed = IsClosure(what, &context))
            {
                what = closed;
                continue;
            }

            // If we have a name on the left, lookup name and start again
            Prefix *pfx = (Prefix *) (Tree *) what;
            Tree   *callee = pfx->left;

            // Check if we had something like '(X->X+1) 31' as closure
            Context_p calleeContext = nullptr;
            if (Tree *inside = IsClosure(callee, &calleeContext))
                callee = inside;

            if (Name *name = callee->AsName())
                // A few cases where we don't interpret the result
                if (name->value == "matching"   ||
                    name->value == "extern")
                    return what;

            // This variable records if we evaluated the callee
            Tree *newCallee = nullptr;
            Tree *arg = pfx->right;

            // If we have an infix on the left, check if it's a single rewrite
            if (Infix *lifx = IsConstantDeclaration(callee))
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
                if (Tree *inside = IsClosure(newCallee, &context))
                {
                    what = arg;
                    // Check if we have a single definition on the left
                    if (Infix *ifx = IsConstantDeclaration(inside))
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
            if (name == "is")
            {
                // Declarations evaluate last non-declaration result, or self
                return encloseResult(context, originalScope, result);
            }

            // Check type matching
            if (name == "as")
            {
                Scope *scope = context->Symbols();
                result = xl_typecheck(scope, infix->right, infix->left);
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
                IsClosure(left, &context);
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


Tree *Interpreter::EvaluateClosure(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//    Evaluate 'what', possibly returned as a closure in case not in 'context'
// ----------------------------------------------------------------------------
{
    // Create scope for declarations, and evaluate in this context
    Tree_p result = what;
    Errors *errors = MAIN->errors;
    uint errCount = errors->Count();
    if (context->ProcessDeclarations(what) && errCount == errors->Count())
        result = Instructions(context, what);

    // This is a safe point for checking collection status
    GarbageCollector::SafePoint();

    return result;
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

    Tree *  Do(Tree *what)
    {
        return what;
    }
    Tree *  Do(Name *what)
    {
        if (Tree *bound = context->Bound(what))
        {
            if (Tree *eval = Interpreter::IsClosure(bound, nullptr))
                bound = eval;
            return bound;
        }
        return what;
    }
    Tree *  Do(Prefix *what)
    {
        Tree *left  = what->left->Do(this);
        Tree *right = what->right->Do(this);
        if (left != what->left || right != what->right)
            return new Prefix(left, right, what->Position());
        return what;
    }
    Tree *  Do(Postfix *what)
    {
        Tree *left  = what->left->Do(this);
        Tree *right = what->right->Do(this);
        if (left != what->left || right != what->right)
            return new Postfix(left, right, what->Position());
        return what;

    }
    Tree *  Do(Infix *what)
    {
        if (IsTypeAnnotation(what) || IsCondition(what))
            return what->left->Do(this);
        Tree *left  = what->left->Do(this);
        Tree *right = what->right->Do(this);
        if (left != what->left || right != what->right)
            return new Infix(what->name, left, right, what->Position());
        return what;
    }

    Tree *  Do(Block *what)
    {
        Tree *chld = what->child->Do(this);
        if (chld != what->child)
            return new Block(chld,what->opening,what->closing,what->Position());
        return what;
    }

    Context_p context;
};


static Tree *formTypeCheck(Scope *scope, Tree *shape, Tree *value)
// ----------------------------------------------------------------------------
//    Check a value against a type shape
// ----------------------------------------------------------------------------
{
    // Strip outermost block if there is one
    if (Block *block = shape->AsBlock())
        shape = block->child;

    // Check if the shape matches
    Context_p context = new Context(scope);
    Context_p locals = new Context(context);
    EvalCache cache;
    TreeList  args;
    Bindings  bindings(context, locals, value, cache, args);
    if (!shape->Do(bindings))
    {
        record(interpreter_typecheck, "Shape of tree %t does not match %t",
               value, shape);
        return nullptr;
    }

    // Reconstruct the resulting value from the shape
    Expansion expand(locals);
    value = shape->Do(expand);

    // The value is associated to the symbols we extracted
    record(interpreter_typecheck, "Shape of tree %t matches %t", value, shape);
    return Interpreter::MakeClosure(locals, value);
}


Tree *Interpreter::TypeCheck(Scope *scope, Tree *type, Tree *value)
// ----------------------------------------------------------------------------
//   Check if 'value' matches 'type' in the given context
// ----------------------------------------------------------------------------
{
    record(interpreter_typecheck, "Check %t against %t in scope %t",
           value, type, scope);

    // Accelerated type check for the builtin or constructed types
    if (TypeCheckOpcode *builtin = type->GetInfo<TypeCheckOpcode>())
    {
        // If this is marked as builtin, check if the test passes
        if (Tree *converted = builtin->Check(scope, value))
        {
            record(interpreter_typecheck, "Check %t converted as %t",
                   value, converted);
            return converted;
        }
    }
    else
    {
        // Check a type like 'matching (X, Y)'
        if (Prefix *ptype = type->AsPrefix())
            if (Name *ptypename = ptype->left->AsName())
                if (ptypename->value == "matching")
                    return formTypeCheck(scope, ptype->right, value);

        record(interpreter_typecheck, "No code for %t, opcode is %O",
               type, type->GetInfo<Opcode>());
    }


    // No direct or converted match, end of game
    record(interpreter_typecheck, "Type checking %t failed", value);
    return nullptr;
}



// ============================================================================
//
//    Include the opcodes
//
// ============================================================================

#include "opcodes.h"
#include "interpreter.tbl"

XL_END
