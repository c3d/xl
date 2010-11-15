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


Rewrite *Context::Define(Tree *form, Tree *value, Tree *type)
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
    Rewrite *rewrite = new Rewrite(form, value, type);
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
    Context_p nextContext = NULL;                                       \
    for (Context_p context = start; context; context = nextContext)     \
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
        


Tree *Context::Assign(Tree *tgt, Tree *src, lookup_mode lookup)
// ----------------------------------------------------------------------------
//   Perform an assignment in the given context
// ----------------------------------------------------------------------------
{
    Tree_p target = tgt;
    Tree_p source = src;
    Tree_p value = Evaluate(src);

    // Check if we have a typed assignment
    Tree_p type = NULL;
    if (Infix *infix = target->AsInfix())
    {
        if (infix->name == ":")
        {
            if (Name *tname = infix->left->AsName())
            {
                type = Evaluate(infix->right);
                target = tname;
            }
        }
    }

    // If the target is not a name, evaluate it
    if (target->Kind() != NAME)
    {
        target = Evaluate(target);
        if (target->Kind() != NAME)
            Ooops("Assignment target $1 is not a name", target);
    }
    return AssignTree(target, value, type, lookup);
}


Tree *Context::AssignTree(Tree *tgt, Tree *val, Tree *tp,
                          lookup_mode lookup)
// ----------------------------------------------------------------------------
//   Perform an assignment in the given context
// ----------------------------------------------------------------------------
{
    Tree_p target = tgt;
    Tree_p value  = val;
    Tree_p type   = tp;
    if (Name *name = tgt->AsName())
    {
        // Check that we have only "real" names assigned to
        ValidateNames(name);

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
                    // Check if this is a redefinition for the given name
                    Name *from = candidate->from->AsName();
                    if (from && from->value == name->value)
                    {
                        Tree *ctype = candidate->type;

                        // Check if this is a value that was assigned to
                        if (candidate->native == xl_assigned_value)
                        {
                            // This was an assigned value, replace it
                            if (type)
                            {
                                Ooops("Variable $1 already exists", name);
                                Ooops("Declared as $1", from);
                            }
                            if (!ctype ||
                                ValueMatchesType(this, ctype, value, true))
                            {
                                // Type compatibility checked: assign value
                                candidate->to = value;
                            }
                            else
                            {
                                Ooops("Value $1 is not compatible", value);
                                Ooops("with type $2 of $1", from, ctype);
                            }
                            return value;
                        }

                        // Otherwise, check if this is a bound name
                        if (ctype == name_type)
                        {
                            if (Name *tname = candidate->to->AsName())
                            {
                                Context *st = context->stack;
                                return st->AssignTree(tname,value,type,lookup);
                            }
                        }
                        
                        // Can't assign if this already existed
                        Ooops("Assigning to $1", name);
                        Ooops("previously defined as $1", from);
                        return value;
                    }
                    
                    rewrite_table &rwh = candidate->hash;
                    found = rwh.find(key);
                    candidate = found == rwh.end() ? NULL : (*found).second;
                }
            }

            // If a type was specified, create variable in inner context
            if (type)
                break;
        }
        END_FOR_CONTEXTS;

        if (type && !ValueMatchesType(this, type, value, true))
        {
            Ooops("Value $1 is not compatible", value);
            Ooops("with declared type $1", type);
        }

        // Create a rewrite
        Rewrite *rewrite = new Rewrite(name, value, type);

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
    Tree_p   result = what;
    Tree_p   instrs = ProcessDeclarations(what);
    Context_p eval   = this;
    if (instrs)
    {
        Tree_p next = instrs;
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
                Tree_p   tail   = NULL;
                Context *old = eval;
                result = eval->Evaluate(what, empty, lookup, &eval, &tail);
                if (tail)
                {
                    next = tail;

                    // If we have some kind of closure, evaluate directly
                    if (Prefix *prefix = next->AsPrefix())
                    {
                        if (Context *context = next->Get<ClosureInfo>())
                        {
                            next = prefix->right;
                            eval = context;
                        }
                    }

                    // The following allows us to benefit from tail opt in
                    // the common case where the called function is a block.
                    // It is safe because we got a new evaluation context
                    if (Block *block = next->AsBlock())
                    {
                        if (block->IsGroup())
                        {
                            if (eval == old)
                                eval = new Context(eval, eval);
                            next = eval->ProcessDeclarations(block->child);
                        }
                    }

                    // If we are here, pop execution stack back at this level
                    eval->stack = this;
                    
                } // if(tail)
            }

            // Check if we had an error. If so, abort right now
            if (MAIN->HadErrors())
                return result;
        }
    }

    GarbageCollector::Collect();

    return result;
}


