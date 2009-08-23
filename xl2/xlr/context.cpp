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

XL_BEGIN

// ============================================================================
// 
//   Symbols: symbols management
// 
// ============================================================================

void Symbols::EnterName(text name, Tree *value)
// ----------------------------------------------------------------------------
//   Enter a value in the namespace
// ----------------------------------------------------------------------------
{
    names[name] = value;
}


Name *Symbols::Allocate(Name *n)
// ----------------------------------------------------------------------------
//   Enter a value in the namespace
// ----------------------------------------------------------------------------
{
    names[n->value] = n;
    return n;
}


Rewrite *Symbols::EnterRewrite(Rewrite *rw)
// ----------------------------------------------------------------------------
//   Enter the given rewrite in the rewrites table
// ----------------------------------------------------------------------------
{
    if (rewrites)
        return rewrites->Add(rw);
    rewrites = rw;
    return rw;
}


Rewrite *Symbols::EnterRewrite(Tree *from, Tree *to)
// ----------------------------------------------------------------------------
//   Create a rewrite for the current context and enter it
// ----------------------------------------------------------------------------
{
    Rewrite *rewrite = new Rewrite(this, from, to);
    return EnterRewrite(rewrite);
}


void Symbols::Clear()
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
//    Evaluation of trees
// 
// ============================================================================

void Symbols::ParameterList(Tree *form, tree_list &list)
// ----------------------------------------------------------------------------
//    List the parameters for the form and add them to the list
// ----------------------------------------------------------------------------
{
    // Identify all parameters in 'from'
    Symbols parms(this);
    ParameterMatch matchParms(&parms);
    Tree *parmsOK = form->Do(matchParms);
    if (!parmsOK)
    {
        Context *context = Context::context;
        context->Error("Internal: what parameter list in '$1'?", form);
    }

    // Build the parameter list for 'to'
    list = matchParms.order;
}


Tree *Symbols::Compile(Tree *source, CompiledUnit &unit, bool nullIfBad)
// ----------------------------------------------------------------------------
//    Return an optimized version of the source tree, ready to run
// ----------------------------------------------------------------------------
{
    // Record rewrites and data declarations in the current context
    Symbols parms(this);
    DeclarationAction declare(&parms);
    Tree *result = source->Do(declare);

    // Compile code for that tree
    CompileAction compile(&parms, unit, nullIfBad);
    result = source->Do(compile);

    // If we didn't compile successfully, report
    if (!result)
    {
        Context *context = Context::context;
        if (nullIfBad)
            return result;
        return context->Error("Couldn't compile '$1'", source);
    }

    // If we compiled successfully, get the code and store it
    return result;
}


Tree *Symbols::CompileAll(Tree *source)
// ----------------------------------------------------------------------------
//   Compile a top-level tree
// ----------------------------------------------------------------------------
//    This associates an eval_fn to the tree, i.e. code that takes a tree
//    as input and returns a tree as output.
{
    Context *context = Context::context;
    Compiler *compiler = context->compiler;
    tree_list noParms;
    CompiledUnit unit (compiler, source, noParms);
    if (unit.IsForwardCall())
        return source;

    Tree *result = Compile(source, unit, false);
    if (!result)
        return result;

    eval_fn fn = unit.Finalize();
    source->code = fn;
    return source;
}


Tree *Symbols::Run(Tree *code)
// ----------------------------------------------------------------------------
//   Execute a compiled code tree - Very similar to xl_evaluate
// ----------------------------------------------------------------------------
{
    Tree *result = code;
    if (!result)
        return result;

     IFTRACE(eval)
        std::cerr << "RUN: " << code << '\n';

   if (!result->IsConstant())
    {
        if (!result->code)
        {
            Symbols *symbols = result->symbols;
            if (!symbols)
                symbols = this;
            result = symbols->CompileAll(result);
        }

        assert(result->code);
        result = result->code(code);
    }
    return result;
}



// ============================================================================
// 
//   Garbage collection
// 
// ============================================================================
//   This is just a rather simple mark and sweep garbage collector.

ulong Context::gc_increment = 200;
ulong Context::gc_growth_percent = 200;
Context *Context::context = NULL;

Context::~Context()
// ----------------------------------------------------------------------------
//   Delete all globals allocated by that context
// ----------------------------------------------------------------------------
{
    globals_table::iterator g;
    for (g = globals.begin(); g != globals.end(); g++)
        delete *g;
}


