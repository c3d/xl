// *****************************************************************************
// context.cpp                                                        XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2003-2004,2009-2012,2014-2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010-2013, Jérôme Forissier <jerome@taodyne.com>
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

#include "context.h"
#include "tree.h"
#include "errors.h"
#include "options.h"
#include "renderer.h"
#include "basics.h"
#include "runtime.h"
#include "main.h"
#include "opcodes.h"
#include "save.h"
#include "cdecls.h"
#include "interpreter.h"
#include "bytecode.h"

#ifndef INTERPRETER_ONLY
#include "compiler.h"
#include "types.h"
#endif // INTERPRETER_ONLY

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <sys/stat.h>

RECORDER(scope_enter,  16, "Entering items in the scope");
RECORDER(scope_lookup, 16, "Lookup items from a scope");

XL_BEGIN

// ============================================================================
//
//   Context: Representation of execution context
//
// ============================================================================

#define REWRITE_NAME            "\n"
#define REWRITE_CHILDREN_NAME   ";"


uint Context::hasRewritesForKind = 0;

Context::Context()
// ----------------------------------------------------------------------------
//   Constructor for a top-level evaluation context
// ----------------------------------------------------------------------------
    : symbols()
{
    symbols = new Scope(xl_nil, xl_nil);
}


Context::Context(Context *parent, TreePosition pos)
// ----------------------------------------------------------------------------
//   Constructor creating a child context in a parent context
// ----------------------------------------------------------------------------
    : symbols(parent->symbols)
{
    CreateScope(pos);
}


Context::Context(const Context &source)
// ----------------------------------------------------------------------------
//   Constructor for a top-level evaluation context
// ----------------------------------------------------------------------------
    : symbols(source.symbols)
{}


Context::Context(Scope *symbols)
// ----------------------------------------------------------------------------
//   Constructor from a known symbol table
// ----------------------------------------------------------------------------
    : symbols(symbols)
{}


Context::~Context()
// ----------------------------------------------------------------------------
//   Destructor for execution context
// ----------------------------------------------------------------------------
{}



// ============================================================================
//
//    High-level evaluation functions
//
// ============================================================================

Scope *Context::CreateScope(TreePosition pos)
// ----------------------------------------------------------------------------
//    Add a local scope to the current context
// ----------------------------------------------------------------------------
{
    symbols = new Scope(symbols, xl_nil, pos);
    return symbols;
}


void Context::PopScope()
// ----------------------------------------------------------------------------
//   Remove the innermost local scope
// ----------------------------------------------------------------------------
{
    if (Scope *enclosing = Enclosing(symbols))
        symbols = enclosing;
}


Context *Context::Parent()
// ----------------------------------------------------------------------------
//   Return the parent context
// ----------------------------------------------------------------------------
{
    if (Scope *psyms = Enclosing(symbols))
        return new Context(psyms);
    return nullptr;
}



// ============================================================================
//
//    Entering symbols
//
// ============================================================================

RECORDER(xl2c, 64, "XL to C translation");

bool Context::ProcessDeclarations(Tree *what)
// ----------------------------------------------------------------------------
//   Process all declarations, return true if there are instructions
// ----------------------------------------------------------------------------
{
    Tree_p next   = nullptr;
    bool   result = false;

    while (what)
    {
        bool isInstruction = true;
        if (Infix *infix = what->AsInfix())
        {
            if (IsDeclaration(infix))
            {
                Enter(infix);
                isInstruction = false;
            }
            else if (IsSequence(infix))
            {
                // Chain of declarations, avoiding recursing if possible.
                if (Infix *left = infix->left->AsInfix())
                {
                    isInstruction = false;
                    if (IsDeclaration(left))
                        Enter(left);
                    else
                        isInstruction = ProcessDeclarations(left);
                }
                else if (Prefix *left = infix->left->AsPrefix())
                {
                    isInstruction = ProcessDeclarations(left);
                }
                next = infix->right;
            }
        }
        else if (Prefix *prefix = what->AsPrefix())
        {
            if (Name *pname = prefix->left->AsName())
            {
                if (pname->value == "data")
                {
                    Define(prefix->right, xl_self);
                    isInstruction = false;
                }
                else if (pname->value == "extern")
                {
                    CDeclaration *pcd = new CDeclaration;
                    Infix *normalForm = pcd->Declaration(prefix->right);
                    record(xl2c, "C: %t is XL: %t", prefix, normalForm);
                    if (normalForm)
                    {
                        // Process C declarations only in optimized mode
                        Define(normalForm->left, normalForm->right);
                        prefix->SetInfo<CDeclaration>(pcd);
                        isInstruction = false;
                    }
                    else
                    {
                        delete pcd;
                    }
                }
            }
        }

        // Check if we see instructions
        result |= isInstruction;

        // Consider next in chain
        what = next;
        next = nullptr;
    }
    return result;
}


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


