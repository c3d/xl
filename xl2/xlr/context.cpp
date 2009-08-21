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


Tree *Namespace::Name(text name, bool deep)
// ----------------------------------------------------------------------------
//   Lookup a name in the symbol table
// ----------------------------------------------------------------------------
{
    for (Namespace *c = this; c; c = c->Parent())
    {
        if (c->name_symbols.count(name))
            return c->name_symbols[name];
        if (!deep)
            break;
    }
    return NULL;
}


void Namespace::EnterName (text name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a name in the namespace
// ----------------------------------------------------------------------------
{
    name_symbols[name] = value;
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
    name_symbols = empty;
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
        active_set::iterator i;
        ulong deletedCount = 0, activeCount = 0;

        IFTRACE(memory)
            std::cerr << "Garbage collecting...";
        
        // Mark roots and stack
        for (i = roots.begin(); i != roots.end(); i++)
            (*i)->Do(gc);

        // Mark all rewrites
        if (rewrites)
            rewrites->Do(gc);

        // Then delete all trees in active set that are no longer referenced
        for (i = active.begin(); i != active.end(); i++)
        {
            activeCount++;
            if (!gc.alive.count(*i))
            {
                deletedCount++;
                delete *i;
            }
        }
        active = gc.alive;
        gc_threshold = active.size() * gc_growth_percent / 100 + gc_increment;
        IFTRACE(memory)
            std::cerr << "done: Purged " << deletedCount
                      << " out of " << activeCount
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
//    Parameter matching - Test input parameters
// 
// ============================================================================

struct ParameterMatch : Action
// ----------------------------------------------------------------------------
//   Check if two trees match, collect 'variables' and emit type test nodes
// ----------------------------------------------------------------------------
{
    ParameterMatch (Tree *t, Context *c):
        context(c), test(t), defined(NULL), code(NULL), end(NULL) {}

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

    Context * context;          // Context in which we test
    Tree *    test;             // Tree we test
    Tree *    defined;          // Tree beind defined, e.g. 'sin' in 'sin X'
    Tree *    code;             // Generated code
    Tree *    end;              // End label if necessary
};


Tree *ParameterMatch::Append(Tree *left, Tree *right)
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


Tree *ParameterMatch::Do(Tree *what)
// ----------------------------------------------------------------------------
//   Default is to return failure
// ----------------------------------------------------------------------------
{
    return NULL;
}


Tree *ParameterMatch::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   An integer matches the exact value
// ----------------------------------------------------------------------------
{
    // Compile the test tree
    Tree *compiled = context->Compile(test);
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


Tree *ParameterMatch::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   A real matches the exact value
// ----------------------------------------------------------------------------
{
    // Compile the test tree
    Tree *compiled = context->Compile(test);
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


Tree *ParameterMatch::DoText(Text *what)
// ----------------------------------------------------------------------------
//   A text matches the exact value
// ----------------------------------------------------------------------------
{
    // Compile the test tree
    Tree *compiled = context->Compile(test);
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


Tree *ParameterMatch::DoName(Name *what)
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
    }
    else
    {
        // Compile what we are testing against
        Tree *compiled = context->Compile(test);
        tree_position pos = test->Position();

        // If input tree doesn't compile by itself, we can't match this,
        // e.g. if we have 2+3*i, and we try to compile 3*i
        if (!compiled)
            return NULL;

        // Check if the name already exists, e.g. 'false' or 'A+A'
        // If it does, we generate a run-time check to verify equality
        if (Tree *existing = context->Name(what->value))
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
        Named *named = new Named(compiled, pos);
        context->EnterName(what->value, named);
        return what;
    }
    return NULL;
}


Tree *ParameterMatch::DoBlock(Block *what)
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


Tree *ParameterMatch::DoInfix(Infix *what)
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

        // Compile what we are testing against
        Tree *compiled = context->Compile(test);
        tree_position pos = test->Position();

        // Enter the name in symbol table
        Named *named = new Named(compiled, pos);
        context->EnterName(varName->value, named);

        // Evaluate type expression, e.g. 'integer' in example above
        Tree *typeExpr = context->Compile(what->right);

        // Insert a run-time type test
        if (!end)
            end = new BranchTarget(pos);
        Tree *itest = new TypeTest(compiled, typeExpr, NULL, end, pos);
        code = Append(code, itest);
        
        return what;
    }

    // Otherwise, this is a mismatch
    return NULL;
}


Tree *ParameterMatch::DoPrefix(Prefix *what)
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


Tree *ParameterMatch::DoPostfix(Postfix *what)
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


Tree *ParameterMatch::DoNative(Native *what)
// ----------------------------------------------------------------------------
//   Should never be used
// ----------------------------------------------------------------------------
{
    return context->Error("Internal error: Native parameter '$1'", what);
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

    // Add a definition in the current context
    void        EnterRewrite(Tree *defined, Tree *definition);

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
    // Check if a name has already been seen. If so, return its reference.
    if (Tree *result = context->Name(what->value))
        return result;
    
    // Otherwise, create a NULL reference and return that.
    Named *named = new Named(NULL, what->Position());
    context->EnterName(what->value, named);
    return named;
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
        Tree *right = what->right->Do(this);
        return Append(left, right);
    }

    // Check if this is a rewrite declaration
    if (what->name == "->")
    {
        // Enter the rewrite
        EnterRewrite(what->left, what->right);

        // Return a total lack of anything else to do
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
                // If we find a real match, it will require its own context
                Invoke *invoke = new Invoke(context, what->Position());
                ParameterMatch matchParms(what, &invoke->locals);
                Tree *match = candidate->from->Do(matchParms);
                if (match)
                {
                    // We have found a possible match.
                    Tree *code = NULL;

                    // If there is test code to validate candidate, use it
                    if (matchParms.code)
                        code = matchParms.code;

                    // Apply the rewrite to the input tree
                    Tree *rewritten = candidate->Apply(what, &invoke->locals);

                    // Compile the result and store it as invoke child
                    invoke->child = context->Compile(rewritten);
                    code = Append(code, invoke);

                    // If we defined an end point for matchParms, append it
                    if (matchParms.end)
                        code = Append(code, matchParms.end);

                    // Append generated code to result
                    result = Append(result, code);
                }
            }

            // Otherwise, check if we have a key match in the hash table,
            // and if so follow it.
            if (candidate->hash.count(formKey) > 0)
                candidate = candidate->hash[formKey];
            else
                candidate = NULL;
        } // while(candidate)
    } // for(namespaces)

    return result;
}