Tree **Context::AddGlobal(Tree *value)
// ----------------------------------------------------------------------------
//   Create a global, immutable address for LLVM
// ----------------------------------------------------------------------------
{
    Tree **ptr = new Tree*;
    *ptr = value;
    return ptr;
}


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
        Mark(what);
        return what;
    }
    Tree *DoBlock(Block *what)
    {
        if (Mark(what))
            Action::DoBlock(what);              // Do child
        return what;
    }
    Tree *DoInfix(Infix *what)
    {
        if (Mark(what))
            Action::DoInfix(what);              // Do children
        return what;
    }
    Tree *DoPrefix(Prefix *what)
    {
        if (Mark(what))
            Action::DoPrefix(what);             // Do children
        return what;
    }
    Tree *DoPostfix(Postfix *what)
    {
        if (Mark(what))
            Action::DoPostfix(what);            // Do children
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
        ulong deletedCount = 0, activeCount = 0;

        IFTRACE(memory)
            std::cerr << "Garbage collecting...";

        // Mark roots, names, rewrites and stack
        for (active_set::iterator a = roots.begin(); a != roots.end(); a++)
            (*a)->Do(gc);
        for (symbol_iter y = names.begin(); y != names.end(); y++)
            (*y).second->Do(gc);
        if (rewrites)
            rewrites->Do(gc);

        formats_table::iterator f;
        formats_table &formats = Renderer::renderer->formats;
        for (f = formats.begin(); f != formats.end(); f++)
            (*f).second->Do(gc);

        globals_table::iterator g;
        for (g = globals.begin(); g != globals.end(); g++)
            (**g)->Do(gc);

        // Then delete all trees in active set that are no longer referenced
        for (active_set::iterator a = active.begin(); a != active.end(); a++)
        {
            activeCount++;
            if (!gc.alive.count(*a))
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
                      << " threshold " << gc_threshold << "\n";
    }
}



// ============================================================================
// 
//    Hash key for tree rewrite
// 
// ============================================================================
//    We use this hashing key to quickly determine if two trees "look the same"

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
        key = (key << 3) ^ Hash(4, what->opening + what->closing);
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
//    Parameter match - Isolate parameters in an rewrite source
// 
// ============================================================================

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
        if (Tree *existing = symbols->Named(what->value))
            return existing;

        // If first occurence of the name, enter it in symbol table
        Tree *result = symbols->Allocate(what);
        order.push_back(result);
        return result;
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
        Context *context = Context::context;

        // Check the variable name, e.g. K in example above
        Name *varName = what->left->AsName();
        if (!varName)
            return context->Error("Expected a name, got '$1' ", what->left);

        // Check if the name already exists
        if (Tree *existing = symbols->Named(varName->value))
            return context->Error("Typed name '$1' already exists as '$2'",
                                  what->left, existing);

        // Enter the name in symbol table
        Tree *result = symbols->Allocate(varName);
        order.push_back(result);
        return result;
    }

    // If this is the first one, this is what we define
    if (!defined)
        defined = what;

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
    Infix *defined_infix = defined->AsInfix();
    if (defined_infix)
        defined = NULL;

    Tree *lr = what->left->Do(this);
    if (!lr)
        return NULL;
    Tree *rr = what->right->Do(this);
    if (!rr)
        return NULL;

    if (!defined && defined_infix)
        defined = defined_infix;

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



// ============================================================================
// 
//    Argument matching - Test input parameters
// 
// ============================================================================

Tree *ArgumentMatch::Compile(Tree *source)
// ----------------------------------------------------------------------------
//    Compile the source tree, and record we use the value in expr cache
// ----------------------------------------------------------------------------
{
    // Compile the code
    if (!unit.Known(source))
    {
        source = symbols->Compile(source, unit, true);
        if (!source)
            return NULL; // No match
    }
    else
    {
        // Generate code to evaluate the argument
        LocalSave<bool> nib(compile->nullIfBad, true);
        source = source->Do(compile);
    }

    return source;
}


Tree *ArgumentMatch::CompileValue(Tree *source)
// ----------------------------------------------------------------------------
//   Compile the source and make sure we evaluate it
// ----------------------------------------------------------------------------
{
    Tree *result = Compile(source);

    if (result)
    {
        if (Name *name = result->AsName())
        {
            llvm::BasicBlock *bb = unit.BeginLazy(name);
            unit.NeedStorage(name);
            unit.CallEvaluate(name);
            unit.EndLazy(name, bb);
        }
    }
    return result;
}