Rewrite *Context::Define(Tree *form, Tree *value, bool overwrite)
// ----------------------------------------------------------------------------
//   Enter a rewrite in the current context
// ----------------------------------------------------------------------------
{
    Infix *decl = new Infix("is", form, value, form->Position());
    return Enter(decl, overwrite);
}


Rewrite *Context::Define(text name, Tree *value, bool overwrite)
// ----------------------------------------------------------------------------
//   Enter a rewrite in the current context
// ----------------------------------------------------------------------------
{
    Name *nameTree = new Name(name, value->Position());
    return Define(nameTree, value, overwrite);
}


Rewrite *Context::Enter(Infix *rewrite, bool overwrite)
// ----------------------------------------------------------------------------
//   Enter a known declaration
// ----------------------------------------------------------------------------
{
    // If the rewrite is not good, just exit
    if (!IsDeclaration(rewrite))
        return nullptr;

    // In interpreted mode, just skip any C declaration
#ifndef INTERPRETER_ONLY
    if (Opt::optimize <= 1)
#endif
        if (Prefix *cdecl = rewrite->right->AsPrefix())
            if (Name *cname = cdecl->left->AsName())
                if (cname->value == "C")
                    return nullptr;

    // Find 'from', 'to' and 'hash' for the rewrite
    Tree *from = rewrite->left;

    // Check what we are really defining, and verify if it's a name
    Tree *defined = RewriteDefined(from);
    Name *name = defined->AsName();
    ulong h = Hash(defined);

    // Record which kinds we have rewrites for
    HasOneRewriteFor(defined->Kind());

    // Validate form names, emit errors in case of problem.
    ValidateNames(from);

    // Find locals symbol table, populate it
    // The context always has the locals on the left and enclosing on the right.
    // In order to allow for log(N) lookup in the locals, we maintain a
    // structure matching the old Rewrite class, which has a declaration
    // and two or more possible children (currently two).
    // That structure has the following layout: (A->B ; (L; R)), where
    // A->B is the local declaration, L and R are the possible children.
    // Children are initially nil.
    Scope   *scope  = symbols;
    Tree_p  &locals = ScopeLocals(scope);
    Tree_p  *parent = &locals;
    Rewrite *result = nullptr;
    while (!result)
    {
        // If we have found a nil spot, that's where we can insert
        if (*parent == xl_nil)
        {
            // Initialize the local entry with nil children
            RewriteChildren *nil_children =
                new RewriteChildren(REWRITE_CHILDREN_NAME,
                                    xl_nil, xl_nil,
                                    rewrite->Position());

            // Create the local entry
            Rewrite *entry = new Rewrite(REWRITE_NAME, rewrite, nil_children,
                                         rewrite->Position());

            // Insert the entry in the parent
            *parent = entry;

            // We are done
            result = entry;
            break;
        }

        // This should be a rewrite entry, follow it
        Rewrite *entry = (*parent)->As<Rewrite>();

        // If we are definig a name, signal if we redefine it
        if (name)
        {
            Infix *decl = RewriteDeclaration(entry);
            Tree *declDef = RewriteDefined(decl->left);
            if (Name *declName = declDef->AsName())
            {
                if (declName->value == name->value)
                {
                    if (overwrite)
                    {
                        decl->right = rewrite->right;
                        return entry;
                    }
                    else
                    {
                        Ooops("Name $1 is redefined", name);
                        Ooops("Previous definition was in $1", decl);
                    }
                }
            }
        }

        RewriteChildren *children = RewriteNext(entry);
        if (h & 1)
            parent = &children->right;
        else
            parent = &children->left;
        h = Rehash(h);
    }

    // Return the entry we created
    return result;
}