Tree *Context::Evaluate(Tree *what,             // Value to evaluate
                        tree_map &values,       // Cache of values
                        lookup_mode lookup,     // Lookup mode
                        Context_p *tailContext,  // Optional tail recursion ctxt
                        Tree_p *tailTree)        // Optional tail recursion next
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

    // Normalize arguments
    if (Prefix *prefix = what->AsPrefix())
        NormalizeArguments(",", &prefix->right);

    // Build the hash key for the tree to evaluate
    ulong key = Hash(what);

    // Loop over all contexts
    Tree_p saveWhatFromGC = what;
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
                        Context_p eval = new Context(context, this);
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
                            Tree*result = candidate->to;
                            if (result && result != candidate->from)
                            {
                                // In general, we evaluate names in the current
                                // context, except when looking at name aliases
                                Context_p eval =
                                    candidate->type == name_type
                                    ? (Context *) context->stack
                                    : this;

                                if (tailContext)
                                {
                                    *tailContext = eval;
                                    *tailTree = result;
                                    return result;
                                }
                                else
                                {
                                    result = eval->Evaluate(result, lookup);
                                }
                            }
                            values[what] = result;
                            return result;
                        }
                    }
                    else
                    {
                        // Keep evaluating
                        Context_p eval = new Context(context, this);
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

    // If we are unable to find the right form, check standard prefix forms
    if (Prefix *prefix = what->AsPrefix())
    {
        // Check what we invoke and its argument
        Tree *invoked = prefix->left;
        Tree *arg = prefix->right;

        // If we have a block, look at what's inside
        if (Block *block = invoked->AsBlock())
            invoked = block->child;

        // First scenario: a prefix with a bound name (bug #458)
        if (Name *name = invoked->AsName())
        {
            Context_p where = NULL;
            if (Tree *existing = Bound(name, SCOPE_LOOKUP, &where))
            {
                if (existing != name)
                {
                    // Try again with the bound form
                    Errors errors;
                    arg = CreateLazy(arg);
                    Prefix_p bpfx = new Prefix(prefix, existing, arg);
                    Tree *result = Evaluate(bpfx, values, lookup,
                                            tailContext,tailTree);

                    // If we had error, keep the original message (clearer)
                    if (!errors.Swallowed())
                        return result;
                }
            }
        }

        // Second scenario: universal rewrite rules and "when" clauses
        if (Infix *infix = invoked->AsInfix())
        {
            if (infix->name == "->")
            {
                if (Name *defined = infix->left->AsName())
                {
                    Tree_p body = infix->right;
                    Context_p eval = new Context(this, this);
                    eval->Define(defined, arg);
                    Tree_p result = eval->Evaluate(body);
                    return result;
                }
            }
        }
    }

    // Error case - Raise an error
    static bool inError = false;
    if (lookup & AVOID_ERRORS)
    {
        Ooops("Bind failed to evaluate $1", what);
        what = NULL;
    }
    else if (inError)
    {
        Ooops("An error happened while processing error $1", what);
        what = NULL;
    }
    else
    {
        static Name_p evaluationError = new Name("evaluation_error");
        LocalSave<bool> saveInError(inError, true);
        tree_map emptyCache;
        Prefix_p errorForm = new Prefix(evaluationError,what,what->Position());
        what = Evaluate(errorForm, emptyCache);
    }
    return what;
}