Tree *ArgumentMatch::CompileClosure(Tree *source)
// ----------------------------------------------------------------------------
//    Compile the source tree for lazy evaluation, i.e. wrap in code
// ----------------------------------------------------------------------------
{
    // Compile leaves normally
    if (source->IsLeaf())
        return Compile(source);

    // For more complex expression, return a constant tree
    unit.ConstantTree(source);

    // Record which elements of the expression are captured from context
    Context *context = Context::context;
    Compiler *compiler = context->compiler;
    EnvironmentScan env(symbols);
    Tree *envOK = source->Do(env);
    if (!envOK)
        return context->Error("Internal: what environment in '$1'?", source);

    // Create the parameter list with all imported locals
    tree_list parms, args;
    capture_table::iterator c;
    for (c = env.captured.begin(); c != env.captured.end(); c++)
    {
        Tree *name = (*c).first;
        Symbols *where = (*c).second;
        if (where == context)
        {
            // This is a global, we'll find it running the target.
        }
        else
        {
            // This is a local: simply pass it around
            parms.push_back(name);
            args.push_back(name);
        }
    }
    
    // Create the compilation unit and check if we are already compiling this
    // Can this happen for a closure?
    CompiledUnit subUnit(compiler, source, parms);
    if (!subUnit.IsForwardCall())
    {
        Tree *result = symbols->Compile(source, subUnit, false);
        if (!result)
             return result;

        eval_fn fn = subUnit.Finalize();
        source->code = fn;
    }

    // Create a call to xl_new_closure to save the required trees
    unit.CreateClosure(source, args);

    return source;
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
//   An integer argument matches the exact value
// ----------------------------------------------------------------------------
{
    // If the tested tree is a constant, it must be an integer with same value
    if (test->IsConstant())
    {
        if (Integer *it = test->AsInteger())
            if (it->value == what->value)
                return what;
        return NULL;
    }

    // Compile the test tree
    Tree *compiled = CompileValue(test);
    if (!compiled)
        return NULL;

    // Compare at run-time the actual tree value with the test value
    unit.IntegerTest(compiled, what->value);
    return compiled;
}


Tree *ArgumentMatch::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   A real matches the exact value
// ----------------------------------------------------------------------------
{
    // If the tested tree is a constant, it must be an integer with same value
    if (test->IsConstant())
    {
        if (Real *rt = test->AsReal())
            if (rt->value == what->value)
                return what;
        return NULL;
    }

    // Compile the test tree
    Tree *compiled = CompileValue(test);
    if (!compiled)
        return NULL;

    // Compare at run-time the actual tree value with the test value
    unit.RealTest(compiled, what->value);
    return compiled;
}


Tree *ArgumentMatch::DoText(Text *what)
// ----------------------------------------------------------------------------
//   A text matches the exact value
// ----------------------------------------------------------------------------
{
    // If the tested tree is a constant, it must be an integer with same value
    if (test->IsConstant())
    {
        if (Text *tt = test->AsText())
            if (tt->value == what->value)
                return what;
        return NULL;
    }

    // Compile the test tree
    Tree *compiled = CompileValue(test);
    if (!compiled)
        return NULL;

    // Compare at run-time the actual tree value with the test value
    unit.TextTest(compiled, what->value);
    return compiled;
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
        if (Name *nt = test->AsName())
            if (nt->value == what->value)
                return what;
        return NULL;
    }
    else
    {
        // Check if the name already exists, e.g. 'false' or 'A+A'
        // If it does, we generate a run-time check to verify equality
        if (Tree *existing = rewrite->Named(what->value))
        {
            // Insert a dynamic tree comparison test
            Tree *testCode = CompileValue(test);
            if (!testCode)
                return NULL;
            Tree *thisCode = Compile(existing);
            if (!thisCode)
                return NULL;
            unit.ShapeTest(testCode, thisCode);

            // Return compilation success
            return what;
        }

        // If first occurence of the name, enter it in symbol table
        Tree *compiled = CompileClosure(test);
        if (!compiled)
            return NULL;

        locals->EnterName(what->value, compiled);
        return what;
    }
}


