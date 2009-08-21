// ****************************************************************************
//  context.cpp                     (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Description of an execution context
// 
// 
// 
// 
// 
// 
// 
// 
// ****************************************************************************
// This document is confidential.
// Do not redistribute without written permission
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include <iostream>
#include <cstdlib>
#include "context.h"
#include "tree.h"
#include "errors.h"
#include "options.h"
#include "renderer.h"
#include "opcodes.h"
#include "basics.h"

XL_BEGIN

// ============================================================================
// 
//   Namespace: symbols management
// 
// ============================================================================

Namespace::~Namespace()
// ----------------------------------------------------------------------------
//   Delete all included rewrites if necessary
// ----------------------------------------------------------------------------
{
    if (rewrites)
        delete rewrites;
}


void Namespace::EnterName(text name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a value in the namespace
// ----------------------------------------------------------------------------
{
    names[name] = value;
}


Variable *Namespace::AllocateVariable (text name, ulong treepos)
// ----------------------------------------------------------------------------
//   Enter a variable in the namespace
// ----------------------------------------------------------------------------
{
    Variable *variable = dynamic_cast<Variable *> (names[name]);
    if (variable)
        return variable;
    variable = new Variable(numVars++, treepos);
    names[name] = variable;
    return variable;
}


Rewrite *Namespace::EnterRewrite(Rewrite *rw)
// ----------------------------------------------------------------------------
//   Enter the given rewrite in the rewrites table
// ----------------------------------------------------------------------------
{
    if (rewrites)
        return rewrites->Add(rw);
    rewrites = rw;
    return rw;
}


void Namespace::Clear()
// ----------------------------------------------------------------------------
//   Clear all symbol tables
// ----------------------------------------------------------------------------
{
    symbol_table empty;
    names = empty;
    if (rewrites)
    {
        delete rewrites;
        rewrites = NULL;
    }
}



// ============================================================================
// 
//   Garbage collection
// 
// ============================================================================

ulong Context::gc_increment = 200;
ulong Context::gc_growth_percent = 200;
Context *Context::context = NULL;

struct GCAction : Action
// ----------------------------------------------------------------------------
//   Mark trees for garbage collection and compute active set
// ----------------------------------------------------------------------------
{
    GCAction (): alive() {}
    ~GCAction () {}

    bool Mark(Tree *what)
    {
        typedef std::pair<active_set::iterator, bool> inserted;
        inserted ins = alive.insert(what);
        return ins.second;
    }
    Tree *Do(Tree *what)
    {
        if (Mark(what))
            what->DoData(this);
        return what;
    }
    Tree *DoBlock(Block *what)
    {
        if (Mark(what))
        {
            what->DoData(this);
            Action::DoBlock(what);
        }
        return what;
    }
    Tree *DoInfix(Infix *what)
    {
        if (Mark(what))
        {
            what->DoData(this);
            Action::DoInfix(what);
        }
        return what;
    }
    Tree *DoPrefix(Prefix *what)
    {
        if (Mark(what))
        {
            what->DoData(this);
            Action::DoPrefix(what);
        }
        return what;
    }
    Tree *DoPostfix(Postfix *what)
    {
        if (Mark(what))
        {
            what->DoData(this);
            Action::DoPostfix(what);
        }
        return what;
    }
    active_set  alive;
};


void Context::CollectGarbage ()
// ----------------------------------------------------------------------------
//   Mark all active trees
// ----------------------------------------------------------------------------
{
    if (active.size() > gc_threshold)
    {
        GCAction gc;
        active_set::iterator a;
        symbol_table::iterator s;
        compile_cache::iterator cc;
        value_list::iterator v;
        ulong deletedCount = 0, activeCount = 0, nativeCount = 0;

        IFTRACE(memory)
            std::cerr << "Garbage collecting...";

        // Loop on all known contexts from here
        for (Context *c = this; c; c = c->Parent())
        {
            // Mark roots, names, rewrites and stack
            for (a = c->roots.begin(); a != c->roots.end(); a++)
                (*a)->Do(gc);
            for (s = c->names.begin(); s != c->names.end(); s++)
                (*s).second->Do(gc);
            if (c->rewrites)
                c->rewrites->Do(gc);
            if (c->run_stack)
            {
                value_list &values = c->run_stack->values;
                for (v = values.begin(); v != values.end(); v++)
                    (*v)->Do(gc);
            }
            for (cc = compiled.begin(); cc != compiled.end(); cc++)
                (*cc).second->Do(gc);
        }

        // Then delete all trees in active set that are no longer referenced
        for (a = active.begin(); a != active.end(); a++)
        {
            activeCount++;
            if (dynamic_cast<Native *> (*a))
            {
                nativeCount++;
            }
            else if (!gc.alive.count(*a))
            {
                deletedCount++;
                delete *a;
            }
        }
        active = gc.alive;
        gc_threshold = active.size() * gc_growth_percent / 100 + gc_increment;
        IFTRACE(memory)
            std::cerr << "done: Purged " << deletedCount
                      << " out of " << activeCount
                      << " and " << nativeCount << " natives, "
                      << " threshold " << gc_threshold << "\n";
    }
}



// ============================================================================
// 
//    Hash key for tree rewrite
// 
// ============================================================================

struct RewriteKey : Action
// ----------------------------------------------------------------------------
//   Compute a hashing key for a rewrite
// ----------------------------------------------------------------------------
{
    RewriteKey (ulong base = 0): key(base) {}
    ulong Key()  { return key; }

    ulong Hash(ulong id, text t)
    {
        ulong result = 0xC0DED;
        text::iterator p;
        for (p = t.begin(); p != t.end(); p++)
            result = (result * 0x301) ^ *p;
        return id | (result << 3);
    }
    ulong Hash(ulong id, ulong value)
    {
        return id | (value << 3);
    }

    Tree *DoInteger(Integer *what)
    {
        key = (key << 3) ^ Hash(0, what->value);
        return what;
    }
    Tree *DoReal(Real *what)
    {
        key = (key << 3) ^ Hash(1, *((ulong *) &what->value));
        return what;
    }
    Tree *DoText(Text *what)
    {
        key = (key << 3) ^ Hash(2, what->value);
        return what;
    }
    Tree *DoName(Name *what)
    {
        key = (key << 3) ^ Hash(3, what->value);
        return what;
    }

    Tree *DoBlock(Block *what)
    {
        key = (key << 3) ^ Hash(4, what->Opening() + what->Closing());
        return what;
    }
    Tree *DoInfix(Infix *what)
    {
        key = (key << 3) ^ Hash(5, what->name);
        return what;
    }
    Tree *DoPrefix(Prefix *what)
    {
        ulong old = key;
        key = 0; what->left->Do(this);
        key = (old << 3) ^ Hash(6, key);
        return what;
    }
    Tree *DoPostfix(Postfix *what)
    {
        ulong old = key;
        key = 0; what->right->Do(this);
        key = (old << 3) ^ Hash(7, key);
        return what;
    }
    Tree *Do(Tree *what)
    {
        key = (key << 3) ^ Hash(1, (ulong) what);
        return what;
    }

    ulong key;
};



// ============================================================================
// 
//    Tree rewrites actions
// 
// ============================================================================

struct TreeRewrite : Action
// ----------------------------------------------------------------------------
//   Apply all transformations to a tree
// ----------------------------------------------------------------------------
{
    TreeRewrite (Context *c): context(c) {}

    Tree *DoInteger(Integer *what)
    {
        return what;
    }
    Tree *DoReal(Real *what)
    {
        return what;
    }
    Tree *DoText(Text *what)
    {
        return what;
    }
    Tree *DoName(Name *what)
    {
        Tree *result = context->Name(what->value);
        if (result)
            return result;
        return what;
    }

    Tree *DoBlock(Block *what)
    {
        Tree *child = what->child->Do(this);
        return Block::MakeBlock(child, what->Opening(), what->Closing(),
                                what->Position());
    }
    Tree *DoInfix(Infix *what)
    {
        Tree *left = what->left->Do(this);
        Tree *right = what->right->Do(this);
        return new Infix(what->name, left, right, what->Position());
    }
    Tree *DoPrefix(Prefix *what)
    {
        Tree *left = what->left->Do(this);
        Tree *right = what->right->Do(this);
        return new Prefix(left, right, what->Position());
    }
    Tree *DoPostfix(Postfix *what)
    {
        Tree *left = what->left->Do(this);
        Tree *right = what->right->Do(this);
        return new Postfix(left, right, what->Position());
    }
    Tree *Do(Tree *what)
    {
        // Native trees and such are left unchanged
        return what;
    }

    Context *   context;
};



// ============================================================================
// 
//    Parameter match - Isolate parameters in an rewrite source
// 
// ============================================================================

struct ParameterMatch : Action
// ----------------------------------------------------------------------------
//   Check if two trees match, collect 'variables' and emit type test nodes
// ----------------------------------------------------------------------------
{
    ParameterMatch (Context *c): context(c), defined(NULL) {}

    virtual Tree *Do(Tree *what);
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);
    virtual Tree *DoNative(Native *what);

    Context * context;          // Context in which we test
    Tree *    defined;          // Tree beind defined, e.g. 'sin' in 'sin X'
};


