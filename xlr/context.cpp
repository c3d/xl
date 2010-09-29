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
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
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


Tree *Context::ProcessDeclarations(Tree *what)
// ----------------------------------------------------------------------------
//   Process all declarations, return instructions (non-declarations) or NULL
// ----------------------------------------------------------------------------
{
    Tree_p  instrs = NULL;
    Tree_p *instrP = &instrs;
    Tree   *next   = NULL;

    while (what)
    {
        Tree *instr = NULL;

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
                Define(infix->left, infix->right);
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
                    DefineData(prefix->right);
                    instr = NULL;
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


Rewrite *Context::Define(Tree *form, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a rewrite in the context
// ----------------------------------------------------------------------------
{
    // If we have a block on the left, define the child of that block
    if (Block *block = form->AsBlock())
        form = block->child;

    // Check if we rewrite a constant. If so, remember that we rewrite them
    if (form->IsConstant())
        hasConstants = true;

    // Check that we have only names in the pattern
    ValidateNames(form);

    // Create a rewrite
    Rewrite *rewrite = new Rewrite(form, value);
    ulong key = HashForm(form);

    // Walk through existing rewrites
    Rewrite_p *parent = &rewrites[key]; 

    // Check if we insert a name. If so, we shouldn't redefine
    if (Name *name = form->AsName())
    {
        // Name: check for redefinitions
        while (Rewrite *where = *parent)
        {
            if (where->from == form && where->to == value)
                return where;

            if (Name *existing = where->from->AsName())
            {
                if (existing->value == name->value)
                {
                    if (where->to->GetInfo<PrefixDefinitionsInfo>())
                    {
                        // Implicit definition for prefix name, override
                        where->to = value;
                        break;
                    }
                    else
                    {
                        Ooops("Name $1 already exists", name);
                        Ooops("Previous definition was $1", existing);
                    }
                }
            }
            parent = &where->hash[key];
        }
    }           
    else
    {
        // Not a name, won't check for redefinitions
        while (Rewrite *where = *parent)
        {
            if (where->from == form && where->to == value)
                return where;
            parent = &where->hash[key];
        }
    }

    // Create the rewrite
    *parent = rewrite;

    // Check if we defined a prefix with a name on the left.
    // If so, this also implicitly defines the associated name
    if (Prefix *prefix = form->AsPrefix())
    {
        // We only do that if defining a named prefix ("function"), like "foo 0"
        if (Name *defined = prefix->left->AsName())
        {
            // We don't do that for operators, e.g. "+0" will not define "+"
            // (debatable. We'd need to selectively disable ValidateNames)
            if (defined->value.size() && isalpha(defined->value[0]))
            {
                // Selector for the definition
                Tree *selector = prefix->right;
                typedef PrefixDefinitionsInfo PDI;

                // Check whatever was already defined
                if (Tree *existing = Bound(defined))
                {
                    if (Block *block = existing->AsBlock())
                    {
                        // Append to last definition we had for that name
                        if (PDI *info = block->GetInfo<PDI>())
                        {
                            // Extract definition for the name
                            Infix *nameDef = new Infix("->", selector, value,
                                                       selector->Position());
                            Infix *last = info->last;
                            Tree *previous = last ? last->right : block->child;
                            Infix *cr = new Infix("\n", previous, nameDef,
                                                  nameDef->Position());
                            if (last)
                                last->right = cr;
                            else
                                block->child = cr;
                            info->last = cr;
                        }
                    }
                }
                else // Does not exist in current scope
                {
                    // First time we see this block here, just define it

                    // Extract selector and build name definition
                    Infix *nameDef = new Infix("->", selector, value,
                                               selector->Position());
                    Block *block = new Block(nameDef,
                                             Block::indent, Block::unindent,
                                             defined->Position());
                    block->SetInfo<PDI>(new PDI());
                    Define(defined, block);
                }
            } // Valid name (not operator)
        } // Not a definition of a prefix
    }

    return rewrite;
}


Rewrite *Context::DefineData(Tree *data)
// ----------------------------------------------------------------------------
//   Enter a data declaration
// ----------------------------------------------------------------------------
{
    return Define(data, NULL);
}


// Macro to lookup over all contexts
#define FOR_CONTEXTS(start, context)                                    \
{                                                                       \
    context_set  set;                                                   \
    context_list list;                                                  \
    context_list::iterator iter;                                        \
    if (lookup & IMPORTED_LOOKUP)                                       \
    {                                                                   \
        Contexts(lookup, set, list);                                    \
        iter = list.begin();                                            \
    }                                                                   \
    Context *nextContext = NULL;                                        \
    for (Context *context = start; context; context = nextContext)      \
    {

#define END_FOR_CONTEXTS                                        \
        if (lookup & IMPORTED_LOOKUP)                           \
            nextContext = iter == list.end() ? NULL : *iter++;  \
        else if (lookup & SCOPE_LOOKUP)                         \
            nextContext = context->scope;                       \
        else if (lookup & STACK_LOOKUP)                         \
            nextContext = context->stack;                       \
    }                                                           \
}
        


Tree *Context::Assign(Tree *target, Tree *source, lookup_mode lookup)
// ----------------------------------------------------------------------------
//   Perform an assignment in the given context
// ----------------------------------------------------------------------------
{
    Tree *value = Evaluate(source);

    // If the target is not a name, evaluate it
    if (target->Kind() != NAME)
    {
        target = Evaluate(target);
        if (target->Kind() != NAME)
            Ooops("Assignment target $1 is not a name", target);
    }

    if (Name *name = target->AsName())
    {
        // Build the hash key for the tree to evaluate
        ulong key = Hash(name);

        // Loop over all contexts, searching for a pre-existing assignment
        FOR_CONTEXTS(this, context)
        {
            rewrite_table &rwt = context->rewrites;
            rewrite_table::iterator found = rwt.find(key);
            if (found != rwt.end())
            {
                Rewrite *candidate = (*found).second;
                while (candidate)
                {
                    if (Name *from = candidate->from->AsName())
                    {
                        if (name->value == from->value)
                        {
                            if (candidate->native == xl_assigned_value)
                            {
                                // This was an assigned value, replace it
                                candidate->to = value;
                                return value;
                            }

                            // Can't assign if this already existed
                            Ooops("Assigning to $1", name);
                            Ooops("previously defined as $1", from);
                            return value;
                        }
                    }
                    
                    rewrite_table &rwh = candidate->hash;
                    found = rwh.find(key);
                    candidate = found == rwh.end() ? NULL : (*found).second;
                }
            } 
        }
        END_FOR_CONTEXTS;

        // Check that we have only names in the pattern
        ValidateNames(name);

        // Create a rewrite
        Rewrite *rewrite = new Rewrite(name, value);

        // Walk through existing rewrites (we already checked for redefinitions)
        Rewrite_p *parent = &rewrites[key]; 
        while (Rewrite *where = *parent)
            parent = &where->hash[key];

        // Insert the rewrite
        *parent = rewrite;

        // Mark the rewrite as an assignment
        rewrite->native = xl_assigned_value;
    }
    return value;
}


Tree *Context::Evaluate(Tree *what, lookup_mode lookup)
// ----------------------------------------------------------------------------
//   Evaluate 'what' in the given context
// ----------------------------------------------------------------------------
{
    // Process declarations and evaluate the rest
    Tree    *result = what;
    Tree    *instrs = ProcessDeclarations(what);
    Context *eval   = this;
    if (instrs)
    {
        Tree *next = instrs;
        while (next)
        {
            Infix *seq = next->AsInfix();
            if (seq && (seq->name == "\n" || seq->name == ";"))
            {
                what = seq->left;
                next = seq->right;

                tree_map empty;
                result = eval->Evaluate(what, empty, lookup);
            }
            else
            {
                what = next;
                next = NULL;

                // Opportunity for tail recursion
                tree_map empty;
                Tree    *tail   = NULL;
                Context *old = eval;
                result = eval->Evaluate(what, empty, lookup, &eval, &tail);
                if (tail)
                {
                    next = tail;

                    // The following allows us to benefit from tail opt in
                    // the common case where the called function is a block.
                    // It is safe because we got a new evaluation context
                    if (Block *block = tail->AsBlock())
                    {
                        if (block->IsIndent() ||
                            block->IsParentheses() ||
                            block->IsBraces())
                        {
                            if (eval == old)
                                eval = new Context(eval, eval);
                            next = eval->ProcessDeclarations(block->child);
                        }
                    }
                }
            }
        }
    }
    return result;
}


Tree *Context::Evaluate(Tree *what,             // Value to evaluate
                        tree_map &values,       // Cache of values
                        lookup_mode lookup,     // Lookup mode
                        Context **tailContext,  // Optional tail recursion ctxt
                        Tree **tailTree)        // Optional tail recursion next
// ----------------------------------------------------------------------------
//   Check if something is in the cache, otherwise evaluate it
// ----------------------------------------------------------------------------
{
    // Quick optimization for constants
    if (!hasConstants && what->IsConstant())
        return what;

    // Check if we already evaluated this
    tree_map::iterator cached = values.find(what);
    if (cached != values.end())
        return (*cached).second;

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
    FOR_CONTEXTS(this, context)
    {
        rewrite_table &rwt = context->rewrites;
        rewrite_table::iterator found = rwt.find(key);
        if (found == rwt.end())
        {
            // Not found with exact key, check with kind alone
            found = rwt.find(key & 0xF);
            if (found == rwt.end())
                found = rwt.find(0);
        }
        if (found != rwt.end())
        {
            Rewrite *candidate = (*found).second;
            while (candidate)
            {
                ulong formKey = HashForm(candidate->from);
                if (formKey == key)
                {
                    IFTRACE(eval)
                        std::cerr << "Tree " << ShortTreeForm(what)
                                  << " candidate in " << context
                                  << " is " << ShortTreeForm(candidate->from)
                                  << "\n";

                    // If this is native C code, invoke it.
                    if (native_fn fn = candidate->native)
                    {
                        // Case where we have an assigned value
                        if (fn == xl_assigned_value)
                            return candidate->to;

                        // Bind native context
                        TreeList args;
                        Context *eval = new Context(context, this);
                        if (eval->Bind(candidate->from, what, values, &args))
                        {
                            uint arity = args.size();
                            Compiler *comp = MAIN->compiler;
                            adapter_fn adj = comp->ArrayToArgsAdapter(arity);
                            Tree **args0 = (Tree **) &args[0];
                            Tree *result = adj(fn, eval, what, args0);
                            values[what] = result;
                            return result;
                        }
                    }
                    else if (Name *name = candidate->from->AsName())
                    {
                        // A name always evaluates in the present context,
                        // not context of origin (we'd have a closure)
                        Name *vname = what->AsName();
                        assert(vname || !"Hash function is broken");
                        if (name->value == vname->value)
                        {
                            Tree *result = candidate->to;
                            if (result && result != candidate->from)
                            {
                                if (tailContext)
                                {
                                    *tailContext = this;
                                    *tailTree = result;
                                    return result;
                                }
                                else
                                {
                                    result = Evaluate(result, lookup);
                                }
                            }
                            values[what] = result;
                            return result;
                        }
                    }
                    else
                    {
                        // Keep evaluating
                        Context *eval = new Context(context, this);
                        if (eval->Bind(candidate->from, what, values))
                        {
                            Tree *result = candidate->from;
                            if (Tree *to = candidate->to)
                            {
                                if (tailContext)
                                {
                                    *tailContext = eval;
                                    *tailTree = to;
                                    return to;
                                }
                                else
                                {
                                    result = eval->Evaluate(to,lookup);
                                }
                            }
                            else
                            {
                                result = xl_evaluate_children(eval, result);
                            }
                            values[what] = result;
                            return result;
                        }
                    }
                }

                rewrite_table &rwh = candidate->hash;
                found = rwh.find(key);
                candidate = found == rwh.end() ? NULL : (*found).second;
            } // Loop on candidates
        } // If found candidate
    }
    END_FOR_CONTEXTS;

    // Error case - Raise an error
    static bool inError = false;
    if (lookup & AVOID_ERRORS)
    {
        Ooops("Bind failed to evaluate $1", what);
    }
    else if (inError)
    {
        Ooops("An error happened while processing error $1", what);
    }
    else
    {
        static Name_p evaluationError = new Name("evaluation_error");
        LocalSave<bool> saveInError(inError, true);
        Prefix *errorForm = new Prefix(evaluationError, what, what->Position());
        what = Evaluate(errorForm);
    }
    return what;
}


Tree *Context::EvaluateBlock(Tree *what)
// ----------------------------------------------------------------------------
//   Create a new inner scope for evaluating the value
// ----------------------------------------------------------------------------
{
    Context *block = new Context(this, this);
    what = block->Evaluate(what);
    return what;
}


ulong Context::HashForm(Tree *form)
// ----------------------------------------------------------------------------
//   Eliminate the "when" clauses from the form
// ----------------------------------------------------------------------------
{
    // Check if we have a guard
    while (Infix *infix = form->AsInfix())
        if (infix->name == "when")
            form = infix->left;
        else
            break;
    return Hash(form);
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
        if (t == ":")
            t = "";
        break;
    case PREFIX:
        if (Name *name = ((Prefix *) what)->left->AsName())
            h = Hash(name);
        break;
    case POSTFIX:
        if (Name *name = ((Postfix *) what)->right->AsName())
            h = Hash(name);
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


bool Context::Bind(Tree *form, Tree *value, tree_map &cache, TreeList *args)
// ----------------------------------------------------------------------------
//   Test if we can match arguments to values
// ----------------------------------------------------------------------------
{
    kind k = form->Kind();
    Context *eval = stack;      // Evaluate in caller's stack
    Errors errors;

    switch(k)
    {
    case INTEGER:
    {
        Integer *f = (Integer *) form;
        value = eval->Evaluate(value, cache, BIND_LOOKUP);
        if (errors.Swallowed())
            return false;
        if (Integer *iv = value->AsInteger())
            return iv->value == f->value;
        return false;
    }
    case REAL:
    {
        Real *f = (Real *) form;
        value = eval->Evaluate(value, cache, BIND_LOOKUP);
        if (errors.Swallowed())
            return false;
        if (Real *rv = value->AsReal())
            return rv->value == f->value;
        return false;
    }
    case TEXT:
    {
        Text *f = (Text *) form;
        value = eval->Evaluate(value, cache, BIND_LOOKUP);
        if (errors.Swallowed())
            return false;
        if (Text *tv = value->AsText())
            return (tv->value == f->value &&
                    tv->opening == f->opening &&
                    tv->closing == f->closing);
        return false;
    }


    case NAME:
    {
        Name *f = (Name *) form;

        // Test if the name is already bound, and if so, if trees match
        if (Tree *bound = Bound(f, SCOPE_LOOKUP))
        {
            if (bound == form)
                return true;
            if (EqualTrees(bound, value))
                return true;
            value = eval->Evaluate(value, cache, BIND_LOOKUP);
            bound = eval->Evaluate(bound, cache, BIND_LOOKUP);
            if (errors.Swallowed())
                return false;
            return EqualTrees(bound, value);
        }

        // Define the name in the given context (same as if it was 'lazy')
        value = eval->CreateLazy(value);
        if (args)
            args->push_back(value);
        else
            Define(form, value);
        return true;
    }

    case INFIX:
    {
        Infix *fi = (Infix *) form;

        // Check type declarations
        if (fi->name == ":")
        {
            if (Name *name = fi->left->AsName())
            {
                if (Name *typeName = fi->right->AsName())
                {
                    if (name->value == typeName->value)
                    {
                        // Got x:x in the interface: accept only identical name
                        if (value->Kind() != NAME)
                        {
                            value = eval->Evaluate(value, cache, BIND_LOOKUP);
                            if (errors.Swallowed())
                                return false;
                        }
                        if (Name *nval = value->AsName())
                        {
                            if (nval->value == name->value)
                            {
                                if (args)
                                {
                                    args->push_back(value);
                                }
                                else
                                {
                                    Rewrite *rw = Define(name, name);
                                    rw->native = xl_named_value;
                                }
                                return true;
                            }
                        }
                        // Name or form mismatch, don't accept it
                        return false;
                    }
                }

                // This is always an error if the name is bound in this scope
                if (Tree *existing = Bound(name, LOCAL_LOOKUP))
                {
                    Ooops("Name $1 was already defined", name);
                    Ooops("with value $1", existing);
                    return false;
                }

                // Evaluate the type
                Tree *type = fi->right;
                type = eval->Evaluate(type, cache, BIND_LOOKUP);
                if (errors.Swallowed())
                    return false;

                // Evaluate the value and match its type if the type is not tree
                if (type == source_type)
                {
                    // No evaluation at all
                    type = tree_type;
                }
                else if (type == tree_type)
                {
                    // Evaluate to a tree if we have a name
                    if (Name *name = value->AsName())
                        if (Tree *bound = eval->Bound(name))
                            value = bound;
                    type = tree_type;
                }
                else if (type == code_type)
                {
                    // We want a code closure
                    value = eval->CreateCode(value);
                    type = tree_type;
                }
                else if (type == lazy_type)
                {
                    value = eval->CreateLazy(value);
                    type = tree_type;
                }
                else
                {
                    value = eval->Evaluate(value, cache, BIND_LOOKUP);
                    if (errors.Swallowed())
                        return false;
                    if (type != value_type)
                        value = ValueMatchesType(this, type, value, true);
                    if (!value)
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
        else if (fi->name == "when")
        {
            // We have a guard - first test if we can bind the left part
            if (!Bind(fi->left, value, cache, args))
                return false;

            // Now try to test the guard value
            Tree *guard = Evaluate(fi->right, cache, BIND_LOOKUP);
            if (errors.Swallowed())
                return false;
            return guard == xl_true;
        }

        // If we match the infix name, we can bind left and right
        if (Infix *infix = value->AsInfix())
            if (fi->name == infix->name)
                return (Bind(fi->left,  infix->left,  cache, args) &&
                        Bind(fi->right, infix->right, cache, args));

        // If direct binding failed, evaluate and try again ("diamond")
        value = eval->Evaluate(value, cache, BIND_LOOKUP);
        if (errors.Swallowed())
            return false;
        if (Infix *infix = value->AsInfix())
            if (fi->name == infix->name)
                return (Bind(fi->left,  infix->left,  cache, args) &&
                        Bind(fi->right, infix->right, cache, args));

        // Otherwise, we don't have a match
        return false;
    }

    case PREFIX:
    {
        Prefix *pf = (Prefix *) form;

        // If the left side is a name, make sure it's an exact match
        if (Prefix *prefix = value->AsPrefix())
        {
            if (Name *name = pf->left->AsName())
            {
                Tree *vname = prefix->left;
                if (vname->Kind() != NAME)
                {
                    vname = eval->Evaluate(vname, cache, BIND_LOOKUP);
                    if (errors.Swallowed())
                        return false;
                }
                if (Name *vn = vname->AsName())
                    if (name->value != vn->value)
                        return false;
            }
            else
            {
                if (!Bind(pf->left, prefix->left, cache, args))
                    return false;
            }
            return Bind(pf->right, prefix->right, cache, args);
        }
        return false;
    }

    case POSTFIX:
    {
        Postfix *pf = (Postfix *) form;

        // If the right side is a name, make sure it's an exact match
        if (Postfix *postfix = value->AsPostfix())
        {
            if (Name *name = pf->right->AsName())
            {
                Tree *vname = postfix->right;
                if (vname->Kind() != NAME)
                {
                    vname = eval->Evaluate(vname, cache, BIND_LOOKUP);
                    if (errors.Swallowed())
                        return false;
                }
                if (Name *vn = vname->AsName())
                    if (name->value != vn->value)
                        return false;
            }
            else
            {
                if (!Bind(pf->right, postfix->right, cache, args))
                    return false;
            }
            return Bind(pf->left, postfix->left, cache, args);
        }
        return false;
    }

    case BLOCK:
    {
        Block *block = (Block *) form;
        if (Block *bval = value->AsBlock())
            if (bval->opening == block->opening &&
                bval->closing == block->closing)
                return Bind(block->child, bval->child, cache, args);
        return Bind(block->child, value, cache, args);
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
    FOR_CONTEXTS(this, context)
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
    }
    END_FOR_CONTEXTS;

    // Not bound
    return NULL;
}


Tree *Context::CreateCode(Tree *value)
// ----------------------------------------------------------------------------
//   Create a closure to record the current context to be evaluted once
// ----------------------------------------------------------------------------
{
    // Quick optimization for constants
    if (!hasConstants && value->IsConstant())
        return value;
    static Name_p closureName = new Name("<code>");

    if (Name *name = value->AsName())
        if (Tree *existing = Bound(name))
            value = existing;
    if (Prefix *prefix = value->AsPrefix())
        if (Name *name = prefix->left->AsName())
            if (name->value == closureName->value)
                return value;

    Prefix *result = new Prefix(closureName, value);
    result->Set<ClosureInfo>(this);
    return result;
}


Tree *Context::EvaluateCode(Tree *closure, Tree *value)
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

    // Return the value
    return result;
}


Tree *Context::CreateLazy(Tree *value)
// ----------------------------------------------------------------------------
//   Create a closure to record the current context to be evaluted once
// ----------------------------------------------------------------------------
{
    // Quick optimization for constants
    if (!hasConstants && value->IsConstant())
        return value;
    static Name_p closureName = new Name("<lazy>");

    if (Name *name = value->AsName())
        if (Tree *existing = Bound(name))
            value = existing;
    if (Prefix *prefix = value->AsPrefix())
        if (Name *name = prefix->left->AsName())
            if (name->value == closureName->value)
                return value;

    Prefix *result = new Prefix(closureName, value);
    result->Set<ClosureInfo>(this);
    return result;
}


Tree *Context::EvaluateLazy(Tree *closure, Tree *value)
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
    static Name_p evaluatedName = new Name("<value>");
    Prefix *prefix = closure->AsPrefix(); assert(prefix || !"Invalid closure");
    prefix->left = evaluatedName;
    prefix->right = result;

    return result;
}


static void ListNameRewrites(rewrite_table &table,
                             text prefix,
                             rewrite_list &list)
// ----------------------------------------------------------------------------
//    List all the names matching in the given rewrite table
// ----------------------------------------------------------------------------
{
    rewrite_table::iterator i;
    for (i = table.begin(); i != table.end(); i++)
    {
        Rewrite *rw = (*i).second;
        Tree *from = rw->from;
        if (Name *name = from->AsName())
        {
            if (name->value.find(prefix) == 0)
            {
                list.push_back(rw);
                ListNameRewrites(rw->hash, prefix, list);
            }
        }
    }
}


void Context::ListNames(text prefix, rewrite_list &list, lookup_mode lookup)
// ----------------------------------------------------------------------------
//   List all names that begin with the given text
// ----------------------------------------------------------------------------
{
    Context *next = NULL;
    for (Context *context = this; context; context = next)
    {
        // List names in the given context
        ListNameRewrites(context->rewrites, prefix, list);

        // Select which scope to use next
        if (lookup & SCOPE_LOOKUP)
            next = context->scope;
        else if (lookup & STACK_LOOKUP)
            next = context->stack;
    }
}


void Context::Contexts(lookup_mode lookup, context_set &set,context_list &list)
// ----------------------------------------------------------------------------
//   List all the contexts we need to lookup for a given lookup mode
// ----------------------------------------------------------------------------
{
    // Check if this is already a known context
    if (set.count(this))
        return;

    // Insert self in the set and in the ordered list
    set.insert(this);
    list.push_back(this);

    if (scope && (lookup & SCOPE_LOOKUP))
        scope->Contexts(lookup, set, list);
    if (stack && (lookup & STACK_LOOKUP))
        stack->Contexts(lookup, set, list);
    if (lookup & IMPORTED_LOOKUP)
        for (context_set::iterator i = imported.begin(); i!=imported.end(); i++)
            (*i)->Contexts(lookup, set, list);
}


void Context::Clear()
// ----------------------------------------------------------------------------
//   Clear the symbol table
// ----------------------------------------------------------------------------
{
    rewrites.clear();
    imported.clear();
}

XL_END


extern "C" void debugrw(XL::Rewrite *r)
// ----------------------------------------------------------------------------
//   For the debugger, dump a rewrite
// ----------------------------------------------------------------------------
{
    if (r)
    {
        if (r->native == XL::xl_assigned_value)
            std::cerr << r->from << " := " << r->to << "\n";
        else
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
    while (c && c != XL::MAIN->context)
    {
        debugs(c);
        c = c->scope;
    }
    if (c == XL::MAIN->context)
        std::cerr << "(MAIN CONTEXT: " << c << ")\n";
    else
        std::cerr << "(FINISHED AT NON-MAIN CONTEXT " << c << ")\n";
}


extern "C" void debugst(XL::Context *c)
// ----------------------------------------------------------------------------
//   For the debugger, dump a symbol table along the stack
// ----------------------------------------------------------------------------
{
    using namespace XL;
    while (c && c != XL::MAIN->context)
    {
        debugs(c);
        c = c->stack;
    }
}
