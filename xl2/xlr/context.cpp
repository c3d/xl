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
    if (rewrites) delete rewrites;
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
//    Evaluation of trees
// 
// ============================================================================

Tree *Context::Run(Tree *source, bool eager)
// ----------------------------------------------------------------------------
//   Apply any applicable rewrite to the source and evaluate result
// ----------------------------------------------------------------------------
{
    bool changed = true;
    IFTRACE(eval)
        std::cout << (eager ? "Eval: " : "Run: ") << source << "\n";

    while (changed)
    {
        changed = false;
        if (!source)
            return source;

        for (Namespace *c = this; c; c = c->Parent())
        {
            if (Rewrite *rew = c->Rewrites())
            {
                Context locals(rew->context);
                if (Rewrite *handler = rew->Handler(source, &locals))
                {
                    Tree *rewritten = handler->Apply(source, &locals);
                    IFTRACE(rewrite)
                        std::cout << (eager ? "Eager " : "Lazy ") << source
                                  << " ===> " << rewritten << "\n";
                    source = rewritten;
                    changed = true;
                    break;
                }
            }
        }

        if (!changed && eager && source)
        {
            Tree *result = source->Run(this);
            if (result != source)
            {
                IFTRACE(rewrite)
                    std::cout << (eager ? "EagerTail " : "LazyTail") << source
                              << " ===> " << result << "\n";
                source = result;
                changed = true;
            }
        }
    }
    return source;
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


struct TreeMatch : Action
// ----------------------------------------------------------------------------
//   Check if two trees match
// ----------------------------------------------------------------------------
{
    TreeMatch (Tree *t, Context *c):
        test(t), context(c), defined(NULL)
    {
        context->Clear();
    }

    Tree *DoInteger(Integer *what)
    {
        if (Integer *it = dynamic_cast<Integer *> (context->Eval(test)))
            if (it->value == what->value)
                return what;
        return NULL;
    }
    Tree *DoReal(Real *what)
    {
        if (Real *rt = dynamic_cast<Real *> (context->Eval(test)))
            if (rt->value == what->value)
                return what;
        return NULL;
    }
    Tree *DoText(Text *what)
    {
        if (Text *tt = dynamic_cast<Text *> (context->Eval(test)))
            if (tt->value == what->value)
                return what;
        return NULL;
    }
    Tree *DoName(Name *what)
    {
        if (!defined)
        {
            // The first name we see must match exactly
            defined = what;
            if (Name *nt = dynamic_cast<Name *> (test))
                if (nt->value == what->value)
                    return what;
        }
        else
        {
            // Subsequence names are considered variable names unless
            // they already exist in the context. This also is useful
            // to check "A+A"
            // Subtlety: if the name exists, e.g. "false", we need to
            // do an eager evaluation of the match. Otherwise, we
            // enter the tree, so we do lazy evaluation.
            if (Tree *existing = context->Name(what->value))
            {
                if (existing == context->Eval(test)) // Revisit: tree match?
                    return what;
                return NULL;
            }
            context->EnterName(what->value, test);
            return what;
        }
        return NULL;
    }

    Tree *DoBlock(Block *what)
    {
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

        // If the reference is a parenthese block, we can match its child
        Parentheses paren(NULL);
        if (what->Opening() == paren.Opening() &&
            what->Closing() == paren.Closing())
        {
            return what->child->Do(this);
        }

        return NULL;
    }
    Tree *DoInfix(Infix *what)
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

            // Evaluate type expression, e.g. 'integer' in example above
            Tree *typeExpr = context->Run(what->right);

            // Calling the type expression returns the expression
            // (with appropriate casts as necessary) if of the right type
            // In that case, we enter the name K in the context
            Tree *result = typeExpr->Call(context, test);
            if (result)
                context->EnterName(varName->value, result);
            return result;
        }
        return NULL;
    }
    Tree *DoPrefix(Prefix *what)
    {
        if (Prefix *pt = dynamic_cast<Prefix *> (test))
        {
            // Check if we match the tree, e.g. f(A) vs. f(2)
            Infix *defined_infix = dynamic_cast<Infix *> (defined);
            if (defined_infix)
                defined = NULL;
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
            if (!defined && defined_infix)
                defined = defined_infix;
            return what;
        }
        return NULL;
    }
    Tree *DoPostfix(Postfix *what)
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
    Tree *Do(Tree *what)
    {
        return false;
    }

    Tree *      test;
    Context *   context;
    Tree *      defined;
};


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


Rewrite *Rewrite::Handler(Tree *form, Context *locals)
// ----------------------------------------------------------------------------
//   Find the rewrite that deals with the given tree
// ----------------------------------------------------------------------------
{
    Rewrite *candidate = this;
    ulong formKey, testKey;

    // Compute the hash key for the form we have to match
    RewriteKey formKeyHash;
    form->Do(formKeyHash);
    formKey = formKeyHash.Key();

    while (candidate)
    {
        // Compute the hash key for the 'from' of the current rewrite
        RewriteKey testKeyHash;
        candidate->from->Do(testKeyHash);
        testKey = testKeyHash.Key();

        // If we have an exact match for the keys, we may have a winner
        if (testKey == formKey)
        {
            TreeMatch testTreeShape(form, locals);
            Tree *result = candidate->from->Do(testTreeShape);
            if (result)
                return candidate;
        }

        // Otherwise, check if we have a key match in the hash table,
        // and if so follow it.
        if (candidate->hash.count(formKey) > 0)
            candidate = candidate->hash[formKey];
        else
            candidate = NULL;
    }

    return candidate;
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