Tree *ParameterMatch::Do(Tree *what)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *ParameterMatch::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *ParameterMatch::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *ParameterMatch::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *ParameterMatch::DoName(Name *what)
// ----------------------------------------------------------------------------
//    Identify the parameters being defined in the shape
// ----------------------------------------------------------------------------
{
    if (!defined)
    {
        // The first name we see must match exactly, e.g. 'sin' in 'sin X'
        defined = what;
        return what;
    }
    else
    {
        // Check if the name already exists, e.g. 'false' or 'A+A'
        if (Tree *existing = context->Name(what->value))
            return existing;

        // If first occurence of the name, enter it in symbol table
        Variable *v = context->AllocateVariable(what->value, what->Position());
        return v;
    }
}


Tree *ParameterMatch::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   Parameters in a block are in its child
// ----------------------------------------------------------------------------
{
    return what->child->Do(this);
}


Tree *ParameterMatch::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Check if we match an infix operator
// ----------------------------------------------------------------------------
{
    // Check if we match a type, e.g. 2 vs. 'K : integer'
    if (what->name == ":")
    {
        // Check the variable name, e.g. K in example above
        Name *varName = dynamic_cast<Name *> (what->left);
        if (!varName)
            return context->Error("Expected a name, got '$1' ", what->left);

        // Check if the name already exists
        if (Tree *existing = context->Name(varName->value))
            return context->Error("Typed name '$1' already exists as '$2'",
                                  what->left, existing);

        // Enter the name in symbol table
        Variable *variable = context->AllocateVariable(varName->value,
                                                       varName->Position());
        return variable;
    }

    // Otherwise, test left and right
    Tree *lr = what->left->Do(this);
    if (!lr)
        return NULL;
    Tree *rr = what->right->Do(this);
    if (!rr)
        return NULL;
    return what;
}