Tree *Context::Assign(Tree *ref, Tree *value)
// ----------------------------------------------------------------------------
//   Perform an assignment in the given context
// ----------------------------------------------------------------------------
{
    // Check if the reference already exists
    Infix *decl = Reference(ref);
    if (!decl)
    {
        // The reference does not exist: we need to create it.

        // Strip outermost block if there is one
        if (Block *block = ref->AsBlock())
            ref = block->child;

        // Enter in the symbol table
        Define(ref, value);
    }
    else
    {
        // Check if the declaration has a type, i.e. it is 'X as integer'
        if (Tree *type = RewriteType(decl->left))
        {
            Scope *scope = Symbols();
            Tree *castedValue = xl_typecheck(scope, type, value);
            if (castedValue)
            {
                value = castedValue;
            }
            else
            {
                Ooops("New value $1 does not match existing type", value);
                Ooops("for declaration $1", decl);
                value = decl->right; // Preserve existing value
            }
        }

        // Update existing value in place
        decl->right = value;
    }

    // Return evaluated assigned value
    return value;
}



// ============================================================================
//
//    Tree attributes
//
// ============================================================================

// Set and get per-context tree attributes
Tree *Context::Info(text key, Tree *what, bool recurse)
// ----------------------------------------------------------------------------
//    Return the information associated with the given attribute
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << key << "@" << (void *) what;
    return Named(out.str(), recurse);
}


Rewrite *Context::SetInfo(text key, Tree *what, Tree *value)
// ----------------------------------------------------------------------------
//   Set the information associated with the tree
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << key << "@" << (void *) what;
    return Define(out.str(), value, false);
}


Tree *Context::Type(Tree *what)
// ----------------------------------------------------------------------------
//   Return the type associated with the value
// ----------------------------------------------------------------------------
{
    return Info("type", what);
}


Rewrite *Context::SetType(Tree *what, Tree *type)
// ----------------------------------------------------------------------------
//   Set the type associated with the value
// ----------------------------------------------------------------------------
{
    return SetInfo("type", what, type);
}



// ============================================================================
//
//    Context attributes
//
// ============================================================================

Rewrite *Context::SetAttribute(text attribute, Tree *value, bool owr)
// ----------------------------------------------------------------------------
//   Set an attribute for the innermost context
// ----------------------------------------------------------------------------
{
    return Define(attribute, value, owr);
}


Rewrite *Context::SetAttribute(text attribute, longlong value, bool owr)
// ----------------------------------------------------------------------------
//   Set an attribute for the innermost context
// ----------------------------------------------------------------------------
{
    return SetAttribute(attribute,new Integer(value,symbols->Position()),owr);
}


Rewrite *Context::SetAttribute(text attribute, double value, bool owr)
// ----------------------------------------------------------------------------
//   Set the override priority for the innermost scope in the context
// ----------------------------------------------------------------------------
{
    return Define(attribute, new Real(value, symbols->Position()), owr);
}


Rewrite *Context::SetAttribute(text attribute, text value, bool owr)
// ----------------------------------------------------------------------------
//   Set the file name the innermost scope in the context
// ----------------------------------------------------------------------------
{
    return Define(attribute, new Text(value, symbols->Position()), owr);
}



// ============================================================================
//
//    Path management
//
// ============================================================================

void Context::SetPrefixPath(text prefix, text path)
// ----------------------------------------------------------------------------
//   Set the path for the given prefix
// ----------------------------------------------------------------------------
{
}


