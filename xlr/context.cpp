// ****************************************************************************
//  context.cpp                     (C) 1992-2003 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//     The execution environment for XL
//
//     This defines both the compile-time environment (Context), where we
//     keep symbolic information, e.g. how to rewrite trees, and the
//     runtime environment (Runtime), which we use while executing trees
//
//     See comments at beginning of context.h for details
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
#include "basics.h"
#include "compiler.h"
#include "runtime.h"
#include "main.h"
#include "types.h"
#include <sstream>

XL_BEGIN

// ============================================================================
//
//   Context: Representation of execution context
//
// ============================================================================

Context::Context(Context *scope, Context *stack)
// ----------------------------------------------------------------------------
//   Constructor for an execution context
// ----------------------------------------------------------------------------
    : scope(scope), stack(stack), rewrites(),
      hasConstants(scope ? scope->hasConstants : false)
{}


Context::~Context()
// ----------------------------------------------------------------------------
//   Destructor for execution context
// ----------------------------------------------------------------------------
{}


static void ValidateNames(Tree *form)
// ----------------------------------------------------------------------------
//   Check that we have only valid names in the pattern
// ----------------------------------------------------------------------------
{
    switch(form->Kind())
    {
    case INTEGER:
    case REAL:
    case TEXT:
        break;
    case NAME:
    {
        Name *name = (Name *) form;
        if (name->value.length() && !isalpha(name->value[0]))
            Ooops("The pattern variable $1 is not a name", name);
        break;
    }
    case INFIX:
    {
        Infix *infix = (Infix *) form;
        ValidateNames(infix->left);
        ValidateNames(infix->right);
        break;
    }
    case PREFIX:
    {
        Prefix *prefix = (Prefix *) form;
        if (prefix->left->Kind() != NAME)
            ValidateNames(prefix->left);
        ValidateNames(prefix->right);
        break;
    }
    case POSTFIX:
    {
        Postfix *postfix = (Postfix *) form;
        if (postfix->right->Kind() != NAME)
            ValidateNames(postfix->right);
        ValidateNames(postfix->left);
        break;
    }
    case BLOCK:
    {
        Block *block = (Block *) form;
        ValidateNames(block->child);
        break;
    }
    }
}