Tree *ParameterMatch::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   For prefix expressions, simply test left then right
// ----------------------------------------------------------------------------
{
    Tree *lr = what->left->Do(this);
    if (!lr)
        return NULL;
    Tree *rr = what->right->Do(this);
    if (!rr)
        return NULL;
    return what;
}


Tree *ParameterMatch::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//    For postfix expressions, simply test right, then left
// ----------------------------------------------------------------------------
{
    // Note that ordering is reverse compared to prefix, so that
    // the 'defined' names is set correctly
    Tree *rr = what->right->Do(this);
    if (!rr)
        return NULL;
    Tree *lr = what->left->Do(this);
    if (!lr)
        return NULL;
    return what;
}


Tree *ParameterMatch::DoNative(Native *what)
// ----------------------------------------------------------------------------
//   Should never be used
// ----------------------------------------------------------------------------
{
    return context->Error("Internal error: Native parameter '$1'", what);
}



// ============================================================================
// 
//    Parameter matching - Test input parameters
// 
// ============================================================================

typedef std::map<Tree *, ulong> eval_cache;

struct ArgumentMatch : Action
// ----------------------------------------------------------------------------
//   Check if two trees match, collect 'variables' and emit type test nodes
// ----------------------------------------------------------------------------
{
    ArgumentMatch (Tree *t, Context *l, Context *c, eval_cache &evals):
        locals(l), context(c),
        test(t), defined(NULL), code(NULL), end(NULL),
        expressions(evals) {}

    Tree *  Append(Tree *left, Tree *right);

    virtual Tree *Do(Tree *what);
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);
    virtual Tree *DoNative(Native *what);

    Tree *        Compile(Tree *source);

    Context *     locals;       // Context where we declare arguments
    Context *     context;      // Context in which we test values
    Tree *        test;         // Tree we test
    Tree *        defined;      // Tree beind defined, e.g. 'sin' in 'sin X'
    Tree *        code;         // Generated code
    Tree *        end;          // End label if necessary
    eval_cache &  expressions;  // Expressions needed for determination
};


Tree *ArgumentMatch::Compile(Tree *source)
// ----------------------------------------------------------------------------
//    Compile the source tree, and record we use the value in expr cache
// ----------------------------------------------------------------------------
{
    // Compile the code
    Tree *code = context->Compile(source, true);
    if (!code)
        return NULL; // No match

    // For leaves, delayed invokation doesn't help
    if (Leaf *leaf = dynamic_cast<Leaf *> (code))
        return leaf;

    // Identify stack slot for that expression
    ulong id = expressions.size();
    if (expressions.count(code) > 0)
        id = expressions[code];
    else
        expressions[code] = id;

    // Record a lazy-evaluation node to evaluate on demand
    EvaluateArgument *eval = new EvaluateArgument(code, id, source);
    return eval;
}


