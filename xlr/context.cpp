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
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "context.h"
#include "tree.h"
#include "errors.h"
#include "options.h"
#include "renderer.h"
#include "basics.h"
#include "compiler.h"
#include "runtime.h"
#include "main.h"
#include "opcodes.h"
#include "types.h"
#include "save.h"
#include "cdecls.h"

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <sys/stat.h>

XL_BEGIN

// ============================================================================
//
//   Context: Representation of execution context
//
// ============================================================================

Context::Context()
// ----------------------------------------------------------------------------
//   Constructor for a top-level evaluation context
// ----------------------------------------------------------------------------
    : symbols()
{
    symbols = new Infix("\n", xl_nil, xl_nil);
}


Context::Context(Context *parent)
// ----------------------------------------------------------------------------
//   Constructor creating a child context in a parent context
// ----------------------------------------------------------------------------
    : symbols(parent->symbols)
{
    CreateScope();
}


Context::Context(const Context &source)
// ----------------------------------------------------------------------------
//   Constructor for a top-level evaluation context
// ----------------------------------------------------------------------------
    : symbols(source.symbols)
{}


Context::Context(Infix *symbols)
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

void Context::CreateScope()
// ----------------------------------------------------------------------------
//    Add a local scope to the current context
// ----------------------------------------------------------------------------
{
    symbols = new Infix("\n", xl_nil, symbols, symbols->Position());
}


void Context::PopScope()
// ----------------------------------------------------------------------------
//   Remove the innermost local scope
// ----------------------------------------------------------------------------
{
    if (Infix *enclosing = symbols->right->AsInfix())
        symbols = enclosing;
}


Context *Context::Parent()
// ----------------------------------------------------------------------------
//   Return the parent context
// ----------------------------------------------------------------------------
{
    if (Infix *psyms = symbols->right->AsInfix())
        return new Context(psyms);
    return NULL;
}


Tree *Context::Evaluate(Tree *what)
// ----------------------------------------------------------------------------
//   Evaluate 'what' in the given context
// ----------------------------------------------------------------------------
{
    assert (!GarbageCollector::Running());

    // Check if we need to compile the tree in the current context
    if (!what->code)
    {
        if (!MAIN->compiler->Compile(this, what))
        {
            Ooops("Error compiling $1", what);
            return what;
        }
        if (!what->code)
        {
            Ooops("Internal error: no code generated for $1", what);
            return what;
        }
    }

    return what->code(this, what);
}


Tree *Context::Call(text prefix, TreeList &argList)
// ----------------------------------------------------------------------------
//    Compile a calll
// ----------------------------------------------------------------------------
{
    uint arity = argList.size();
    TreePosition pos = arity ? argList[0]->Position() : ~0;
    Tree *call = new Name(prefix, pos);
    if (arity)
    {
        Tree *args = argList[arity + ~0];
        for (uint a = 1; a < arity; a++)
        {
            Tree *arg = argList[arity + ~a];
            args = new Infix(",", arg, args, pos);
        }
        call = new Prefix(call, args, pos);
    }
    return Evaluate(call);
}



// ============================================================================
// 
//    Entering symbols
// 
// ============================================================================

