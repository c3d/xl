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

Context::Context(Context *parent)
// ----------------------------------------------------------------------------
//   Constructor for an execution context
// ----------------------------------------------------------------------------
    : parent(parent), rewrites(),
      hasConstants(parent ? parent->hasConstants : false)
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
    // Check if we rewrite a constant. If so, remember it
    if (form->IsConstant())
        hasConstants = true;

    // If we have a block on the left, define the child of that block
    if (Block *block = form->AsBlock())
        form = block->child;

    // Check that we have only names in the pattern
    ValidateNames(form);

    // Create a rewrite and add it to the table
    Rewrite *rewrite = new Rewrite(form, value);
    ulong key = Hash(form);
    Rewrite_p *parent = &rewrites[key];
    while (Rewrite *where = *parent)
        parent = &where->hash[key];
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


Tree *Context::Evaluate(Tree *what)
// ----------------------------------------------------------------------------
//   Evaluate 'what' in the given context
// ----------------------------------------------------------------------------
{
    // Quick optimization for constants
    if (!hasConstants && what->IsConstant())
        return what;

    // Depth check
    static uint depth = 0;
    depth++;
    if (depth > MAIN->options.stack_depth)
    {
        Ooops("Recursed too deep evaluating $1", what);
        return what;
    }

    // Build the hash key for the tree to evaluate
    ulong key = Hash(what);

    // Loop over all contexts
    for (Context *context = this; context; context = context->parent)
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
                            depth--;
                            return result;
                        }
                    }
                    else if (Name *name = candidate->from->AsName())
                    {
                        Name *vname = what->AsName();
                        if (name->value == vname->value)
                        {
                            Tree *result = candidate->to;
                            if (result != candidate->from)
                                result = Evaluate(result);
                            return result;
                        }
                    }
                    else
                    {
                        // Keep evaluating
                        Context *formContext = new Context(this);
                        if (formContext->Bind(candidate->from, what))
                        {
                            Tree *result = candidate->from;
                            if (Tree *to = candidate->to)
                            {
                                if (to != result)
                                    result = formContext->Evaluate(to);
                            }
                            else
                            {
                                result = xl_evaluate_children(formContext,
                                                              result);
                            }
                            depth--;
                            return result;
                        }
                    }
                }

                rewrite_table &rwh = candidate->hash;
                found = rwh.find(key);
                candidate = found == rwh.end() ? NULL : (*found).second;
            }
        }
    }

    // Error case - Should we raise some error here?
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
    Context *eval = args ? this : (Context *) parent;

    switch(k)
    {
    case INTEGER:
        value = Evaluate(value);
        if (Integer *iv = value->AsInteger())
            return iv->value == ((Integer *) form)->value;
        return false;
    case REAL:
        value = Evaluate(value);
        if (Real *rv = value->AsReal())
            return rv->value == ((Real *) form)->value;
        return false;
    case TEXT:
        value = Evaluate(value);
        if (Text *tv = value->AsText())
            return (tv->value == ((Text *) form)->value &&
                    tv->opening == ((Text *) form)->opening &&
                    tv->closing == ((Text *) form)->closing);
        return false;


    case NAME:
        // Test if the name is already bound, and if so, if trees match
        if (Tree *bound = Bound((Name *) form))
        {
            if (bound == form)
                return true;
            if (EqualTrees(bound, form))
                return true;
            value = eval->Evaluate(value);
            return EqualTrees(bound, value);
        }

        // Define the name in the given context
        if (args)
            args->push_back(value);
        else
            Define(form, value);
        return true;

    case INFIX:
        // Check type declarations
        if (((Infix *) form)->name == ":")
        {
            // Evaluate the type
            Tree *type = ((Infix *) form)->right;
            type = eval->Evaluate(type);

            // We need to evaluate the value
            if (type != tree_type)
            {
                value = eval->Evaluate(value);

                // Check if the value matches the type
                if (!ValueMatchesType(this, type, value, false))
                    return false;
            }

            return Bind(((Infix *) form)->left, value, args);
        }

        // If we match the infix name, we can bind left and right
        if (Infix *infix = value->AsInfix())
            if (((Infix *) form)->name == infix->name)
                return (Bind(((Infix *) form)->left, infix->left, args) &&
                        Bind(((Infix *) form)->right, infix->right, args));

        // Otherwise, we don't have a match
        return false;

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


Tree *Context::Bound(Name *name, bool recurse)
// ----------------------------------------------------------------------------
//   Return the value a name is bound to, or NULL if none...
// ----------------------------------------------------------------------------
{
    // Build the hash key for the tree to evaluate
    ulong key = Hash(name);

    // Loop over all contexts
    for (Context *context = this; context; context = context->parent)
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
                        return candidate->to ? candidate->to : name;

                rewrite_table &rwh = candidate->hash;
                found = rwh.find(key);
                candidate = found == rwh.end() ? NULL : (*found).second;
            }
        }
        if (!recurse)
            break;
    }

    // Not bound
    return NULL;
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
//   For the debugger, dump a symbol table
// ----------------------------------------------------------------------------
{
    using namespace XL;
    while (c)
    {
        debugs(c);
        c = c->parent;
    }
}