Tree *ArgumentMatch::Append(Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//   Append 'right' at end of native tree list starting at 'left'
// ----------------------------------------------------------------------------
// REVISIT: This looks suspiciously like CompileAction::Append
{
    // If left is not a native, e.g. 0, we just optimize it away
    Native *native = dynamic_cast<Native *> (left);
    if (!native)
        return right;
    return native->Append(right);
}


Tree *ArgumentMatch::Do(Tree *what)
// ----------------------------------------------------------------------------
//   Default is to return failure
// ----------------------------------------------------------------------------
{
    return NULL;
}


Tree *ArgumentMatch::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   An integer matches the exact value
// ----------------------------------------------------------------------------
{
    // Compile the test tree
    Tree *compiled = Compile(test);
    tree_position pos = test->Position();

    // If the tested tree is a leaf, it must be an integer with same value
    if (Leaf *leaf = dynamic_cast<Leaf *> (compiled))
    {
        if (Integer *it = dynamic_cast<Integer *> (leaf))
            if (it->value == what->value)
                return what;
        return NULL;
    }

    // Otherwise, we need to insert a dynamic integer test
    if (!end)
        end = new BranchTarget(pos);
    Tree *itest = new IntegerTest(compiled, what->value, NULL, end, pos);
    code = Append(code, itest);

    // And return success so far
    return what;
}


Tree *ArgumentMatch::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   A real matches the exact value
// ----------------------------------------------------------------------------
{
    // Compile the test tree
    Tree *compiled = Compile(test);
    tree_position pos = test->Position();

    // If the tested tree is a leaf, it must be a real with same value
    if (Leaf *leaf = dynamic_cast<Leaf *> (compiled))
    {
        if (Real *it = dynamic_cast<Real *> (leaf))
            if (it->value == what->value)
                return what;
        return NULL;
    }

    // Otherwise, we need to insert a dynamic real test
    if (!end)
        end = new BranchTarget(pos);
    Tree *itest = new RealTest(compiled, what->value, NULL, end, pos);
    code = Append(code, itest);

    // And return success so far
    return what;
}


Tree *ArgumentMatch::DoText(Text *what)
// ----------------------------------------------------------------------------
//   A text matches the exact value
// ----------------------------------------------------------------------------
{
    // Compile the test tree
    Tree *compiled = Compile(test);
    tree_position pos = test->Position();

    // If the tested tree is a leaf, it must be a real with same value
    if (Leaf *leaf = dynamic_cast<Leaf *> (compiled))
    {
        if (Text *it = dynamic_cast<Text *> (leaf))
            if (it->value == what->value)
                return what;
        return NULL;
    }

    // Otherwise, we need to insert a dynamic real test
    if (!end)
        end = new BranchTarget(pos);
    Tree *itest = new TextTest(compiled, what->value, NULL, end, pos);
    code = Append(code, itest);

    // And return success so far
    return what;
}


Tree *ArgumentMatch::DoName(Name *what)
// ----------------------------------------------------------------------------
//    Identify the parameters being defined in the shape
// ----------------------------------------------------------------------------
{
    if (!defined)
    {
        // The first name we see must match exactly, e.g. 'sin' in 'sin X'
        defined = what;
        if (Name *nt = dynamic_cast<Name *> (test))
            if (nt->value == what->value)
                return what;
        return NULL;
    }
    else
    {
        // Compile what we are testing against
        Tree *compiled = Compile(test);
        tree_position pos = test->Position();

        // If input tree doesn't compile by itself, we can't match this,
        if (!compiled)
            return NULL;

        // Check if the name already exists, e.g. 'false' or 'A+A'
        // If it does, we generate a run-time check to verify equality
        if (Tree *existing = locals->NamedTree(what->value))
        {
            // Insert a dynamic tree comparison test
            if (!end)
                end = new BranchTarget(pos);
            Tree *itest = new EqualityTest(compiled, existing, NULL, end, pos);
            code = Append(code, itest);

            // Return success at compile time
            return what;
        }

        // If first occurence of the name, enter it in symbol table
        locals->EnterName(what->value, compiled);
        return what;
    }
}


Tree *ArgumentMatch::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   Check if we match a block
// ----------------------------------------------------------------------------
{
    static Block       indent(NULL);
    static Parentheses paren(NULL);

    // Test if we exactly match the block, i.e. the reference is a block
    if (Block *bt = dynamic_cast<Block *> (test))
    {
        if (bt->Opening() == what->Opening() &&
            bt->Closing() == what->Closing())
        {
            test = bt->child;
            Tree *br = what->child->Do(this);
            test = bt;
            if (br)
                return br;
        }
    }

    // Otherwise, if the block is an indent or parenthese, optimize away
    if ((what->Opening() == paren.Opening() &&
         what->Closing() == paren.Closing()) ||
        (what->Opening() == indent.Opening() &&
         what->Closing() == indent.Closing()))
    {
        return what->child->Do(this);
    }
    
    return NULL;
}


Tree *ArgumentMatch::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Check if we match an infix operator
// ----------------------------------------------------------------------------
{
    if (Infix *it = dynamic_cast<Infix *> (test))
    {
        // Check if we match the tree, e.g. A+B vs 2+3
        if (it->name == what->name)
        {
            if (!defined)
                defined = what;
            test = it->left;
            Tree *lr = what->left->Do(this);
            test = it;
            if (!lr)
                return NULL;
            test = it->right;
            Tree *rr = what->right->Do(this);
            test = it;
            if (!rr)
                return NULL;
            return what;
        }
    }

    // Check if we match a type, e.g. 2 vs. 'K : integer'
    if (what->name == ":")
    {
        // Check the variable name, e.g. K in example above
        Name *varName = dynamic_cast<Name *> (what->left);
        if (!varName)
            return context->Error("Expected a name, got '$1' ",
                                  what->left);

        // Check if the name already exists
        if (Tree *existing = context->Name(varName->value))
            return context->Error("Name '$1' already exists as '$2'",
                                  what->left, existing);

        // Evaluate type expression, e.g. 'integer' in example above
        Tree *typeExpr = context->Compile(what->right);
        if (dynamic_cast<AnyType *> (typeExpr))
        {
            // If we know statically it's an any-type check, don't add checks
            // Since this was explicitly declared, still eager evaluation
            Tree *compiled = Compile(test);
            locals->EnterName(varName->value, compiled);
        }
        else if (dynamic_cast<TreeType *> (typeExpr))
        {
            // No type check in that case, and switch to lazy evaluation
            // (i.e. we generate a quote, caller must explicitly run it)
            Context block(context);
            Tree *compiled = block.Compile(test);
            Invoke *invoke = new Invoke(block.Depth(), compiled);
            QuotedTree *quote = new QuotedTree(invoke);
            locals->EnterName(varName->value, quote);
        }
        else
        {
            // Compile what we are testing against
            Tree *compiled = Compile(test);
            tree_position pos = test->Position();
            
            // Insert a run-time type test
            if (!end)
                end = new BranchTarget(pos);
            Tree *itest = new TypeTest(compiled, typeExpr, NULL, end, pos);
            code = Append(code, itest);

            // Enter the result of the type test in symbol table
            locals->EnterName(varName->value, compiled);
        }

        
        return what;
    }

    // Otherwise, this is a mismatch
    return NULL;
}


Tree *ArgumentMatch::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   For prefix expressions, simply test left then right
// ----------------------------------------------------------------------------
{
    if (Prefix *pt = dynamic_cast<Prefix *> (test))
    {
        // Check if we match the tree, e.g. f(A) vs. f(2)
        // Note that we must test left first to define 'f' in above case
        test = pt->left;
        Tree *lr = what->left->Do(this);
        test = pt;
        if (!lr)
            return NULL;
        test = pt->right;
        Tree *rr = what->right->Do(this);
        test = pt;
        if (!rr)
            return NULL;
        return what;
    }
    return NULL;
}


Tree *ArgumentMatch::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//    For postfix expressions, simply test right, then left
// ----------------------------------------------------------------------------
{
    if (Postfix *pt = dynamic_cast<Postfix *> (test))
    {
        // Check if we match the tree, e.g. A! vs 2!
        // Note that ordering is reverse compared to prefix, so that
        // the 'defined' names is set correctly
        test = pt->right;
        Tree *rr = what->right->Do(this);
        test = pt;
        if (!rr)
            return NULL;
        test = pt->left;
        Tree *lr = what->left->Do(this);
        test = pt;
        if (!lr)
            return NULL;
        return what;
    }
    return NULL;
}


Tree *ArgumentMatch::DoNative(Native *what)
// ----------------------------------------------------------------------------
//   Should never be used
// ----------------------------------------------------------------------------
{
    return context->Error("Internal error: Native parameter '$1'", what);
}



// ============================================================================
// 
//   Declaration action - Enter all tree rewrites in the current context
// 
// ============================================================================

struct DeclarationAction : Action
// ----------------------------------------------------------------------------
//   Compute an optimized version of the input tree
// ----------------------------------------------------------------------------
{
    DeclarationAction (Context *c): context(c) {}

    virtual Tree *Do(Tree *what);
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);
    virtual Tree *DoNative(Native *what);

    void        EnterRewrite(Tree *defined, Tree *definition);

    Context *context;
};