Tree *ArgumentMatch::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   Check if we match a block
// ----------------------------------------------------------------------------
{
    // Test if we exactly match the block, i.e. the reference is a block
    if (Block *bt = test->AsBlock())
    {
        if (bt->opening == what->opening &&
            bt->closing == what->closing)
        {
            test = bt->child;
            Tree *br = what->child->Do(this);
            test = bt;
            if (br)
                return br;
        }
    }

    // Otherwise, if the block is an indent or parenthese, optimize away
    if ((what->opening == "(" && what->closing == ")") ||
        (what->opening == Block::indent && what->closing == Block::unindent))
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
    if (Infix *it = test->AsInfix())
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
        Context *context = Context::context;
        Name *varName = what->left->AsName();
        if (!varName)
            return context->Error("Expected a name, got '$1' ", what->left);

        // Check if the name already exists
        if (Tree *existing = symbols->Named(varName->value))
            return context->Error("Name '$1' already exists as '$2'",
                                  what->left, existing);

        // Evaluate type expression, e.g. 'integer' in example above
        Tree *typeExpr = Compile(what->right);
        if (!typeExpr)
            return NULL;

        // Compile what we are testing against
        Tree *compiled = CompileValue(test);
        if (!compiled)
            return NULL;

        // Insert a run-time type test
        unit.TypeTest(compiled, typeExpr);

        // Enter the compiled expression in the symbol table
        locals->EnterName(varName->value, compiled);
        
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
    if (Prefix *pt = test->AsPrefix())
    {
        // Check if we match the tree, e.g. f(A) vs. f(2)
        // Note that we must test left first to define 'f' in above case
        Infix *defined_infix = defined->AsInfix();
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


Tree *ArgumentMatch::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//    For postfix expressions, simply test right, then left
// ----------------------------------------------------------------------------
{
    if (Postfix *pt = test->AsPostfix())
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



// ============================================================================
// 
//    Environment scan - Identify which names are imported from context
// 
// ============================================================================

Tree *EnvironmentScan::Do(Tree *what)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *EnvironmentScan::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *EnvironmentScan::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *EnvironmentScan::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *EnvironmentScan::DoName(Name *what)
// ----------------------------------------------------------------------------
//    Check if name is found in context, if so record where we took it from
// ----------------------------------------------------------------------------
{
    for (Symbols *s = symbols; s; s = s->Parent())
    {
        if (Tree *existing = s->Named(what->value, false))
        {
            // Found the symbol in the given symbol table
            if (!captured.count(existing))
                captured[existing] = s;
            break;
        }
    }
    return what;
}


Tree *EnvironmentScan::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   Parameters in a block are in its child
// ----------------------------------------------------------------------------
{
    return what->child->Do(this);
}


Tree *EnvironmentScan::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Check if we match an infix operator
// ----------------------------------------------------------------------------
{
    // Test left and right
    what->left->Do(this);
    what->right->Do(this);
    return what;
}


Tree *EnvironmentScan::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   For prefix expressions, simply test left then right
// ----------------------------------------------------------------------------
{
    what->left->Do(this);
    what->right->Do(this);
    return what;
}


Tree *EnvironmentScan::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//    For postfix expressions, simply test right, then left
// ----------------------------------------------------------------------------
{
    // Order shouldn't really matter here (unlike ParameterMach)
    what->right->Do(this);
    what->left->Do(this);
    return what;
}



// ============================================================================
// 
//   Declaration action - Enter all tree rewrites in the current symbols
// 
// ============================================================================

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
    // Deal with 'data' declarations
    if (Name *name = what->left->AsName())
    {
        if (name->value == "data")
        {
            EnterRewrite(what->right, NULL);
            return what;
        }
    }

    return what;
}


Tree *DeclarationAction::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//    All postfix operations translate into a rewrite
// ----------------------------------------------------------------------------
{
    return what;
}


void DeclarationAction::EnterRewrite(Tree *defined, Tree *definition)
// ----------------------------------------------------------------------------
//   Add a definition in the current context
// ----------------------------------------------------------------------------
{
    Rewrite *rewrite = new Rewrite(symbols, defined, definition);
    symbols->EnterRewrite(rewrite);
}



// ============================================================================
// 
//   Compilation action - Generation of "optimized" native trees
// 
// ============================================================================

CompileAction::CompileAction(Symbols *s, CompiledUnit &u, bool nib)
// ----------------------------------------------------------------------------
//   Constructor
// ----------------------------------------------------------------------------
    : symbols(s), unit(u), needed(), nullIfBad(nib)
{}


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
    unit.ConstantInteger(what);
    return what;
}


Tree *CompileAction::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Reals evaluate directly
// ----------------------------------------------------------------------------
{
    unit.ConstantReal(what);
    return what;
}


Tree *CompileAction::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Text evaluates directly
// ----------------------------------------------------------------------------
{
    unit.ConstantText(what);
    return what;
}


Tree *CompileAction::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Build a unique reference in the context for the entity
// ----------------------------------------------------------------------------
{
    // Normally, the name should have been declared in ParameterMatch
    Context *context = Context::context;
    if (Tree *result = symbols->Named(what->value))
    {
        // Check if there is code we need to call
        Compiler *compiler = context->compiler;
        if (compiler->functions.count(result))
        {
            // Case of "Name -> Foo": Invoke Name
            tree_list noArgs;
            unit.NeedStorage(what);
            unit.Invoke(what, result, noArgs);
            return what;
        }
        else if (unit.value.count(result))
        {
            // Case of "Foo(A,B) -> B" with B: evaluate B lazily
            unit.Copy(result, what, false);
            return result;
        }
        else
        {
            // Return the name itself by default
            unit.ConstantTree(result);
            unit.Copy(result, what);
        }

        return result;
    }
    return context->Error("Name '$1' does not exist", what);
}


Tree *CompileAction::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   Optimize away indent or parenthese blocks, evaluate others
// ----------------------------------------------------------------------------
{
    if ((what->opening == Block::indent && what->closing == Block::unindent) ||
        (what->opening == "(" && what->closing == ")"))
    {
        if (unit.Known(what))
            unit.Copy(what, what->child, false);
        Tree *result = what->child->Do(this);
        if (!result)
            return NULL;
        unit.Copy(result, what);
        return what;
    }
    
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
        if (!what->left->Do(this))
            return NULL;
        if (unit.Known(what->left))
            unit.CallEvaluate(what->left);
        if (!what->right->Do(this))
            return NULL;
        if (unit.Known(what->right))
            unit.CallEvaluate(what->right);
        unit.Copy(what->right, what);
        return what;
    }

    // Check if this is a rewrite declaration
    if (what->name == "->")
    {
        // If so, skip, this has been done in DeclarationAction
        return what;
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
    bool foundUnconditional = false;
    bool foundSomething = false;
    ExpressionReduction reduction (unit, what);

    Context *context = Context::context;
    for (Symbols *s = symbols; s && !foundUnconditional; s = s->Parent())
    {
        Rewrite *candidate = s->Rewrites();
        while (candidate && !foundUnconditional)
        {
            // Compute the hash key for the 'from' of the current rewrite
            RewriteKey testKeyHash;
            candidate->from->Do(testKeyHash);
            testKey = testKeyHash.Key();

            // If we have an exact match for the keys, we may have a winner
            if (testKey == formKey)
            {
                // If we find a real match, identify its parameters
                Symbols parms(candidate->symbols);
                ParameterMatch matchParms(&parms);
                Tree *parmsTest = candidate->from->Do(matchParms);
                if (!parmsTest)
                    return context->Error(
                        "Internal: Invokation parameters for '$1'?",
                        candidate->from);

                // Create the invokation point
                reduction.NewForm();
                Symbols args(symbols);
                ArgumentMatch matchArgs(what,
                                        symbols, &args, candidate->symbols,
                                        this, needed);
                Tree *argsTest = candidate->from->Do(matchArgs);
                if (argsTest)
                {
                    // Record that we found something
                    foundSomething = true;

                    // If this is a data form, we are done
                    if (!candidate->to)
                    {
                        foundUnconditional = !unit.failbb;
                        reduction.Succeeded();
                    }
                    else
                    {
                        // We should have same number of args and parms
                        ulong parmCount = parms.names.size();
                        if (args.names.size() != parmCount)
                        {
                            symbol_iter a, p;
                            std::cerr << "Args/parms mismatch:\n";
                            std::cerr << "Parms:\n";
                            for (p = parms.names.begin();
                                 p != parms.names.end();
                                 p++)
                            {
                                text name = (*p).first;
                                Tree *parm = parms.Named(name);
                                std::cerr << "   " << name
                                          << " = " << parm << "\n";
                            }
                            std::cerr << "Args:\n";
                            for (a = args.names.begin();
                                 a != args.names.end();
                                 a++)
                            {
                                text name = (*a).first;
                                Tree *arg = args.Named(name);
                                std::cerr << "   " << name
                                          << " = " << arg << "\n";
                            }
                        }

                        // Map the arguments we found in parameter order
                        // (actually, in reverse order, which is what we want)
                        tree_list argsList;
                        tree_list::iterator p;
                        tree_list &order = matchParms.order;
                        for (p = order.begin(); p != order.end(); p++)
                        {
                            Name *name = (*p)->AsName();
                            Tree *argValue = args.Named(name->value);
                            argsList.push_back(argValue);
                        }

                        // Compile the candidate
                        Tree *code = candidate->Compile();

                        // Invoke the candidate
                        unit.Invoke(what, code, argsList);

                        // If there was no test code, don't keep testing further
                        foundUnconditional = !unit.failbb;
                        
                        // This is the end of a successful invokation
                        reduction.Succeeded();
                    } // if (data form)
                } // Match args
                else
                {
                    // Indicate unsuccessful invokation
                    reduction.Failed();
                }
            } // Match test key

            // Otherwise, check if we have a key match in the hash table,
            // and if so follow it.
            if (!foundUnconditional && candidate->hash.count(formKey) > 0)
                candidate = candidate->hash[formKey];
            else
                candidate = NULL;
        } // while(candidate)
    } // for(namespaces)

    // If we didn't find anything, report it
    if (!foundSomething)
    {
        if (nullIfBad)
            return NULL;
        Context *context = Context::context;
        return context->Error("No rewrite candidate for '$1'", what);
    }
    return what;
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
        Tree *arg0 = new Text (message);
        Tree *info = arg3;
        if (arg2)
            info = info ? new Infix(",", arg2, info) : arg2;
        if (arg1)
            info = info ? new Infix(",", arg1, info) : arg1;
        info = info ? new Infix(",", arg0, info) : arg0;

        Prefix *errorCall = new Prefix(handler, info);
        return context->Run(errorCall);
    }

    // No handler: terminate
    errors.Error(message, arg1, arg2, arg3);
    std::exit(1);
}