Rewrite *Context::Define(Tree *form, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a rewrite in the context
// ----------------------------------------------------------------------------
{
    // If we have a block on the left, define the child of that block
    if (Block *block = form->AsBlock())
        form = block->child;

    // Check if we rewrite a constant. If so, remember it
    if (form->IsConstant())
        hasConstants = true;

    // Check that we have only names in the pattern
    ValidateNames(form);

    // Check if we insert a name. If so, we shouldn't redefine
    Name *name = form->AsName();

    // Create a rewrite
    Rewrite *rewrite = new Rewrite(form, value);
    ulong key = Hash(form);

    // Walk through existing rewrites
    Rewrite_p *parent = &rewrites[key]; 
    if (name)
    {
        // Name: check for redefinitions
        while (Rewrite *where = *parent)
        {
            if (Name *existing = where->from->AsName())
            {
                if (existing->value == name->value)
                {
                    Ooops("Name $1 already exists", name);
                    Ooops("Previous definition was $1", existing);
                }
            }
            parent = &where->hash[key];
        }
    }           
    else
    {
        // Not a name, won't check for redefinitions
        while (Rewrite *where = *parent)
            parent = &where->hash[key];
    }

    // Create the rewrite
    *parent = rewrite;

    return rewrite;
}


Rewrite *Context::DefineData(Tree *data)
// ----------------------------------------------------------------------------
//   Enter a data declaration
// ----------------------------------------------------------------------------
{
    return Define(data, NULL);
}


Tree *Context::Evaluate(Tree *what, lookup_mode lookup)
// ----------------------------------------------------------------------------
//   Evaluate 'what' in the given context
// ----------------------------------------------------------------------------
{
    // Quick optimization for constants
    if (!hasConstants && what->IsConstant())
        return what;

    // Depth check
    static uint depth = 0;
    XL::LocalSave<uint> save(depth, depth+1);
    if (depth > MAIN->options.stack_depth)
    {
        Ooops("Recursed too deep evaluating $1", what);
        return what;
    }

    // Build the hash key for the tree to evaluate
    ulong key = Hash(what);

    // Loop over all contexts
    Context *next = NULL;
    for (Context *context = this; context; context = next)
    {
        rewrite_table &rwt = context->rewrites;
        rewrite_table::iterator found = rwt.find(key);
        if (found != rwt.end())
        {
            Rewrite *candidate = (*found).second;
            while (candidate)
            {
                ulong formKey = Hash(candidate->from);
                if (formKey == key)
                {
                    // If this is native C code, invoke it.
                    if (native_fn fn = candidate->native)
                    {
                        // Bind native context
                        TreeList args;
                        if (Bind(candidate->from, what, &args))
                        {
                            uint arity = args.size();
                            Compiler *comp = MAIN->compiler;
                            adapter_fn adj = comp->ArrayToArgsAdapter(arity);
                            Tree **args0 = (Tree **) &args[0];
                            Tree *result = adj(fn, this, what, args0);
                            return result;
                        }
                    }
                    else if (Name *name = candidate->from->AsName())
                    {
                        Name *vname = what->AsName();
                        assert(vname || !"Hash function is broken");
                        if (name->value == vname->value)
                        {
                            Tree *result = candidate->to;
                            if (result != candidate->from)
                            {
                                next = new Context(context, this);
                                result = next->Evaluate(result);
                            }
                            return result;
                        }
                    }
                    else
                    {
                        // Keep evaluating
                        Context *eval = new Context(context, this);
                        if (eval->Bind(candidate->from, what))
                        {
                            Tree *result = candidate->from;
                            if (Tree *to = candidate->to)
                                result = eval->Evaluate(to);
                            else
                                result = xl_evaluate_children(eval, result);
                            return result;
                        }
                    }
                }

                rewrite_table &rwh = candidate->hash;
                found = rwh.find(key);
                candidate = found == rwh.end() ? NULL : (*found).second;
            } // Loop on candidates
        } // If found candidate

        // Select which scope to use next
        if (lookup == SCOPE_LOOKUP)
            next = context->scope;
        else if (STACK_LOOKUP)
            next = context->stack;
    } // Loop on contexts

    // Error case - Should we raise some error here?
    return what;
}


Tree *Context::EvaluateBlock(Tree *what)
// ----------------------------------------------------------------------------
//   Create a new inner scope for evaluating the value
// ----------------------------------------------------------------------------
{
    Context *block = new Context(this, stack);
    what = block->Evaluate(what);
    return what;
}


ulong Context::Hash(Tree *what)
// ----------------------------------------------------------------------------
//   Compute the hash code in the rewrite table
// ----------------------------------------------------------------------------
{
    kind  k = what->Kind();
    ulong h = 0;
    text  t;

    switch(k)
    {
    case INTEGER:
        h = ((Integer *) what)->value;
        break;
    case REAL:
        h = *((ulong *) &((Real *) what)->value);
        break;
    case TEXT:
        t = ((Text *) what)->value;
        break;
    case NAME:
        t = ((Name *) what)->value;
        break;
    case BLOCK:
        t = ((Block *) what)->opening + ((Block *) what)->closing;
        break;
    case INFIX:
        t = ((Infix *) what)->name;
        break;
    case PREFIX:
        h = Hash(((Prefix *) what)->left);
        break;
    case POSTFIX:
        h = Hash(((Postfix *) what)->right);
        break;
    }

    if (t.length())
    {
        h = 0xC0DED;
        for (text::iterator p = t.begin(); p != t.end(); p++)
            h = (h * 0x301) ^ *p;
    }

    h = (h << 4) | (ulong) k;

    return h;
}


bool Context::Bind(Tree *form, Tree *value, TreeList *args)
// ----------------------------------------------------------------------------
//   Test if we can match arguments to values
// ----------------------------------------------------------------------------
{
    kind k = form->Kind();
    Context *eval = args ? this : (Context *) stack;

    switch(k)
    {
    case INTEGER:
        value = eval->Evaluate(value);
        if (Integer *iv = value->AsInteger())
            return iv->value == ((Integer *) form)->value;
        return false;
    case REAL:
        value = eval->Evaluate(value);
        if (Real *rv = value->AsReal())
            return rv->value == ((Real *) form)->value;
        return false;
    case TEXT:
        value = eval->Evaluate(value);
        if (Text *tv = value->AsText())
            return (tv->value == ((Text *) form)->value &&
                    tv->opening == ((Text *) form)->opening &&
                    tv->closing == ((Text *) form)->closing);
        return false;


    case NAME:
        // Test if the name is already bound, and if so, if trees match
        if (Tree *bound = Bound((Name *) form, SCOPE_LOOKUP))
        {
            if (bound == form)
                return true;
            if (EqualTrees(bound, form))
                return true;
            value = eval->Evaluate(value);
            bound = eval->Evaluate(bound);
            return EqualTrees(bound, value);
        }

        // Define the name in the given context
        value = eval->CreateClosure(value);
        if (args)
            args->push_back(value);
        else
            Define(form, value);
        return true;

    case INFIX:
    {
        Infix *fi = (Infix *) form;

        // Check type declarations
        if (fi->name == ":")
        {
            if (Name *name = fi->left->AsName())
            {
                // This is always an error if the name is bound in this scope
                if (Tree *existing = Bound(name, LOCAL_LOOKUP))
                {
                    Ooops("Name $1 was already defined", name);
                    Ooops("with value $1", existing);
                    return false;
                }

                // Evaluate the type
                Tree *type = ((Infix *) form)->right;
                type = eval->Evaluate(type);

                // Evaluate the value and match its type if the type is not tree
                if (type != tree_type)
                {
                    value = eval->Evaluate(value);
                    if (!ValueMatchesType(this, type, value, false))
                        return false;
                }

                // Define the name locally, do not create a closure
                if (args)
                    args->push_back(value);
                else
                    Define(name, value);
                return true;
            } // We have a name on the left
        } // We have an infix :

        // If we match the infix name, we can bind left and right
        if (Infix *infix = value->AsInfix())
            if (((Infix *) form)->name == infix->name)
                return (Bind(((Infix *) form)->left, infix->left, args) &&
                        Bind(((Infix *) form)->right, infix->right, args));

        // If direct binding failed, evaluate and try again ("diamond")
        value = eval->Evaluate(value);
        if (Infix *infix = value->AsInfix())
            if (((Infix *) form)->name == infix->name)
                return (Bind(((Infix *) form)->left, infix->left, args) &&
                        Bind(((Infix *) form)->right, infix->right, args));

        // Otherwise, we don't have a match
        return false;
    }

    case PREFIX:
        // If the left side is a name, make sure it's an exact match
        if (Prefix *prefix = value->AsPrefix())
        {
            if (Name *name = ((Prefix *) form)->left->AsName())
            {
                Tree *vname = prefix->left;
                if (vname->Kind() != NAME)
                    vname = eval->Evaluate(vname);
                if (Name *vn = vname->AsName())
                    if (name->value != vn->value)
                        return false;
            }
            else
            {
                if (!Bind(((Prefix *) form)->left, prefix->left, args))
                    return false;
            }
            return Bind(((Prefix *) form)->right, prefix->right, args);
        }
        return false;

    case POSTFIX:
        // If the right side is a name, make sure it's an exact match
        if (Postfix *postfix = value->AsPostfix())
        {
            if (Name *name = ((Postfix *) form)->right->AsName())
            {
                Tree *vname = postfix->right;
                if (vname->Kind() != NAME)
                    vname = eval->Evaluate(vname);
                if (Name *vn = vname->AsName())
                    if (name->value != vn->value)
                        return false;
            }
            else
            {
                if (!Bind(((Postfix *) form)->right, postfix->right, args))
                    return false;
            }
            return Bind(((Postfix *) form)->left, postfix->left, args);
        }
        return false;

    case BLOCK:
    {
        Block *block = (Block *) form;
        if (Block *bval = value->AsBlock())
            if (bval->opening == block->opening &&
                bval->closing == block->closing)
                return Bind(block->child, bval->child, args);
        return Bind(block->child, value, args);
    }
    }

    // Default is to return false
    return false;
}


Tree *Context::Bound(Name *name, lookup_mode lookup)
// ----------------------------------------------------------------------------
//   Return the value a name is bound to, or NULL if none...
// ----------------------------------------------------------------------------
{
    // Build the hash key for the tree to evaluate
    ulong key = Hash(name);

    // Loop over all contexts
    Context *next = NULL;
    for (Context *context = this; context; context = next)
    {
        rewrite_table &rwt = context->rewrites;
        rewrite_table::iterator found = rwt.find(key);
        if (found != rwt.end())
        {
            Rewrite *candidate = (*found).second;
            while (candidate)
            {
                if (Name *from = candidate->from->AsName())
                    if (name->value == from->value)
                        if (Tree *to = candidate->to)
                            return to;
                        else
                            return from;

                rewrite_table &rwh = candidate->hash;
                found = rwh.find(key);
                candidate = found == rwh.end() ? NULL : (*found).second;
            }
        } 

        // Select which scope to use next
        if (lookup == SCOPE_LOOKUP)
            next = context->scope;
        else if (STACK_LOOKUP)
            next = context->stack;
    }

    // Not bound
    return NULL;
}


Tree *Context::CreateClosure(Tree *value)
// ----------------------------------------------------------------------------
//   Create a closure to record the current context
// ----------------------------------------------------------------------------
{
    // Quick optimization for constants
    if (!hasConstants && value->IsConstant())
        return value;
    static Name_p closureName = new Name("<closure>");
    Prefix *result = new Prefix(closureName, value);
    result->Set<ClosureInfo>(this);
    return result;
}


Tree *Context::EvaluateClosure(Tree *closure, Tree *value)
// ----------------------------------------------------------------------------
//   Evaluate the closure in the recorded context
// ----------------------------------------------------------------------------
{
    Context *context = closure->Get<ClosureInfo>();
    if (!context)
    {
        Ooops("Internal: Where did the closure $1 come from?", value);
        context = this;
    }

    // Evaluate the result
    Tree *result = context->Evaluate(value);

    // Cache evaluation result in case we evaluate the closure again
    static Name_p evaluatedName = new Name("<evaluated>");
    Prefix *prefix = closure->AsPrefix(); assert(prefix || !"Invalid closure");
    prefix->left = evaluatedName;
    prefix->right = result;

    return result;
}

XL_END


extern "C" void debugrw(XL::Rewrite *r)
// ----------------------------------------------------------------------------
//   For the debugger, dump a rewrite
// ----------------------------------------------------------------------------
{
    if (r)
    {
        std::cerr << r->from << " -> " << r->to << "\n";
        XL::rewrite_table::iterator i;
        for (i = r->hash.begin(); i != r->hash.end(); i++)
            debugrw((*i).second);
    }
}


extern "C" void debugs(XL::Context *c)
// ----------------------------------------------------------------------------
//   For the debugger, dump a symbol table
// ----------------------------------------------------------------------------
{
    using namespace XL;
    std::cerr << "REWRITES IN CONTEXT " << c << "\n";

    XL::rewrite_table::iterator i;
    for (i = c->rewrites.begin(); i != c->rewrites.end(); i++)
        debugrw((*i).second);
}


extern "C" void debugsc(XL::Context *c)
// ----------------------------------------------------------------------------
//   For the debugger, dump a symbol table along the scope
// ----------------------------------------------------------------------------
{
    using namespace XL;
    while (c)
    {
        debugs(c);
        c = c->scope;
    }
}


extern "C" void debugst(XL::Context *c)
// ----------------------------------------------------------------------------
//   For the debugger, dump a symbol table along the stack
// ----------------------------------------------------------------------------
{
    using namespace XL;
    while (c)
    {
        debugs(c);
        c = c->stack;
    }
}