Tree *DeclarationAction::Do(Tree *what)
// ----------------------------------------------------------------------------
//   Default is to leave trees alone (for native trees)
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *DeclarationAction::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Integers evaluate directly
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *DeclarationAction::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Reals evaluate directly
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *DeclarationAction::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Text evaluates directly
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *DeclarationAction::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Build a unique reference in the context for the entity
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *DeclarationAction::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   Optimize away indent or parenthese blocks, evaluate others
// ----------------------------------------------------------------------------
{
    return what->child->Do(this);
}


Tree *DeclarationAction::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Compile built-in operators: \n ; -> and :
// ----------------------------------------------------------------------------
{
    // Check if this is an instruction list
    if (what->name == "\n" || what->name == ";")
    {
        // For instruction list, string declaration results together
        what->left->Do(this);
        what->right->Do(this);
        return what;
    }

    // Check if this is a rewrite declaration
    if (what->name == "->")
    {
        // Enter the rewrite
        EnterRewrite(what->left, what->right);
        return what;
    }

    return what;
}


Tree *DeclarationAction::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//    All prefix operations translate into a rewrite
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *DeclarationAction::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//    All postfix operations translate into a rewrite
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *DeclarationAction::DoNative(Native *what)
// ----------------------------------------------------------------------------
//    Leave all native code alone
// ----------------------------------------------------------------------------
{
    return what;
}


void DeclarationAction::EnterRewrite(Tree *defined, Tree *definition)
// ----------------------------------------------------------------------------
//   Add a definition in the current context
// ----------------------------------------------------------------------------
{
    context->EnterRewrite(defined, definition);
}



// ============================================================================
// 
//   Compilation action - Generation of "optimized" native trees
// 
// ============================================================================

struct CompileAction : Action
// ----------------------------------------------------------------------------
//   Compute an optimized version of the input tree
// ----------------------------------------------------------------------------
{
    CompileAction (Context *c): context(c) {}

    virtual Tree *Do(Tree *what);
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);
    virtual Tree *DoNative(Native *what);

    // Append a tree to a native tree (i.e. at end of its 'next')
    Tree   *    Append(Tree *left, Tree *right);

    // Build code selecting among rewrites in current context
    Tree *      Rewrites(Tree *what);

    Context *context;
};