Tree *Context::ErrorHandler()
// ----------------------------------------------------------------------------
//    Return the innermost error handler
// ----------------------------------------------------------------------------
{
    return error_handler;
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
//   Compile code for the 'to' form
// ----------------------------------------------------------------------------
//   This is similar to Context::Compile, except that it may generate a
//   function with more parameters, i.e. Tree *f(Tree *, Tree *, ...),
//   where there is one input arg per variable in the 'from' tree
{
    assert (to || !"Rewrite::Compile called for data rewrite?");
    if (to->code)
        return to;

    Context *context = Context::context;
    Compiler *compiler = context->compiler;

    // Identify all parameters in 'from'
    Symbols parms(symbols);
    ParameterMatch matchParms(&parms);
    Tree *parmsOK = from->Do(matchParms);
    if (!parmsOK)
        return context->Error("Internal: what parameters in '$1'?", from);

    // Create the compilation unit and check if we are already compiling this
    CompiledUnit unit(compiler, to, matchParms.order);
    if (unit.IsForwardCall())
    {
        // Recursive compilation of that form
        // REVISIT: Can this happen?
        return to;              // We know how to invoke it anyway
    }

    // Compile the body of the rewrite
    CompileAction compile(&parms, unit, false);
    Tree *result = to->Do(compile);
    if (!result)
        return context->Error("Unable to compile '$1'", to);

    // Even if technically, this is not an 'eval_fn' (it has more args),
    // we still record it to avoid recompiling multiple times
    eval_fn fn = compile.unit.Finalize();
    to->code = fn;

    return to;
}

XL_END


extern "C" void debugs(XL::Symbols *s)
// ----------------------------------------------------------------------------
//   For the debugger, dump a symbol table
// ----------------------------------------------------------------------------
{
    using namespace XL;
    std::cerr << "SYMBOLS AT " << s << "\n";
    symbol_table::iterator i;
    for (i = s->names.begin(); i != s->names.end(); i++)
        std::cerr << (*i).first << ": " << (*i).second << "\n";
}


extern "C" void debugsc(XL::Symbols *s)
// ----------------------------------------------------------------------------
//   For the debugger, dump a symbol table
// ----------------------------------------------------------------------------
{
    using namespace XL;
    if (s)
    {
        debugsc(s->Parent());
        debugs(s);
    }
}