void CompileAction::EnterRewrite(Tree *defined, Tree *definition)
// ----------------------------------------------------------------------------
//   Add a definition in the current context
// ----------------------------------------------------------------------------
{
    context->EnterRewrite(defined, definition);
}



// ============================================================================
// 
//    Evaluation of trees
// 
// ============================================================================

Tree *Context::Compile(Tree *source)
// ----------------------------------------------------------------------------
//    Return an optimized version of the source tree, ready to run
// ----------------------------------------------------------------------------
{
    Tree *result = compiled[source];
    if (result)
        return result;

    CompileAction compile(this);
    result = source->Do(compile);
    compiled[source] = result;

    return result;
}


Tree *Context::Run(Tree *code, bool eager)
// ----------------------------------------------------------------------------
//   Execute a compiled code tree
// ----------------------------------------------------------------------------
{
    Tree *result = code;
    while (Native *native = dynamic_cast<Native *> (code))
    {
        result = native->Run(this);
        code = native->Next();
    }
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


Rewrite *Context::EnterInfix(text name, Tree *callee)
// ----------------------------------------------------------------------------
//   Enter an infix expression that will call the given callee
// ----------------------------------------------------------------------------
{
    XL::Name *left = new XL::Name("x");
    XL::Name *right = new XL::Name("y");

    Infix *from = new Infix(name, left, right);
    Prefix *to = new Prefix(callee, from);

    return EnterRewrite(from, to);
}



// ============================================================================
// 
//    Error handling
// 
// ============================================================================

Tree * Context::Error(text message, Tree *arg1, Tree *arg2, Tree *arg3)
// ----------------------------------------------------------------------------
//   Execute the innermost error handler
// ----------------------------------------------------------------------------
{
    Tree *handler = ErrorHandler();
    if (handler)
    {
        Tree *info = new XL::Text (message);
        if (arg1)
            info = new XL::Infix(",", info, arg1, arg1->Position());
        if (arg2)
            info = new XL::Infix(",", info, arg2, arg2->Position());
        if (arg3)
            info = new XL::Infix(",", info, arg3, arg3->Position());
        return handler->Call(this, info);
    }

    // No handler: terminate
    std::cerr << "Error: No error handler\n";
    errors.Error(message, arg1, arg2, arg3);
    std::exit(1);
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


Tree *Rewrite::Apply(Tree *source, Context *locals)
// ----------------------------------------------------------------------------
//   Return a tree built by cloning the 'to' tree and replacing args
// ----------------------------------------------------------------------------
//   Note: this assumes the rewrite results from 'Handler', so that the
//   context is populated with all the right local variables
{
    TreeRewrite rewrite(locals);
    return to->Do(rewrite);
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

XL_END