Tree *CompileAction::Do(Tree *what)
// ----------------------------------------------------------------------------
//   Default is to leave trees alone (for native trees)
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *CompileAction::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Integers evaluate directly
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *CompileAction::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Reals evaluate directly
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *CompileAction::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Text evaluates directly
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *CompileAction::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Build a unique reference in the context for the entity
// ----------------------------------------------------------------------------
{
    // Normally, the name should have been declared in ParameterMatch
    if (Tree *result = context->Name(what->value))
        return result;
    return context->Error("Name '$1' does not exist", what);
}


Tree *CompileAction::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   Optimize away indent or parenthese blocks, evaluate others
// ----------------------------------------------------------------------------
{
    static Block            indent(NULL);
    static Parentheses      paren(NULL);
    
    if (what->Opening() == indent.Opening() &&
        what->Closing() == indent.Closing())
        return what->child->Do(this);
    if (what->Opening() == paren.Opening() &&
        what->Closing() == paren.Closing())
        return what->child->Do(this);
    
    // In other cases, we need to evaluate rewrites
    return Rewrites(what);
}


Tree *CompileAction::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Compile built-in operators: \n ; -> and :
// ----------------------------------------------------------------------------
{
    // Check if this is an instruction list
    if (what->name == "\n" || what->name == ";")
    {
        // For instruction list, string compile results together
        Tree *left = what->left->Do(this);
        if (dynamic_cast<Name *> (what->left))
        {
            // If left is a name, we return a reference to some object
            // We need something to append to, so create an invoke
            Invoke *invoke = new Invoke(context->Depth(),left,what->Position());
            left = invoke;
        }
        Tree *right = what->right->Do(this);
        return Append(left, right);
    }

    // Check if this is a rewrite declaration
    if (what->name == "->")
    {
        // If so, skip, this has been done in DeclarationAction
        return NULL;
    }

    // In all other cases, look up the rewrites
    return Rewrites(what);
}


Tree *CompileAction::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//    All prefix operations translate into a rewrite
// ----------------------------------------------------------------------------
{
    return Rewrites(what);
}


Tree *CompileAction::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//    All postfix operations translate into a rewrite
// ----------------------------------------------------------------------------
{
    return Rewrites(what);
}