Tree *Context::EvaluateBlock(Tree *what)
// ----------------------------------------------------------------------------
//   Create a new inner scope for evaluating the value
// ----------------------------------------------------------------------------
{
    Tree_p result = what;
    Context_p block = new Context(this, this);
    result = block->Evaluate(result);
    return result;
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


bool Context::Bind(Tree *formTree, Tree *valueTree,
                   tree_map &cache, TreeList *args)
// ----------------------------------------------------------------------------
//   Test if we can match arguments to values
// ----------------------------------------------------------------------------
{
    Tree_p form = formTree;
    Tree_p value = valueTree;
    kind k = formTree->Kind();
    Context_p eval = stack; // Evaluate in caller's stack
    Errors errors;

    switch(k)
    {
    case INTEGER:
    {
        Integer *f = (Integer *) formTree;
        value = eval->Evaluate(value, cache, BIND_LOOKUP);
        if (errors.Swallowed())
            return false;
        if (Integer *iv = value->AsInteger())
            return iv->value == f->value;
        return false;
    }
    case REAL:
    {
        Real *f = (Real *) formTree;
        value = eval->Evaluate(value, cache, BIND_LOOKUP);
        if (errors.Swallowed())
            return false;
        if (Real *rv = value->AsReal())
            return rv->value == f->value;
        return false;
    }
    case TEXT:
    {
        Text *f = (Text *) formTree;
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
        Name *f = (Name *) formTree;

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
        Infix *fi = (Infix *) formTree;

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
                Tree_p type = fi->right;
                type = eval->Evaluate(type, cache, BIND_LOOKUP);
                if (errors.Swallowed())
                    return false;

                // Evaluate the value and match its type if the type is not tree
                if (type == source_type)
                {
                    // No evaluation at all, pass data as is.
                    type = tree_type;
                }
                else if (type == block_type  || type == infix_type ||
                         type == prefix_type || type == postfix_type)
                {
                    // No evaluation, but type check
                    value = ValueMatchesType(this, type, value, true);
                    if (!value)
                        return false;
                }
                else if (type == symbol_type ||
                         type == operator_type || type == name_type)
                {
                    // Return bound name if there is one
                    if (Name *name = value->AsName())
                        if (Tree *bound = eval->Bound(name))
                            if (Name *bname = bound->AsName())
                                value = bname;

                    value = ValueMatchesType(this, type, value, true);
                    if (!value)
                        return false;
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
                else if (type == value_type)
                {
                    value = eval->Evaluate(value, cache);
                    if (errors.Swallowed())
                        return false;
                }
                else
                {
                    value = eval->Evaluate(value, cache, BIND_LOOKUP);
                    if (errors.Swallowed())
                        return false;
                    value = ValueMatchesType(this, type, value, true);
                    if (!value)
                        return false;
                }

                // Define the name locally, do not create a closure
                if (args)
                    args->push_back(value);
                else
                    Define(name, value, type);
                return true;
            } // We have a name on the left
        } // We have an infix :
        else if (fi->name == "when")
        {
            // We have a guard - first test if we can bind the left part
            if (!Bind(fi->left, value, cache, args))
                return false;

            // Now try to test the guard value
            Tree_p guard = Evaluate(fi->right, cache, BIND_LOOKUP);
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
        bool indirect = true;
        if (value->IsConstant())
            indirect = false;
        if (Block *block = value->AsBlock())
            if (block->IsIndent())
                indirect = false;
        if (indirect)
        {
            value = eval->Evaluate(value, cache, BIND_LOOKUP);
            if (errors.Swallowed())
                return false;
            if (Infix *infix = value->AsInfix())
                if (fi->name == infix->name)
                    return (Bind(fi->left,  infix->left,  cache, args) &&
                            Bind(fi->right, infix->right, cache, args));
        }

        // Otherwise, we don't have a match
        return false;
    }

    case PREFIX:
    {
        Prefix *pf = (Prefix *) formTree;

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
        Postfix *pf = (Postfix *) formTree;

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
        Block *block = (Block *) formTree;
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


Tree_p *Context::NormalizeArguments(text separator, Tree_p *args)
// ----------------------------------------------------------------------------
//   Create a comma-separated argument list in the right order
// ----------------------------------------------------------------------------
{
    if (Infix *infix = (*args)->AsInfix())
    {
        if (infix->name == separator)
        {
            Tree_p *last = NormalizeArguments(separator, &infix->left);

            // Turn '(A,B),C' into 'A,(B,C)'
            if (last != &infix->left)
            {
                *last = new Infix(infix, *last, infix->right);
                last = NormalizeArguments(separator, last);
                if (args != last)
                    *args = infix->left;
                return last;
            }

            last = NormalizeArguments(separator, &infix->right);
            return last;
        }
    }

    if (Block *block = (*args)->AsBlock())
    {
        if (block->IsParentheses())
        {
            if (Infix *infix = block->child->AsInfix())
            {
                if (infix->name == separator)
                {
                    Tree_p *value = &block->child;
                    Tree_p *last = NormalizeArguments(separator, value);
                    *args = *value;
                    return last;
                }
            }
        }
    }

    return args;
}


Tree *Context::Bound(Name *name, lookup_mode lookup, Context_p *where)
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
                {
                    if (name->value == from->value)
                    {
                        if (where)
                            *where = context;
                        if (Tree *to = candidate->to)
                                return to;
                        else
                            return from;
                    }
                }

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
    if (Name *name = value->AsName())
        if (Tree *existing = Bound(name))
            value = existing;

    // Quick optimization for constants
    if (!hasConstants && value->IsConstant())
        return value;
    if (ClosureValue(value))
        return value;

    static Name_p closureName = new Name("<code>");
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
    if (Name *name = value->AsName())
        if (Tree *existing = Bound(name))
            value = existing;

    // Quick optimization for constants
    if (!hasConstants && value->IsConstant())
        return value;
    if (ClosureValue(value))
        return value;

    static Name_p closureName = new Name("<lazy>");
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


Tree *Context::ClosureValue(Tree *value, Context_p *where)
// ----------------------------------------------------------------------------
//   If a value is a closure, return its value
// ----------------------------------------------------------------------------
{
    if (Prefix *prefix = value->AsPrefix())
    {
        if (Name *name = prefix->left->AsName())
        {
            text n = name->value;
            if (n == "<code>" || n == "<lazy>" || n == "<value>")
            {
                if (Context *ctx = prefix->Get<ClosureInfo>())
                {
                    if (where)
                        *where = ctx;
                    return prefix->right;
                }
            }
        }
    }
    return NULL;
}


static void ListNameRewrites(rewrite_table &table,
                             text prefix,
                             rewrite_list &list,
                             bool prefixesOk)
// ----------------------------------------------------------------------------
//    List all the names matching in the given rewrite table
// ----------------------------------------------------------------------------
{
    rewrite_table::iterator i;
    for (i = table.begin(); i != table.end(); i++)
    {
        Rewrite *rw = (*i).second;
        Tree *from = rw->from;
        Name *name = from->AsName();
        if (!name && prefixesOk)
        {
            if (Prefix *pre = from->AsPrefix())
                name = pre->left->AsName();
        }
        if (name)
        {

            if (name->value.find(prefix) == 0)
            {
                list.push_back(rw);
                ListNameRewrites(rw->hash, prefix, list, prefixesOk);
            }
        }
    }
}


void Context::ListNames(text prefix, rewrite_list &list, lookup_mode lookup,
                        bool prefixesOk)
// ----------------------------------------------------------------------------
//   List all names that begin with the given text
// ----------------------------------------------------------------------------
//   If prefixesOk == true, also return names that are prefixes
{
    Context *next = NULL;
    for (Context *context = this; context; context = next)
    {
        // List names in the given context
        ListNameRewrites(context->rewrites, prefix, list, prefixesOk);

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