Tree *Context::ProcessDeclarations(Tree *what)
// ----------------------------------------------------------------------------
//   Process all declarations, return instructions (non-declarations) or NULL
// ----------------------------------------------------------------------------
{
    Tree_p  instrs = NULL;
    Tree_p *instrP = &instrs;
    Tree_p  next   = NULL;

    while (what)
    {
        Tree_p instr = NULL;

        if (Infix *infix = what->AsInfix())
        {
            if (infix->name == "\n")
            {
                // Chain of \n. Normally, we don't need to recurse,
                // except if we have \n on the left of a \n (malformed list)
                what = infix->left;
                if (next)
                    instr = ProcessDeclarations(what);
                else
                    next = infix->right;
                continue;
            }
            else if (infix->name == "->")
            {
                Enter(infix);
            }
            else
            {
                // Other infix is an instruction
                instr = what;
            }
        }
        else if (Prefix *prefix = what->AsPrefix())
        {
            instr = what;
            if (Name *pname = prefix->left->AsName())
            {
                if (pname->value == "data")
                {
                    Define(prefix->right, xl_self);
                    instr = NULL;
                }
                else if (pname->value == "extern")
                {
                    CDeclaration *pcd = new CDeclaration;
                    Infix *normalForm = pcd->Declaration(prefix->right);
                    IFTRACE(xl2c)
                        std::cout << "C:  " << prefix << "\n"
                                  << "XL: " << normalForm << "\n";
                    if (normalForm)
                    {
                        Define(normalForm->left, normalForm->right);
                        prefix->SetInfo<CDeclaration>(pcd);
                        prefix->right->SetInfo<CDeclaration>(pcd);
                        instr = NULL;
                    }
                    else
                    {
                        delete pcd;
                    }
                }
            }
        }
        else
        {
            // Other cases are instructions
            instr = what;
        }

        // Check if we had an instruction to append to the list
        if (instr)
        {
            if (*instrP)
            {
                Infix *chain = new Infix("\n", *instrP, instr,
                                         instr->Position());
                *instrP = chain;
                instrP = &chain->right;
            }
            else
            {
                *instrP = instr;
            }
        }

        // Consider next in chain
        what = next;
        next = NULL;
    }

    return instrs;
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


Infix *Context::Define(Tree *form, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a rewrite in the current context
// ----------------------------------------------------------------------------
{
    Infix *decl = new Infix("->", form, value, form->Position());
    return Enter(decl);
}


Infix *Context::Define(text name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a rewrite in the current context
// ----------------------------------------------------------------------------
{
    Name *nameTree = new Name(name, value->Position());
    return Define(nameTree, value);
}


Infix *Context::Enter(Infix *rewrite)
// ----------------------------------------------------------------------------
//   Enter a known declaration
// ----------------------------------------------------------------------------
{
    // If the rewrite is not good, just exit
    if (rewrite->name != "->")
        return NULL;

    // Find 'from', 'to' and 'hash' for the rewrite
    Tree *from = rewrite->left;
    ulong h = Hash(from, true);

    // Check what we are really defining, and verify if it's a name
    Tree *defined = RewriteDefined(from);
    Name *name = defined->AsName();

    // Validate form names, emit errors in case of problem.
    ValidateNames(from);

    // Find locals symbol table, populate it
    // The context always has the locals on the left and enclosing on the right.
    // In order to allow for log(N) lookup in the locals, we maintain a
    // structure matching the old Rewrite class, which has a declaration
    // and two or more possible children (currently two).
    // That structure has the following layout: (A->B \n (L; R)), where
    // A->B is the local declaration, L and R are the possible children.
    // Children are initially nil.
    Infix *scope = symbols;
    Tree_p &locals = scope->left;
    Tree_p *parent = &locals;
    Infix *result = NULL;
    while (!result)
    {
        // If we have found a nil spot, that's where we can insert
        if (*parent == xl_nil)
        {
            // Initialize the local entry with nil children
            Infix *nil_children = new Infix(";", xl_nil, xl_nil,
                                            rewrite->Position());

            // Create the local entry
            Infix *entry = new Infix("\n", rewrite, nil_children,
                                     rewrite->Position());

            // Insert the entry in the parent
            *parent = entry;

            // We are done
            result = entry;
            break;
        }

        // This should be a rewrite entry, follow it
        Infix *entry = (*parent)->AsInfix();

        // If we are definig a name, signal if we redefine it
        if (name)
        {
            Infix *decl = entry->left->AsInfix();
            Tree *declDef = RewriteDefined(decl->left);
            if (Name *declName = declDef->AsName())
            {
                if (declName->value == name->value)
                {
                    Ooops("Name $1 is redefined", name);
                    Ooops("Previous definition was in $1", decl);
                }
            }
        }

        Infix *children = entry->right->AsInfix();
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
    // Evaluate the value
    value = Evaluate(value);

    // Check if the reference already exists
    Infix *decl = Reference(ref);
    if (!decl)
    {
        // The reference does not exist: we need to create it.

        // Strip outermost block if there is one
        if (Block *block = ref->AsBlock())
            ref = block->child;

        // If we have 'X:integer := 3', define 'X as integer'
        if (Infix *typed = ref->AsInfix())
            if (typed->name == ":")
                typed->name = "as";

        // Enter in the symbol table
        Define(ref, value);
    }
    else
    {
        // Check if the declaration has a type, i.e. it is 'X as integer'
        if (Infix *typeDecl = decl->left->AsInfix())
        {
            if (typeDecl->name == "as")
            {
                Tree *type = typeDecl->right;
                Tree *castedValue = ValueMatchesType(this, type, value, true);
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
        }

        // Update existing value in place
        decl->right = value;
    }

    // Return evaluated assigned value
    return value;
}



// ============================================================================
// 
//    Context attributes
// 
// ============================================================================

Infix *Context::SetOverridePriority(double priority)
// ----------------------------------------------------------------------------
//   Set the override priority for the innermost scope in the context
// ----------------------------------------------------------------------------
{
    return Define("override_priority", new Real(priority));
}


Infix *Context::SetFileName(text filename)
// ----------------------------------------------------------------------------
//   Set the file name the innermost scope in the context
// ----------------------------------------------------------------------------
{
    return Define("file_name", new Text(filename));
}


Infix *Context::EnterDeclarator(text declarator, eval_fn declFn)
// ----------------------------------------------------------------------------
//   Set the file name the innermost scope in the context
// ----------------------------------------------------------------------------
{
    Infix *infix = Define("decl:" + declarator, new Text(declarator));
    infix->right->code = (eval_fn) declFn;
    return infix;
}




// ============================================================================
// 
//    Path management
// 
// ============================================================================

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
    Infix *scope = symbols;
    ulong h0 = Hash(what, false);

    while (scope)
    {
        // Initialize local scope
        Tree_p &locals = scope->left;
        Tree_p *parent = &locals;
        Tree *result = NULL;
        ulong h = h0;

        while (true)
        {
            // If we have found a nil spot, we are done with current scope
            if (*parent == xl_nil)
                break;

            // This should be a rewrite entry, follow it
            Infix *entry = (*parent)->AsInfix();
            Infix *decl = entry->left->AsInfix();
            Infix *children = entry->right->AsInfix();

            // Check that hash matches
            ulong declHash = Hash(decl->left, true);
            if (declHash == h0)
            {
                result = lookup(scope, what, decl, info);
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
        scope = scope->right->AsInfix();
    }

    // Return NULL if all evaluations failed
    return NULL;
}


static Tree *findReference(Infix *scope, Tree *what, Infix *decl, void *)
// ----------------------------------------------------------------------------
//   Return the reference we found
// ----------------------------------------------------------------------------
{
    return decl;
}


Infix *Context::Reference(Tree *form)
// ----------------------------------------------------------------------------
//   Find an existing definition in the symbol table that matches the form
// ----------------------------------------------------------------------------
{
    Tree *result = Lookup(form, findReference, NULL, true);
    if (result)
        if (Infix *decl = result->AsInfix())
            return decl;
    return NULL;
}


static Tree *findValue(Infix *scope, Tree *what, Infix *decl, void *info)
// ----------------------------------------------------------------------------
//   Return the value bound to a given form
// ----------------------------------------------------------------------------
{
    return decl->right;
}


static Tree *findValueX(Infix *scope, Tree *what, Infix *decl, void *info)
// ----------------------------------------------------------------------------
//   Return the value bound to a given form
// ----------------------------------------------------------------------------
{
    Infix_p *rewrite = (Infix_p *) info;
    rewrite[0] = decl;
    rewrite[1] = scope;
    return decl->right;
}


Tree *Context::Bound(Tree *form, bool recurse)
// ----------------------------------------------------------------------------
//   Return the value bound to a given declaration
// ----------------------------------------------------------------------------
{
    Tree *result = Lookup(form, findValue, NULL, recurse);
    return result;
}


Tree *Context::Bound(Tree *form, bool recurse, Infix_p *rewrite, Infix_p *ctx)
// ----------------------------------------------------------------------------
//   Return the value bound to a given declaration
// ----------------------------------------------------------------------------
{
    Infix_p info[2];
    Tree *result = Lookup(form, findValueX, info, recurse);
    if (rewrite)
        *rewrite = info[0];
    if (ctx)
        *ctx = info[1];
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


static ulong listNames(Infix *where, text begin, rewrite_list &list, bool pfx)
// ----------------------------------------------------------------------------
//   List names in the given tree
// ----------------------------------------------------------------------------
{
    ulong count = 0;
    while (where)
    {
        Infix *decl = where->left->AsInfix();
        if (decl && decl->name == "->")
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
        if (where->name == ";" || where->name == "\n")
            count += listNames(where->left->AsInfix(), begin, list, pfx);
        where = decl->right->AsInfix();
    }
    return count;
}


ulong Context::ListNames(text begin, rewrite_list &list,
                        bool recurse, bool includePrefixes)
// ----------------------------------------------------------------------------
//    List names in a context
// ----------------------------------------------------------------------------
{
    Infix *scope = symbols;
    if (!recurse)
        scope = scope->left->AsInfix();
    return listNames(scope, begin, list, includePrefixes);
}



// ============================================================================
// 
//    Hash functions to balance things in the symbol table
// 
// ============================================================================

ulong Context::Hash(text t)
// ----------------------------------------------------------------------------
//   Compute the hash code for a name in the rewrite table
// ----------------------------------------------------------------------------
{
    kind  k = NAME;
    ulong h = 0;

    h = 0xC0DED;
    for (text::iterator p = t.begin(); p != t.end(); p++)
        h = (h * 0x301) ^ *p;
    h = (h << 4) | (ulong) k;

    return h;
}


ulong Context::Hash(Tree *what, bool inDecl)
// ----------------------------------------------------------------------------
//   Compute the hash code in the rewrite table
// ----------------------------------------------------------------------------
// In 'inDecl', we eliminate guards (X when Cond) and types (X as Type)
{
    if (inDecl)
        what = RewriteDefined(what);
 
    kind  k = what->Kind();
    ulong h = 0;
    text  t;

    switch(k)
    {
    case INTEGER:
        h = ((Integer *) what)->value;
        break;
    case REAL:
        {
	    size_t s = sizeof(ulong);
	    if (sizeof(double) < s)
	        s = sizeof(double);
            memcpy(&h, &((Real *) what)->value, s);
        }
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
        if (Name *name = ((Prefix *) what)->left->AsName())
            h = Hash(name, false);
        break;
    case POSTFIX:
        if (Name *name = ((Postfix *) what)->right->AsName())
            h = Hash(name, false);
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
    symbols->left = xl_nil;
}



// ============================================================================
// 
//    Constraints
// 
// ============================================================================

Tree *Constraint::SolveFor(Name *name)
// ----------------------------------------------------------------------------
//   Solve the constraint for the given name
// ----------------------------------------------------------------------------
{
    if (Infix *eq = equation->AsInfix())
    {
        if (eq->name == "=")
        {
            Tree_p &left = eq->left;
            Tree_p &right = eq->right;

            // Check if already solved
            if (left == name)
                return right;

            // Check if we are already in the right form
            if (Name *n = left->AsName())
            {
                if (n->value == name->value)
                {
                    assert (CountName(name, right) == 0 ||
                            !"Invalid equation entered?");
                    return right;
                }
            }

            // Check which side the name is in
            uint cleft = CountName(name, left);
            uint cright = CountName(name, right);

            // Make sure the requested variable is on the left            
            if (cleft == 0 && cright == 1)
            {
                std::swap(left, right);
                std::swap(cleft, cright);
            }

            // If the variable doesn't appear or appears twice, give up
            if (cleft != 1 || cright != 0)
                return NULL;

            // Loop until we solved it
            while (true)
            {
                // Check if we solved it
                if (Name *n = left->AsName())
                {
                    if (n->value == name->value)
                        return right;
                    else
                        return NULL;
                }

                // Check if we have an infix on the left
                if (Infix *infix = left->AsInfix())
                {
                    text iname = infix->name;
                    cleft = CountName(name, infix->left);
                    if (iname == "+")
                    {
                        if (cleft == 1)
                        {
                            // X+a=b -> X=b-a
                            right = new Infix("-", right, infix->right);
                            left = infix->left;
                        }
                        else
                        {
                            // a+X=b -> X=b-a
                            right = new Infix("-", right, infix->left);
                            left = infix->right;
                        }
                    }
                    else if (iname == "-")
                    {
                        if (cleft == 1)
                        {
                            // X-a=b -> X=b+a
                            right = new Infix("+", right, infix->right);
                            left = infix->left;
                        }
                        else
                        {
                            // a-X=b -> X=a-b
                            right = new Infix("-", infix->left, right);
                            left = infix->right;
                        }
                    }
                    else if (iname == "*")
                    {
                        if (cleft == 1)
                        {
                            // X*a=b -> X=b/a
                            right = new Infix("/", right, infix->right);
                            left = infix->left;
                        }
                        else
                        {
                            // a*X=b -> X=b/a
                            right = new Infix("/", right, infix->left);
                            left = infix->right;
                        }
                    }
                    else if (iname == "/")
                    {
                        if (cleft == 1)
                        {
                            // X/a=b -> X=b*a
                            right = new Infix("*", right, infix->right);
                            left = infix->left;
                        }
                        else
                        {
                            // a/X=b -> X=a/b
                            right = new Infix("/", infix->left, right);
                            left = infix->right;
                        }
                    }
                    else
                    {
                        return NULL;
                    }

                }
                else if (Prefix *prefix = left->AsPrefix())
                {
                    if (Name *pn = prefix->left->AsName())
                    {
                        if (pn->value == "+")
                        {
                            left = prefix->right;
                        }
                        else if (pn->value == "-")
                        {
                            left = prefix->right;
                            right = new Prefix(prefix->left, right);
                        }
                        else
                        {
                            return NULL;
                        }
                    }
                    else
                    {
                        return NULL;
                    }
                }
                else if (Block *block = left->AsBlock())
                {
                    if (block->IsParentheses())
                        left = block->child;
                    else
                        return NULL;
                }
                else
                {
                    return NULL;
                }
            }
                
        }
    }

    return NULL;
}


uint Constraint::CountName(Name *name, Tree *expr)
// ----------------------------------------------------------------------------
//   Count how many times the name occurs in the given expression
// ----------------------------------------------------------------------------
{
    if (Name *n = expr->AsName())
        return n->value == name->value ? 1 : 0;

    if (Block *block = expr->AsBlock())
        return CountName(name, block->child);
    if (Infix *infix = expr->AsInfix())
        return CountName(name, infix->left) + CountName(name, infix->right);
    if (Prefix *prefix = expr->AsPrefix())
        return CountName(name, prefix->right);
    if (Postfix *postfix = expr->AsPostfix())
        return CountName(name, postfix->left);

    return 0;
}


bool Constraint::IsValid(Tree *eq, std::set<text> &vars)
// ----------------------------------------------------------------------------
//   Check if a given constraint is valid
// ----------------------------------------------------------------------------
{
    // Count names, they should occur only once
    if (Name *name = eq->AsName())
    {
        if (vars.count(name->value))
            return false;
        vars.insert(name->value);
        return true;
    }

    // Check terminals
    if (eq->AsInteger() || eq->AsReal())
        return true;

    // Check that infix operators are things we know how to deal with
    if (Infix *infix = eq->AsInfix())
    {
        // Check if this is a known operator
        if (infix->name == "=")
        {
            if (vars.count("="))
                return false;
            vars.insert("=");
        }
        else if (infix->name != "+" && infix->name != "-" &&
                 infix->name != "*" && infix->name != "/")
        {
            return false;
        }

        // Valid if both sides are valid
        return (IsValid(infix->left, vars) && IsValid(infix->right, vars));
    }

    // Check prefix operators that we know
    if (Prefix *prefix = eq->AsPrefix())
        if (Name *pn = prefix->left->AsName())
            if (pn->value == "+" || pn->value == "-")
                return IsValid(prefix->right, vars);

    // Check blocks
    if (Block *block = eq->AsBlock())
        if (block->IsParentheses())
            return IsValid(block->child, vars);

    // Other cases are invalid for the moment.
    return false;
}

XL_END


extern "C"
{
void debugl(XL::Infix *scope)
// ----------------------------------------------------------------------------
//    Helper to show a local scope in a symbol table
// ----------------------------------------------------------------------------
{
    if (XL::Allocator<XL::Infix>::IsAllocated(scope))
    {
        while (scope)
        {
            XL::Infix *decl = scope->left->AsInfix();
            XL::Infix *children = scope->right->AsInfix();
            if (decl && decl->name == "->")
            {
                std::cerr << decl->left
                          << "\t->\t" 
                          << XL::ShortTreeForm(decl->right)
                          << "\n";
            }
            else
            {
                std::cerr << "Unknown: " << scope->left << "\n";
            }
            if (children)
            {
                XL::Infix *left = children->left->AsInfix();
                XL::Infix *right = children->right->AsInfix();
                if (left && right)
                {
                    debugl(left);
                    scope = right;
                }
                else if (left)
                {
                    scope = left;
                }
                else
                {
                    scope = right;
                }
            }
            else
            {
                scope = NULL;
            }
        }
    } 
    else
    {
        std::cerr << "Cowardly refusing to render unknown scope pointer "
                  << (void *) scope << "\n";
    }

}


void debugi(XL::Infix *scope)
// ----------------------------------------------------------------------------
//   Helper to show an infix as a symbol table for debugging purpose
// ----------------------------------------------------------------------------
//   The infix can be shown using debug(), but it's less convenient
{
    if (XL::Allocator<XL::Infix>::IsAllocated(scope))
    {
        if (scope && (scope->name == ";" || scope->name == "\n"))
            scope = scope->left->AsInfix();
        else
            scope = NULL;
        debugl(scope);
    }
    else
    {
        std::cerr << "Cowardly refusing to render unknown scope pointer "
                  << (void *) scope << "\n";
    }
}


void debugs(XL::Context *context)
// ----------------------------------------------------------------------------
//   Helper to show a single context for debugging purpose
// ----------------------------------------------------------------------------
//   A context symbols can also be shown with debug(), but it's less convenient
{
    XL::Infix *scope = context->symbols;
    if (XL::Allocator<XL::Infix>::IsAllocated(scope))
    {
        std::cerr << "SYMBOLS AT " << (void *) scope << "\n";
        debugi(scope);
    } 
    else
    {
        std::cerr << "Cowardly refusing to render unknown scope pointer "
                  << (void *) scope << "\n";
    }
   
}


void debugc(XL::Context *context)
// ----------------------------------------------------------------------------
//   Helper to show a context for debugging purpose
// ----------------------------------------------------------------------------
//   A context symbols can also be shown with debug(), but it's less convenient
{
    XL::Infix *scope = context->symbols;
    if (XL::Allocator<XL::Infix>::IsAllocated(scope))
    {
        ulong depth = 0;
        while (scope && (scope->name == ";" || scope->name == "\n"))
        {
            std::cerr << "SYMBOLS #" << depth++
                      << " AT " << (void *) scope << "\n";
            debugi(scope);
            scope = scope->right->AsInfix();
        }
        if (scope)
            std::cerr << "FINAL: " << scope << "\n";
    } 
    else
    {
        std::cerr << "Cowardly refusing to render unknown scope pointer "
                  << (void *) scope << "\n";
    }
   
}

}
