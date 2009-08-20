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

XL_BEGIN

// ============================================================================
// 
//   Namespace: symbols management
// 
// ============================================================================

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


Tree *Namespace::Infix(text name, bool deep)
// ----------------------------------------------------------------------------
//   Lookup an infix operator in the symbol table
// ----------------------------------------------------------------------------
{
    for (Namespace *c = this; c; c = c->Parent())
    {
        if (c->infix_symbols.count(name))
            return c->infix_symbols[name];
        if (!deep)
            break;
    }
    return NULL;
}


Tree *Namespace::Block(text name, bool deep)
// ----------------------------------------------------------------------------
//   Lookup a block operator in the symbol table
// ----------------------------------------------------------------------------
{
    for (Namespace *c = this; c; c = c->Parent())
    {
        if (c->block_symbols.count(name))
            return c->block_symbols[name];
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


void Namespace::EnterInfix (text name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter an infix operator
// ----------------------------------------------------------------------------
{
    infix_symbols[name] = value;
}


void Namespace::EnterBlock (text name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a block operator
// ----------------------------------------------------------------------------
{
    block_symbols[name] = value;
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
//   An action used for garbage collection
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
//    Error handling
// 
// ============================================================================

Tree * Context::Error(text message, Tree *args)
// ----------------------------------------------------------------------------
//   Execute the innermost error handler
// ----------------------------------------------------------------------------
{
    Tree *handler = ErrorHandler();
    if (handler)
    {
        Tree *info = new XL::Text (message);
        if (args)
            info = new XL::Infix(",", info, args, args->Position());
        return handler->Call(this, info);
    }

    // No handler: terminate
    std::cerr << "Error: No error handler\n";
    if (!args)
        std::cerr << "Message: " << message << "\n";
    else
        errors.Error(message, args);
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

XL_END