text Context::ResolvePrefixedPath(text path)
// ----------------------------------------------------------------------------
//   Resolve the file name in the current paths
// ----------------------------------------------------------------------------
{
    return path;
}



// ============================================================================
//
//    Looking up symbols
//
// ============================================================================

Tree *Context::Lookup(Tree *what, lookup_fn lookup, void *info, bool recurse)
// ----------------------------------------------------------------------------
//   Lookup a tree using the given lookup function
// ----------------------------------------------------------------------------
{
    // Quick exit if we have no rewrite for that tree kind
    if (!HasRewritesFor(what->Kind()))
        return nullptr;

    Scope * scope = symbols;
    ulong   h0    = Hash(what);

    while (scope)
    {
        // Initialize local scope
        Tree_p &locals = ScopeLocals(scope);
        Tree_p *parent = &locals;
        Tree *result = nullptr;
        ulong h = h0;

        while (true)
        {
            // If we have found a nil spot, we are done with current scope
            if (*parent == xl_nil)
                break;

            // This should be a rewrite entry, follow it
            Rewrite *entry = (*parent)->As<Rewrite>();
            XL_ASSERT(entry && entry->name == REWRITE_NAME);
            Infix *decl = RewriteDeclaration(entry);
            XL_ASSERT(!decl || IsDeclaration(decl));
            RewriteChildren *children = RewriteNext(entry);
            XL_ASSERT(children && children->name == REWRITE_CHILDREN_NAME);

            // Check that hash matches
            Tree *defined = RewriteDefined(decl->left);
            ulong declHash = Hash(defined);
            if (declHash == h0)
            {
                result = lookup(symbols, scope, what, decl, info);
                if (result)
                    return result;
            }

            // Keep going in local symbol table
            if (h & 1)
                parent = &children->right;
            else
                parent = &children->left;
            h = Rehash(h);
        }

        // Not found in this scope. Keep going with next scope if recursing
        // The last top-level global will be nil, so we will end with scope=NULL
        if (!recurse)
            break;
        scope = Enclosing(scope);
    }

    // Return NULL if all evaluations failed
    return nullptr;
}


static Tree *findReference(Scope *, Scope *, Tree *what, Infix *decl, void *)
// ----------------------------------------------------------------------------
//   Return the reference we found
// ----------------------------------------------------------------------------
{
    return decl;
}


Rewrite *Context::Reference(Tree *form, bool recurse)
// ----------------------------------------------------------------------------
//   Find an existing definition in the symbol table that matches the form
// ----------------------------------------------------------------------------
{
    if (Tree *result = Lookup(form, findReference, nullptr, recurse))
        if (Rewrite *decl = result->As<Rewrite>())
            return decl;
    return nullptr;
}


Tree *Context::DeclaredForm(Tree *form)
// ----------------------------------------------------------------------------
//   Find the original declaration associated to a given form
// ----------------------------------------------------------------------------
{
    if (Tree *result = Lookup(form, findReference, nullptr, true))
        if (Rewrite *decl = result->As<Rewrite>())
            return RewriteDefined(decl->left);
    return nullptr;
}


static Tree *findValue(Scope *, Scope *, Tree *what, Infix *decl, void *info)
// ----------------------------------------------------------------------------
//   Return the value bound to a given form
// ----------------------------------------------------------------------------
{
    if (what->IsLeaf())
        if (!Tree::Equal(what, RewriteDefined(decl->left)))
            return nullptr;
    return decl->right;
}


static Tree *findValueX(Scope *, Scope *scope,
                        Tree *what, Infix *decl, void *info)
// ----------------------------------------------------------------------------
//   Return the value bound to a given form, as well as its scope and decl
// ----------------------------------------------------------------------------
{
    if (what->IsLeaf())
        if (!Tree::Equal(what, RewriteDefined(decl->left)))
            return nullptr;
    Prefix *rewriteInfo = (Prefix *) info;
    rewriteInfo->left = scope;
    rewriteInfo->right = decl;
    return decl->right;
}