Tree *CompileAction::DoNative(Native *what)
// ----------------------------------------------------------------------------
//    Leave all native code alone
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *CompileAction::Append(Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//   Append 'right' at end of native tree list starting at 'left'
// ----------------------------------------------------------------------------
{
    // If left is not a native, e.g. 0, we just optimize it away
    Native *native = dynamic_cast<Native *> (left);
    if (!native)
        return right;
    return native->Append(right);
}


Tree * CompileAction::Rewrites(Tree *what)
// ----------------------------------------------------------------------------
//   Build code selecting among rewrites in current context
// ----------------------------------------------------------------------------
{
    // Compute the hash key for the form we have to match
    ulong formKey, testKey;
    RewriteKey formKeyHash;
    what->Do(formKeyHash);
    formKey = formKeyHash.Key();

    Tree *result = NULL;
    BranchTarget *endOfCall = NULL;
    Tree *endOfPrev = NULL;
    eval_cache needed;

    for (Namespace *c = context; c; c = c->Parent())
    {
        Rewrite *candidate = c->Rewrites();
        while (candidate)
        {
            // Compute the hash key for the 'from' of the current rewrite
            RewriteKey testKeyHash;
            candidate->from->Do(testKeyHash);
            testKey = testKeyHash.Key();

            // If we have an exact match for the keys, we may have a winner
            if (testKey == formKey)
            {
                // If we find a real match, identify its parameters
                Context parms(candidate->context);
                ParameterMatch matchParms(&parms);
                Tree *parmsTest = candidate->from->Do(matchParms);
                if (!parmsTest)
                    return context->Error(
                        "Internal: Invokation parameters for '$1'?",
                        candidate->from);

                // Create the invokation point
                Context args(context);
                ArgumentMatch matchArgs(what, &args, context, needed);
                Tree *argsTest = candidate->from->Do(matchArgs);
                if (argsTest)
                {
                    // We should have same number of args and parms
                    ulong parmCount = parms.names.size();
                    if (parmCount < args.names.size())
                        return context->Error(
                            "Internal: arg/parm mismatch in '$1'", what);

                    // Create invokation node
                    Tree *code = candidate->Compile();
                    Invoke *invoke = new Invoke(candidate->context->Depth(),
                                                code,
                                                what->Position());
                    invoke->values.resize(parmCount);

                    // Map the arguments we found in called stack order
                    // (i.e. we take the order from stack variables ids)
                    symbol_table::iterator i;
                    for (i = parms.names.begin(); i != parms.names.end(); i++)
                    {
                        text name = (*i).first;
                        Tree *argValue = args.NamedTree(name);
                        Tree *parm = parms.NamedTree(name);
                        Variable *parmVar = dynamic_cast<Variable *> (parm);
                        if (!parmVar)
                            return context->Error(
                                "Internal: non-var parm '$1'?", parm);
                        invoke->values[parmVar->id] = argValue;
                    }

                    // If there is test code to validate candidate, prepend it
                    Tree *callCode = NULL;
                    if (matchArgs.code)
                    {
                        // We need an exit point when a call is successful
                        if (!endOfCall)
                            endOfCall = new BranchTarget(what->Position());
                        callCode = matchArgs.code;
                    }

                    // Compile the target and use it as invoke child
                    callCode = Append(callCode, invoke);

                    // If there were tests, branch for successful call
                    if (endOfCall)
                        invoke->Append(endOfCall);

                    // If previous call was conditional, append to failed exit
                    if (endOfPrev)
                        callCode = Append(endOfPrev, callCode);

                    // If this call defined a condition, remember it
                    endOfPrev = matchArgs.end;

                    // Append generated code to result
                    if (!result)
                        result = callCode;
                } // Match args
            } // Match test key

            // Otherwise, check if we have a key match in the hash table,
            // and if so follow it.
            if (candidate->hash.count(formKey) > 0)
                candidate = candidate->hash[formKey];
            else
                candidate = NULL;
        } // while(candidate)
    } // for(namespaces)

    // If there were tests in previous calls, and if none succeeded,
    // indicate that the call failed
    if (endOfPrev)
        Append(endOfPrev, new FailedCall(what, what->Position()));

    // Allocate enough locals for the complete evaluation of the rewrite
    ulong slots = needed.size();
    if (slots)
    {
        AllocateLocals *alloc = new AllocateLocals(slots);
        alloc->next = result;
        result = alloc;
        AllocateLocals *dealloc = new AllocateLocals(-slots);
        if (endOfCall)
            endOfCall->next = dealloc;
        else
            Append(result, dealloc);
    }

    if (!result)
        return context->Error("No candidate for '$1'", what);

    return result;
}



// ============================================================================
// 
//    Evaluation of trees
// 
// ============================================================================

Tree *Context::Name(text name)
// ----------------------------------------------------------------------------
//    Return a variable in the name if there is one
// ----------------------------------------------------------------------------
{
    ulong frame = 0;
    for (Context *c = this; c; c = c->Parent())
    {
        Tree *existing = c->names.count(name) > 0 ? c->names[name] : NULL;
        Variable *variable = dynamic_cast<Variable *> (existing);
        if (existing && !variable)
            return existing;
        if (variable)
        {
            if (!frame)
                return variable;
            return new NonLocalVariable(c->Depth(), variable->id,
                                        variable->Position());
        }
        frame++;
    }
    return NULL;
}


struct ReturnNullIfBad : Native
// ----------------------------------------------------------------------------
//   Simply return NULL if some error occurs
// ----------------------------------------------------------------------------
{
    ReturnNullIfBad() : Native(NULL) {}
    Tree *Run(Stack *stack) { return NULL; }
};


Tree *Context::Compile(Tree *source, bool nullIfBad)
// ----------------------------------------------------------------------------
//    Return an optimized version of the source tree, ready to run
// ----------------------------------------------------------------------------
{
    if (compiled.count(source) > 0)
        return compiled[source];

    ReturnNullIfBad nib;
    Tree *handler = error_handler;
    if (nullIfBad)
        error_handler = &nib;

    DeclarationAction declare(this);
    Tree *result = source->Do(declare);
        
    CompileAction compile(this);
    result = source->Do(compile);

    if (result)
        compiled[source] = result;
    error_handler = handler;

    return result;
}


Tree *Context::Run(Tree *code)
// ----------------------------------------------------------------------------
//   Execute a compiled code tree
// ----------------------------------------------------------------------------
{
    Tree *result = code;
    Stack stack(errors);
    run_stack = &stack;
    result = stack.Run(code);
    run_stack = NULL;
    return result;
}


Rewrite *Context::EnterRewrite(Tree *from, Tree *to)
// ----------------------------------------------------------------------------
//   Create a rewrite for the current context and enter it
// ----------------------------------------------------------------------------
{
    Rewrite *rewrite = new Rewrite(this, from, to);
    return Namespace::EnterRewrite(rewrite);
}



// ============================================================================
// 
//    Error handling
// 
// ============================================================================

static inline const char *indent(ulong sz)
// ----------------------------------------------------------------------------
//    Return enough spacing for indentation
// ----------------------------------------------------------------------------
{
    return "                                        " + (sz<40 ? (40-sz) : 0);
}


Tree *Stack::Run(Tree *code)
// ----------------------------------------------------------------------------
//    Execute code until there is nothing left to do
// ----------------------------------------------------------------------------
{
    Tree *result = NULL;
    IFTRACE(eval)
        std::cerr << indent(values.size())
                  << "Stack " << values.size()
                  << " Exec " << code << "\n";

    if (Leaf *leaf = dynamic_cast<Leaf *> (code))
        return leaf;

    while (Native *native = dynamic_cast<Native *> (code))
    {
        IFTRACE(eval)
            std::cerr << indent(values.size())
                      << "Step " << native->TypeName() << "\n";
        Tree *value = native->Run(this);
        if (value)
        {
            IFTRACE(eval)
                std::cerr << indent(values.size())
                          << "  result " << value << "\n";
            result = value;
        }
        code = native->Next();
    }
    IFTRACE(eval)
        std::cerr << indent(values.size())
                  << "Stack " << values.size()
                  << " Result " << result << "\n";
    return result;
}


Tree *Stack::Error(text message, Tree *arg1, Tree *arg2, Tree *arg3)
// ----------------------------------------------------------------------------
//   Execute the innermost error handler
// ----------------------------------------------------------------------------
{
    Tree *handler = error_handler;
    if (handler)
    {
        Invoke errorInvokation(1, handler, handler->Position());
        Tree *info = new XL::Text (message);
        errorInvokation.AddArgument(info);
        if (arg1)
            errorInvokation.AddArgument(arg1);
        if (arg2)
            errorInvokation.AddArgument(arg2);
        if (arg3)
            errorInvokation.AddArgument(arg3);
        errorInvokation.invoked = handler;
        error_handler = NULL;
        Tree *result =  errorInvokation.Run(this);
        error_handler = handler;
        return result;
    }

    // No handler: terminate
    errors.Error(message, arg1, arg2, arg3);
    std::exit(1);
}


Tree * Context::Error(text message, Tree *arg1, Tree *arg2, Tree *arg3)
// ----------------------------------------------------------------------------
//   Execute the innermost error handler
// ----------------------------------------------------------------------------
{
    Tree *handler = ErrorHandler();
    if (handler)
    {
        Invoke errorInvokation(1, handler, handler->Position());
        Tree *info = new XL::Text (message);
        errorInvokation.AddArgument(info);
        if (arg1)
            errorInvokation.AddArgument(arg1);
        if (arg2)
            errorInvokation.AddArgument(arg2);
        if (arg3)
            errorInvokation.AddArgument(arg3);
        Stack errorStack(errors);
        return errorInvokation.Run(&errorStack);
    }

    // No handler: terminate
    errors.Error(message, arg1, arg2, arg3);
    std::exit(1);
}


ulong Context::Depth()
// ----------------------------------------------------------------------------
//    Return the depth for the current context
// ----------------------------------------------------------------------------
{
    ulong depth = 0;
    for (Context *c = this; c; c = c->Parent())
        depth++;
    return depth;
}


Tree *Context::ErrorHandler()
// ----------------------------------------------------------------------------
//    Return the innermost error handler
// ----------------------------------------------------------------------------
{
    for (Context *c = this; c; c = c->Parent())
        if (c->error_handler)
            return c->error_handler;
    return NULL;
}



// ============================================================================
// 
//    Tree rewrites
// 
// ============================================================================

Rewrite::~Rewrite()
// ----------------------------------------------------------------------------
//   Deletes all children rewrite if any
// ----------------------------------------------------------------------------
{
    rewrite_table::iterator it;
    for (it = hash.begin(); it != hash.end(); it++)
        delete ((*it).second);
}


Rewrite *Rewrite::Add (Rewrite *rewrite)
// ----------------------------------------------------------------------------
//   Add a new rewrite at the right place in an existing rewrite
// ----------------------------------------------------------------------------
{
    Rewrite *parent = this;
    ulong formKey;

    // Compute the hash key for the form we have to match
    RewriteKey formKeyHash;
    rewrite->from->Do(formKeyHash);
    formKey = formKeyHash.Key();

    while (parent)
    {
        // Check if we have a key match in the hash table,
        // and if so follow it.
        if (parent->hash.count(formKey) > 0)
        {
            parent = parent->hash[formKey];
        }
        else
        {
            parent->hash[formKey] = rewrite;
            return parent;
        }
    }

    return NULL;
}


Tree *Rewrite::Do(Action &a)
// ----------------------------------------------------------------------------
//   Apply an action to the 'from' and 'to' fields and all hash entries
// ----------------------------------------------------------------------------
{
    rewrite_table::iterator i;
    Tree *result = from->Do(a);
    result = to->Do(a);
    for (i = hash.begin(); i != hash.end(); i++)
        result = (*i).second->Do(a);
    return result;
}


Tree *Rewrite::Compile(void)
// ----------------------------------------------------------------------------
//   Make sure that the 'to' tree is compiled
// ----------------------------------------------------------------------------
{
    Tree *source = to;
    Leaf *leaf = dynamic_cast<Leaf *> (source);
    if (leaf)
        return leaf;
    Native *native = dynamic_cast<Native *> (source);
    if (native)
        return native;

    // Identify all parameters in 'from'
    Context locals(context);
    ParameterMatch matchParms(&locals);
    Tree *parms = from->Do(matchParms);
    if (!parms)
        return context->Error("Internal: what parameters in '$1'?", from);

    // Put a native placeholder in 'to' in case we compile recursively
    Entry *entry = new Entry(to, to->Position());
    to = entry;

    // Compile the body of the rewrite
    Tree *code = locals.Compile(source);
    if (!code)
        return context->Error("Unable to compile '$1'", source);

    // Remember the compiled form
    if (!entry->next)
        entry->next = code;
    return entry;
}


XL_END