Tree *Context::Bound(Tree *form, bool recurse)
// ----------------------------------------------------------------------------
//   Return the value bound to a given declaration
// ----------------------------------------------------------------------------
{
    Tree *result = Lookup(form, findValue, nullptr, recurse);
    return result;
}


Tree *Context::Bound(Tree *form, bool recurse, Rewrite_p *rewrite, Scope_p *ctx)
// ----------------------------------------------------------------------------
//   Return the value bound to a given declaration
// ----------------------------------------------------------------------------
{
    Prefix info(nullptr, nullptr);
    Tree *result = Lookup(form, findValueX, &info, recurse);
    if (ctx)
        *ctx = info.left->As<Scope>();
    if (rewrite)
        *rewrite = info.right->As<Rewrite>();
    return result;
}


Tree *Context::Named(text name, bool recurse)
// ----------------------------------------------------------------------------
//   Return the value bound to a given name
// ----------------------------------------------------------------------------
{
    Name nameTree(name);
    return Bound(&nameTree, recurse);
}


bool Context::IsEmpty()
// ----------------------------------------------------------------------------
//   Return true if the context has no declaration inside
// ----------------------------------------------------------------------------
{
    return symbols->right == xl_nil;
}


static ulong listNames(Rewrite *where, text begin, RewriteList &list, bool pfx)
// ----------------------------------------------------------------------------
//   List names in the given tree
// ----------------------------------------------------------------------------
{
    ulong count = 0;
    while (where)
    {
        Infix *decl = RewriteDeclaration(where);
        if (decl && IsDeclaration(decl))
        {
            Tree *declared = decl->left;
            Name *name = declared->AsName();
            if (!name && pfx)
            {
                Prefix *prefix = declared->AsPrefix();
                if (prefix)
                    name = prefix->left->AsName();
            }
            if (name && name->value.find(begin) == 0)
            {
                list.push_back(decl);
                count++;
            }
        }
        if (IsSequence(where))
            count += listNames(where->left->As<Rewrite>(), begin, list, pfx);
        where = decl->right->As<Rewrite>();
    }
    return count;
}


ulong Context::ListNames(text begin, RewriteList &list,
                         bool recurse, bool includePrefixes)
// ----------------------------------------------------------------------------
//    List names in a context
// ----------------------------------------------------------------------------
{
    Scope *scope = symbols;
    ulong count = 0;
    while (scope)
    {
        Rewrite *locals = ScopeRewrites(scope);
        count += listNames(locals, begin, list, includePrefixes);
        if (!recurse)
            scope = nullptr;
        else
            scope = Enclosing(scope);
    }
    return count;
}



// ============================================================================
//
//    Hash functions to balance things in the symbol table
//
// ============================================================================

static inline ulong HashText(const text &t)
// ----------------------------------------------------------------------------
//   Compute the has for some text
// ----------------------------------------------------------------------------
{
    ulong h = 0;
    uint  l = t.length();
    kstring ptr = t.data();
    if (l > 8)
        l = 8;
    for (uint i = 0; i < l; i++)
        h = (h * 0x301) ^ *ptr++;
    return h;
}


static inline longlong hashRealToInteger(double value)
// ----------------------------------------------------------------------------
//   Force a static conversion without "breaking strict aliasing rules"
// ----------------------------------------------------------------------------
{
    union { double d; longlong i; } u;
    u.d = value;
    return u.i;
}


ulong Context::Hash(Tree *what)
// ----------------------------------------------------------------------------
//   Compute the hash code in the rewrite table
// ----------------------------------------------------------------------------
{
    kind        k = what->Kind();
    ulong       h = 0xC0DEDUL + 0x29912837UL*k;

    switch(k)
    {
    case INTEGER:
        h += ((Integer *) what)->value;
        break;
    case REAL:
        h += hashRealToInteger(((Real *) what)->value);
        break;
    case TEXT:
        h += HashText(((Text *) what)->value);
        break;
    case NAME:
        h += HashText(((Name *) what)->value);
        break;
    case BLOCK:
        h += HashText(((Block *) what)->opening);
        break;
    case INFIX:
        h += HashText(((Infix *) what)->name);
        break;
    case PREFIX:
        if (Name *name = ((Prefix *) what)->left->AsName())
            h += HashText(name->value);
        break;
    case POSTFIX:
        if (Name *name = ((Postfix *) what)->right->AsName())
            h += HashText(name->value);
        break;
    }

    return h;
}



// ============================================================================
//
//    Utility functions
//
// ============================================================================

void Context::Clear()
// ----------------------------------------------------------------------------
//   Clear the symbol table
// ----------------------------------------------------------------------------
{
    symbols->right = xl_nil;
}


void Context::Dump(std::ostream &out, Scope *scope, bool recurse)
// ----------------------------------------------------------------------------
//   Dump the symbol table to the given stream
// ----------------------------------------------------------------------------
{
    while (scope)
    {
        Scope *parent = Enclosing(scope);
        Rewrite *rw = ScopeRewrites(scope);
        Dump(out, rw);
        if (parent)
            out << "// Parent " << (void *) parent << "\n";
        if (!recurse)
            break;
        scope = parent;
    }
}


void Context::Dump(std::ostream &out, Rewrite *rw)
// ----------------------------------------------------------------------------
//   Dump the symbol table to the given stream
// ----------------------------------------------------------------------------
{
    while (rw)
    {
        Infix * decl = RewriteDeclaration(rw);
        Rewrite *next = RewriteNext(rw);

        if (rw->name != REWRITE_NAME && rw->name != REWRITE_CHILDREN_NAME)
        {
            out << "SCOPE?" << rw << "\n";
        }

        if (decl)
        {
            if (IsDeclaration(decl))
                out << decl->left
                    << decl->name
                    << ShortTreeForm(decl->right) << "\n";
            else
                out << "DECL?" << decl << "\n";
        }
        else if (rw->left != xl_nil)
        {
            out << "LEFT?" << rw->left << "\n";
        }

        // Iterate, avoid recursion in the common case of enclosed scopes
        if (next)
        {
            Infix *nextl = next->left->AsInfix();
            Infix *nextr = next->right->AsInfix();

            if (nextl && nextr)
            {
                Dump(out, nextl);
                rw = nextr;
            }
            else if (nextl)
            {
                rw = nextl;
            }
            else
            {
                rw = nextr;
            }
        }
        else
        {
            rw = nullptr;
        }
    } // while (rw)
}


XL_END

XL::Scope *xldebug(XL::Scope *scope)
// ----------------------------------------------------------------------------
//    Helper to show a global scope in a symbol table
// ----------------------------------------------------------------------------
{
    if (XL::Allocator<XL::Scope>::IsAllocated(scope))
    {
        XL::Context::Dump(std::cerr, scope, false);
        scope = Enclosing(scope);
    }
    else
    {
        std::cerr << "Cowardly refusing to render unknown scope pointer "
                  << (void *) scope << "\n";
    }
    return scope;
}


XL::Rewrite *xldebug(XL::Rewrite *rw)
// ----------------------------------------------------------------------------
//   Helper to show an infix as a local symbol table for debugging purpose
// ----------------------------------------------------------------------------
//   The infix can be shown using debug(), but it's less convenient
{
    if (XL::Allocator<XL::Rewrite>::IsAllocated(rw))
        XL::Context::Dump(std::cerr, (XL::Rewrite *) rw);
    else
        std::cerr << "Cowardly refusing to render unknown rewrite pointer "
                  << (void *) rw << "\n";
    return rw;
}


XL::Scope *xldebug(XL::Context *context)
// ----------------------------------------------------------------------------
//   Helper to show a context for debugging purpose
// ----------------------------------------------------------------------------
{
    if (XL::Allocator<XL::Context>::IsAllocated(context))
    {
        XL::Scope *scope = context->Symbols();
        return xldebug(scope);
    }
    else
    {
        std::cerr << "Cowardly refusing to render unknown context pointer "
                  << (void *) context << "\n";
        return nullptr;
    }
}
XL::Scope *xldebug(XL::Context_p c) { return xldebug((XL::Context *) c); }
