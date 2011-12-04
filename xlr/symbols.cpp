// ****************************************************************************
//  symbols.cpp                     (C) 1992-2003 Christophe de Dinechin (ddd)
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
#include <sstream>
#include "symbols.h"
#include "save.h"
#include "tree.h"
#include "errors.h"
#include "options.h"
#include "renderer.h"
#include "basics.h"
#include "compiler.h"
#include "runtime.h"
#include "main.h"
#include "types.h"

#include <llvm/Analysis/Verifier.h>
#include <llvm/CallingConv.h>
#include "llvm/Constants.h"
#include "llvm/LLVMContext.h"
#include "llvm/DerivedTypes.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include <llvm/PassManager.h>
#include "llvm/Support/raw_ostream.h"
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/StandardPasses.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Support/raw_ostream.h>


XL_BEGIN

// ============================================================================
//
//   Global variables
//
// ============================================================================

declarator_table Symbols::declarators;



// ============================================================================
//
//   Symbols: symbols management
//
// ============================================================================

static void BuildSymbolsList(Symbols *s,
                             symbols_set &visited,
                             symbols_list &lookups)
// ----------------------------------------------------------------------------
//   Build the list of symbols to visit
// ----------------------------------------------------------------------------
{
    while (s)
    {
        if (!visited.count(s))
        {
            lookups.push_back(s);
            visited.insert(s);

            symbols_set::iterator si;
            for (si = s->imported.begin(); si != s->imported.end(); si++)
                BuildSymbolsList(*si, visited, lookups);
        }
        s = s->parent;
    }
}


Rewrite *Symbols::LookupEntry(text name, bool create)
// ----------------------------------------------------------------------------
//   Find the entry for a given name in all visible scopes
// ----------------------------------------------------------------------------
{
    symbols_set visited;
    symbols_list lookups;

    // Build all the symbol tables that we are going to look into
    BuildSymbolsList(this, visited, lookups);

    symbols_list::iterator li;
    for (li = lookups.begin(); li != lookups.end(); li++)
    {
        Symbols *s = *li;
        if (Rewrite *found = s->Entry(name, false))
            return found;
    }

    // If we didn't find it, create it locally
    if (create)
        return Entry(name, create);

    return NULL;
}


Rewrite *Symbols::LookupEntry(Tree *form, bool create)
// ----------------------------------------------------------------------------
//   Find the entry for a given name in all visible scopes
// ----------------------------------------------------------------------------
{
    symbols_set visited;
    symbols_list lookups;

    // Build all the symbol tables that we are going to look into
    BuildSymbolsList(this, visited, lookups);

    symbols_list::iterator li;
    for (li = lookups.begin(); li != lookups.end(); li++)
    {
        Symbols *s = *li;
        if (Rewrite *found = s->Entry(form, false))
            return found;
    }

    // If we didn't find it, create it locally
    if (create)
        return Entry(form, create);

    return NULL;
}


Tree *Symbols::Named(text name, bool deep)
// ----------------------------------------------------------------------------
//   Find the name in the current context
// ----------------------------------------------------------------------------
{
    Rewrite *found = deep ? LookupEntry(name, false) : Entry(name, false);
    if (found)
        return found->to;
    return NULL;
}


void Symbols::EnterName(text name, Tree *value, Rewrite::Kind kind)
// ----------------------------------------------------------------------------
//   Enter a value in the namespace
// ----------------------------------------------------------------------------
{
    Rewrite *found = Entry(name, true);
    if (found->to)
    {
        Ooops("Name $1 already exists", new Name(name, value->Position()));
        Ooops("Previous value was $1", found->to);
    }
    found->kind = kind;
    found->to = value;
}


void Symbols::ExtendName(text name, Tree *value)
// ----------------------------------------------------------------------------
//   Extend a named value as part of a rewrite
// ----------------------------------------------------------------------------
{
    if (!parent->Named(name))
    {
        Rewrite *found = Entry(name, true); 
        found->kind = Rewrite::FORM;
        if (Tree *entry = found->to)
        {
            if (Block *block = entry->AsBlock())
                block->child = new Infix("\n", block->child, value,
                                         value->Position());
            else
                entry = new Block(new Infix("\n", entry, value,
                                            value->Position()),
                                  Block::indent, Block::unindent,
                                  value->Position());
        }
        else
        {
            found->to = value;
        }
    }

    if (value && !value->Symbols())
        value->SetSymbols(this);
}


Name *Symbols::Allocate(Name *n)
// ----------------------------------------------------------------------------
//   Enter a value in the namespace
// ----------------------------------------------------------------------------
{
    Rewrite *entry = Entry(n->value, true);
    if (Tree *existing = entry->to)
    {
        if (Name *name = existing->AsName())
            if (name->value == n->value)
                return name;
        Ooops("Parameter $1 previously had value $2", n, existing);
    }
    entry->to = n;
    entry->kind = Rewrite::PARM;
    return n;
}


ulong Symbols::Count(ulong mask, Rewrite *rw)
// ----------------------------------------------------------------------------
//   Return the number of local variables
// ----------------------------------------------------------------------------
{
    ulong count = 0;
    if (!rw)
        rw = Rewrites();
    if (rw)
    {
        if (mask & (1<<rw->kind))
            count++;
        for (uint i = 0; i < REWRITE_HASH_SIZE; i++)
            if (Rewrite *child = rw->hash[i])
                count += Count(mask, child);
    }
    return count;
}


Rewrite *Symbols::EnterRewrite(Rewrite *rw)
// ----------------------------------------------------------------------------
//   Enter the given rewrite in the rewrites table
// ----------------------------------------------------------------------------
{
    // Record if we ever rewrite 0 or "ABC" in that scope
    if (rw->from->IsConstant())
        has_rewrites_for_constants = true;

    // Create symbol table for this rewrite
    Symbols *locals = new Symbols(this);
    rw->from->SetSymbols(locals);

    // Enter parameters in the symbol table
    ParameterMatch parms(locals);
    Tree *check = rw->from->Do(parms);
    if (!check)
        Ooops("Parameter error for $1", rw->from);
    rw->parameters = parms.order;

    if (rewrites)
    {
        /* Returns parent */ rewrites->Add(rw);
        return rw;
    }
    rewrites = rw;
    return rw;
}


Rewrite *Symbols::EnterRewrite(Tree *from, Tree *to)
// ----------------------------------------------------------------------------
//   Create a rewrite for the current context and enter it
// ----------------------------------------------------------------------------
{
    Rewrite *rewrite = new Rewrite(this, from, to, NULL);
    return EnterRewrite(rewrite);
}


uint Symbols::EnterProperty(Context *context,
                            Tree *self, Tree *storage, Tree *properties)
// ----------------------------------------------------------------------------
//   Attach named property or properties to the given storage [#1635]
// ----------------------------------------------------------------------------
//   Properties are entered in the current context as two local declarations:
//   - One is a prefix with a single argument, it sets the property
//   - One is a name, it gets the property
//
//   The value of the property is initialized with the first of:
//   - The value of a matching name in the storage's symbol table, if it exists
//   - The initialization value of the property, if given
//   - A default value appropriate for the given type (0, 0.0, "" or false)
{
    // If the properties are in a block, process children
    while (Block *block = properties->AsBlock())
        properties = block->child;

    // If the property is a sequence, process them in turn
    if (Infix *infix = properties->AsInfix())
        if (infix->name == "\n" || infix->name == ";")
            return EnterProperty(context, self, storage, infix->left)
                +  EnterProperty(context, self, storage, infix->right);

    // If there is a comment on the property, use that as description
    text description = "";
    if (CommentsInfo *cinfo = properties->GetInfo<CommentsInfo>())
        if (uint size = cinfo->before.size())
            description = cinfo->before[size-1];

    // Extract name, value and type
    Symbols *symbols = storage->Symbols();
    Tree *type = NULL;
    Tree *value = NULL;

    // If the property is like "X := 0", take "0" as the value
    if (Infix *infix = properties->AsInfix())
    {
        if (infix->name == ":=")
        {
            properties = infix->left;
            value = infix->right;
        }
    }

    // If the property is like "X : integer", take "integer" as the type
    if (Infix *infix = properties->AsInfix())
    {
        if (infix->name == ":")
        {
            properties = infix->left;
            type = infix->right;
        }
    }

    // If at that stage the property is not a name, we have a problem
    Name *name = properties->AsName();
    if (name == NULL)
    {
        Ooops("Property '$1' is not a name", properties);
        return 0;
    }
    if (type)
        type = symbols->Run(context, type);

    // Check if there is an existing value with that name in the body
    Tree *existing = symbols->Named(name->value);

    // Enter local declarations for the property getter
    TreePosition pos = properties->Position();
    Name *getForm = new Name(name->value, pos);
    Rewrite *rw = symbols->EnterRewrite(getForm, getForm);
    getForm->code = xl_read_property;
    getForm->SetSymbols(symbols);

    // Enter local declaration for the property setter
    Name *setName = new Name(name->value, pos);
    Tree *setArg = new Name(name->value + "_value", pos);
    if (type)
        setArg = new Infix(":", setArg, type, pos);
    Prefix *setPrefix = new Prefix(setName, setArg, pos);
    rw = symbols->EnterRewrite(setPrefix, setName);
    setName->code = (eval_fn) xl_write_property;
    setName->SetSymbols(symbols);

    // Adjust information for the property
    if (description == "")
        description = name->value;
    if (!type)
        type = tree_type;
    if (existing)
    {
        kind k = existing->Kind();
        if (type == integer_type && k != INTEGER ||
            type == real_type && k != REAL ||
            type == text_type && k != TEXT ||
            type == name_type && k != NAME)
        {
            Ooops("Ignoring existing value for name $1", name);
            Ooops("because its current value $1", existing);
            Ooops("is not compatible with type $1", type);
        }
        else
        {
            value = existing;
        }
    }
    if (value)
    {
        kind k = value->Kind();
        if (type == integer_type && k != INTEGER ||
            type == real_type && k != REAL ||
            type == text_type && k != TEXT ||
            type == name_type && k != NAME)
        {
            Ooops("Value for property $1", name);
            Ooops("is declared as $1,", existing);
            Ooops("which is not compatible with type $1", type);
            value = NULL;
        }
    }
    if (!value)
    {
        if (type == integer_type)
            value = new Integer(0, pos);
        else if (type == real_type)
            value = new Real(0.0, pos);
        else if (type == text_type)
            value = new Text("", "\"", "\"", pos);
        else
            value = xl_false;
    }

    // Set information for the property
    Rewrite *prop = symbols->Entry(name->value, true);
    prop->description = description;
    prop->type = type;
    prop->to = value;

    return 1;
}


Rewrite *Symbols::Entry(text name, bool create)
// ----------------------------------------------------------------------------
//   Binary search to find an entry
// ----------------------------------------------------------------------------
{
    ulong key = Context::Hash(name);
    Rewrite *rw = Rewrites();
    Rewrite *last = NULL;
    while (rw)
    {
        if (Name *from = rw->from->AsName())
            if (from->value == name)
                return rw;

        last = rw;
        REWRITE_NEXT(rw, key);
    }

    // Not found, check if we need to create it
    if (!create)
        return NULL;

    // Create entry
    Name *n = new Name(name);
    rw = new Rewrite(n, NULL, NULL);
    if (last)
        last->hash[key % REWRITE_HASH_SIZE] = rw;
    else
        rewrites = rw;
    return rw;
}


Rewrite *Symbols::Entry(Tree *form, bool create)
// ----------------------------------------------------------------------------
//   Find the entry for a given name in all visible scopes
// ----------------------------------------------------------------------------
{
    ulong fromKey = Context::HashForm(form);
    ulong hkey = fromKey;
    Rewrite *rw = Rewrites();
    Rewrite *last = NULL;
    while (rw)
    {
        ulong testKey = Context::HashForm(rw->from);
        if (testKey == fromKey)
            if (Tree::Equal(form, rw->from, true))
                return rw;

        last = rw;
        REWRITE_NEXT(rw, hkey);
    }

    // Not found, check if we need to create it
    if (!create)
        return NULL;

    // Create entry
    rw = new Rewrite(form, NULL, NULL);
    if (last)
        last->hash[hkey % REWRITE_HASH_SIZE] = rw;
    else
        rewrites = rw;
    return rw;
}


Tree *Symbols::ProcessDeclarations(Tree *tree)
// ----------------------------------------------------------------------------
//   Process the declarations for the given tree, and associate it to symbols
// ----------------------------------------------------------------------------
{
    if (source == tree)
        return tree;

    // Process all declarations
    source = tree;
    DeclarationAction declare(this);
    tree = tree->Do(declare);

    return tree;
}


void Symbols::Clear()
// ----------------------------------------------------------------------------
//   Clear all symbol tables
// ----------------------------------------------------------------------------
{
    symbol_table empty;
    if (rewrites)
        rewrites = NULL;        // Decrease reference count
    calls = empty;
    type_tests.clear();
    error_handler = NULL;
    has_rewrites_for_constants = false;
}



// ============================================================================
//
//    Evaluation of trees
//
// ============================================================================

Tree *Symbols::Compile(Tree *source, OCompiledUnit &unit,
                       bool nullIfBad, bool keepAlternatives, bool noData)
// ----------------------------------------------------------------------------
//    Return an optimized version of the source tree, ready to run
// ----------------------------------------------------------------------------
{
    // Record rewrites and data declarations in the current context
    Tree *result = source;
    if (this->source != source)
        result = ProcessDeclarations(result);

    // Compile code for that tree
    CompileAction compile(this, unit, nullIfBad, keepAlternatives, noData);
    result = source->Do(compile);

    // If we didn't compile successfully, report
    if (!result)
    {
        if (nullIfBad)
            return result;
        return Ooops("Couldn't compile $1", source);
    }

    // If we compiled successfully, get the code and store it
    return result;
}


Tree *Symbols::CompileAll(Tree *source,
                          bool nullIfBad,
                          bool keepAlternatives,
                          bool noData)
// ----------------------------------------------------------------------------
//   Compile a top-level tree
// ----------------------------------------------------------------------------
//    This associates an eval_fn to the tree, i.e. code that takes a tree
//    as input and returns a tree as output.
//    keepAlternatives is set by CompileCall to avoid eliding alternatives
//    based on the value of constants, so that if we compile
//    (key "X"), we also generate the code for (key "Y"), knowing that
//    CompileCall may change the constant at run-time. The objective is
//    to avoid re-generating LLVM code for each and every call
//    (it's more difficult to avoid leaking memory from LLVM)
{
    // Fast-compile constants
    if (!has_rewrites_for_constants && source->IsConstant())
    {
        source->code = xl_identity;
        return source;
    }

    Errors errors;

    IFTRACE(compile)
        std::cerr << "In " << this
                  << " compiling top-level " << source << "\n";

    Compiler *compiler = MAIN->compiler;
    TreeList noParms;
    OCompiledUnit unit (compiler, source, noParms, false);
    if (unit.IsForwardCall())
        return source;

    Tree *result = Compile(source, unit, nullIfBad, keepAlternatives, noData);
    if (!result)
        return result;

    eval_fn fn = unit.Finalize();
    source->code = fn;
    source->symbols = this; // Fix for #1017

    IFTRACE(compile)
        std::cerr << "In " << this
                  << " compiled top-level " << source
                  << " code=" << (void *) fn << "\n";

    return source;
}


Tree *Symbols::CompileCall(text callee, TreeList &arglist,
                           bool nullIfBad, bool cached)
// ----------------------------------------------------------------------------
//   Compile a top-level call, reusing calls if possible
// ----------------------------------------------------------------------------
{
    uint arity = arglist.size();
    text key = "";
    if (cached)
    {
        // Build key for this call
        const char keychars[] = "IRTN.[]|";
        std::ostringstream keyBuilder;
        keyBuilder << callee << ":";
        for (uint i = 0; i < arity; i++)
            keyBuilder << keychars[arglist[i]->Kind()];
        key = keyBuilder.str();

        // Check if we already have a call compiled
        if (Tree *previous = calls[key])
        {
            if (arity)
            {
                // Replace arguments in place if necessary
                Prefix *pfx = previous->AsPrefix();
                Tree_p *args = &pfx->right;
                uint argIndex = 0;
                while (*args && argIndex < arity)
                {
                    Tree *value = arglist[argIndex++];
                    Tree *existing = *args;
                    if (Infix *infix = existing->AsInfix())
                    {
                        existing = infix->left;
                        args = &infix->right;
                    }
                    if (Real *rs = value->AsReal())
                    {
                        if (Real *rt = existing->AsReal())
                            rt->value = rs->value;
                        else
                            Ooops("Real $1 cannot replace non-real $2",
                                  value, existing);
                    }
                    else if (Integer *is = value->AsInteger())
                    {
                        if (Integer *it = existing->AsInteger())
                            it->value = is->value;
                        else
                            Ooops("Integer $1 cannot replace "
                                  "non-integer $2", value, existing);
                    }
                    else if (Text *ts = value->AsText())
                    {
                        if (Text *tt = existing->AsText())
                            tt->value = ts->value;
                        else
                            Ooops("Text $1 cannot replace non-text $2",
                                  value, existing);
                    }
                    else
                    {
                        Ooops("Call has unsupported type for $1", value);
                    }
                }
            }

            // Call the previously compiled code
            return previous;
        }
    }

    Tree *call = new Name(callee);
    if (arity)
    {
        Tree *args = arglist[arity + ~0];
        for (uint a = 1; a < arity; a++)
            args = new Infix(",", arglist[arity + ~a], args);
        call = new Prefix(call, args);
    }
    call = CompileAll(call, nullIfBad, true, false);
    if (cached)
        calls[key] = call;
    return call;
}


Infix *Symbols::CompileTypeTest(Tree *type)
// ----------------------------------------------------------------------------
//   Compile a type test
// ----------------------------------------------------------------------------
{
    // Check if we already have a call compiled for that type
    if (Tree *previous = type_tests[type])
        if (Infix *infix = previous->AsInfix())
            if (infix->code)
                return infix;

    // Create an infix node with two parameters for left and right
    Name *valueParm = new Name("_");
    Infix *call = new Infix(":", valueParm, type);
    TreeList parameters;
    parameters.push_back(valueParm);
    type_tests[type] = call;

    // Create the compilation unit for the infix with two parms
    Compiler *compiler = MAIN->compiler;
    OCompiledUnit unit(compiler, call, parameters, false);
    if (unit.IsForwardCall())
        return call;

    // Create local symbols
    Symbols *locals = new Symbols (this);

    // Record rewrites and data declarations in the current context
    DeclarationAction declare(locals);
    Tree *callDecls = call->Do(declare);
    if (!callDecls)
        Ooops("Internal: Declaration error for call $1", callDecls);

    // Compile the body of the rewrite, keep all alternatives open
    CompileAction compile(locals, unit, false, false, false);
    Tree *result = callDecls->Do(compile);
    if (!result)
        Ooops("Error compiling type test $1", callDecls);

    // Even if technically, this is not an 'eval_fn' (it has more args),
    // we still record it to avoid recompiling multiple times
    eval_fn fn = compile.unit.Finalize();
    call->code = fn;
    call->symbols = locals; // Fix for #1017

    return call;
}


Tree *Symbols::Run(Context *context, Tree *code)
// ----------------------------------------------------------------------------
//   Execute a tree by applying the rewrites in the current context
// ----------------------------------------------------------------------------
{
    static uint index = 0;

    // Trace what we are doing
    Tree *result = code;
    IFTRACE(eval)
        std::cerr << "EVAL" << ++index << ": " << code << '\n';

    // Check trees that we won't rewrite
    if (has_rewrites_for_constants || !code || !code->IsConstant())
    {
        if (!result->code)
        {
            Errors errors;
            Symbols *symbols = result->Symbols();
            if (!symbols)
            {
                std::cerr << "WARNING: Tree '" << code
                          << "' has no symbols\n";
                symbols = this;
            }
            result = symbols->CompileAll(result);
            if (!result->code || errors.Count())
                return Ooops("Error compiling $1", result);
        }

        // Check infinite recursion
        StackDepthCheck stackDepthCheck(result);
        if (!stackDepthCheck)
            result = result->code(context, code);
    }
    IFTRACE(eval)
        std::cerr << "RSLT" << index-- << ": " << result << '\n';
    return result;
}



// ============================================================================
//
//    Error handling
//
// ============================================================================

Tree *Symbols::Ooops(text message, Tree *arg1, Tree *arg2, Tree *arg3)
// ----------------------------------------------------------------------------
//   Execute the innermost error handler
// ----------------------------------------------------------------------------
{
    XLCall call("error");
    call, message;
    if (arg1)
        call, FormatTreeForError(arg1);
    if (arg2)
        call, FormatTreeForError(arg2);
    if (arg3)
        call, FormatTreeForError(arg3);

    Tree *result = call(this, true, false);
    if (!result)
    {
        // Fallback to displaying error on std::err
        XL::Error err(message, arg1, arg2, arg3);
        err.Display();
        return XL::xl_false;
    }
    return result;
}



// ============================================================================
//
//   Stack depth check
//
// ============================================================================

uint StackDepthCheck::stack_depth      = 0;
uint StackDepthCheck::max_stack_depth  = 0;
bool StackDepthCheck::in_error_handler = false;
bool StackDepthCheck::in_error         = false;


void StackDepthCheck::StackOverflow(Tree *what)
// ----------------------------------------------------------------------------
//   We have a stack overflow, bummer
// ----------------------------------------------------------------------------
{
    if (!max_stack_depth)
    {
        max_stack_depth = Options::options->stack_depth;
        if (stack_depth <= max_stack_depth)
            return;
    }
    if (in_error_handler)
    {
        Error("Double stack overflow in $1", what, NULL, NULL).Display();
        in_error_handler = false;
    }
    else
    {
        in_error = true;
        Save<bool> overflow(in_error_handler, true);
        Save<uint> depth(stack_depth, 1);
        Ooops("Stack overflow evaluating $1", what);
    }
}



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
        // We only allow names here, not symbols (bug #154)
        if (what->value.length() == 0 || !isalpha(what->value[0]))
            Ooops("The pattern variable $1 is not a name", what);

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
//   Parameters in a block belong to the child
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
        // Check the variable name, e.g. K in example above
        Name *varName = what->left->AsName();
        if (!varName)
        {
            Ooops("Expected a name, got $1 ", what->left);
            return NULL;
        }

        // Check if the name already exists
        if (Tree *existing = symbols->Named(varName->value, false))
        {
            Ooops("Typed name $1 already exists as $2",
                  what->left, existing);
            Ooops("This is the previous declaration of $1",
                  existing);
            return NULL;
        }

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
//    Argument matching - Test input arguments against parameters
//
// ============================================================================

Tree *ArgumentMatch::Compile(Tree *source, bool noData)
// ----------------------------------------------------------------------------
//    Compile the source tree, and record we use the value in expr cache
// ----------------------------------------------------------------------------
{
    // Compile the code
    if (!unit.IsKnown(source))
    {
        bool nullIfBad = true;
        bool keepAlt = false;
        source = symbols->Compile(source, unit, nullIfBad, keepAlt, noData);
    }
    else
    {
        // Generate code to evaluate the argument
        Save<bool> nib(compile->nullIfBad, true);
        Save<bool> nod(compile->noDataForms, noData);
        source = source->Do(compile);
    }

    if (source && !source->Symbols())
        source->SetSymbols(symbols);

    return source;
}


Tree *ArgumentMatch::CompileValue(Tree *source, bool noData)
// ----------------------------------------------------------------------------
//   Compile the source and make sure we evaluate it
// ----------------------------------------------------------------------------
{
    Tree *result = Compile(source, noData);
    if (result)
    {
        if (Name *name = result->AsName())
        {
            llvm::BasicBlock *bb = unit.BeginLazy(name);
            unit.NeedStorage(name);
            if (!name->Symbols())
                name->SetSymbols(symbols);
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
        return Compile(source, true);

    // For more complex expression, return a constant tree
    unit.ConstantTree(source);
    if (!source->Symbols())
        source->SetSymbols(symbols);

    // Record which elements of the expression are captured from context
    Compiler *compiler = MAIN->compiler;
    EnvironmentScan env(symbols);
    Tree *envOK = source->Do(env);
    if (!envOK)
    {
        Ooops("Internal: what environment in $1?", source);
        return NULL;
    }

    // Create the parameter list with all imported locals
    TreeList parms, args;
    capture_table::iterator c;
    for (c = env.captured.begin(); c != env.captured.end(); c++)
    {
        Tree *name = (*c).first;
        Tree *value = (*c).second;
        if (!unit.IsKnown(value))
            value = Compile(value, true);
        if (unit.IsKnown(value))
        {
            // This is a local: simply pass it around
            parms.push_back(name);
            args.push_back(value);
        }
        else
        {
            // This is a local 'name' like a form definition
            // We don't need to pass these around.
            IFTRACE(closure)
                std::cerr << "WARNING: Tree '" << name
                          << "' not allocated in LLVM\n";
        }
    }

    // Create the compilation unit for the code to enclose
    bool isCallableDirectly = parms.size() == 0;
    OCompiledUnit subUnit(compiler, source, args, !isCallableDirectly);
    if (!subUnit.IsForwardCall())
    {
        // If there is an error compiling, make sure we report it
        // but only if we attempt to actually evaluate the tree
        if (!symbols->Compile(source, subUnit, true))
            subUnit.CallTypeError(source);
        if (eval_fn fn = subUnit.Finalize())
            if (isCallableDirectly)
                source->code = fn;
    }

    // Create a call to xl_new_closure to save the required trees
    unit.CreateClosure(source, parms, args, subUnit.function);

    return source;
}


Tree *ArgumentMatch::Do(Tree *)
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
        Integer *it = test->AsInteger();
        if (!it)
            return NULL;
        if (!compile->keepAlternatives)
        {
            if (it->value == what->value)
                return what;
            return NULL;
        }
    }

    // Compile the test tree
    Tree *compiled = CompileValue(test, true);
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
        Real *rt = test->AsReal();
        if (!rt)
            return NULL;
        if (!compile->keepAlternatives)
        {
            if (rt->value == what->value)
                return what;
            return NULL;
        }
    }

    // Compile the test tree
    Tree *compiled = CompileValue(test, true);
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
        Text *tt = test->AsText();
        if (!tt)
            return NULL;
        if (!compile->keepAlternatives)
        {
            if (tt->value == what->value)
                return what;
            return NULL;
        }
    }

    // Compile the test tree
    Tree *compiled = CompileValue(test, true);
    if (!compiled)
        return NULL;

    // Compare at run-time the actual tree value with the test value
    unit.TextTest(compiled, what->value);
    return compiled;
}


Tree *ArgumentMatch::DoName(Name *what)
// ----------------------------------------------------------------------------
//    Bind arguments to parameters being defined in the shape
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
        if (Tree *existing = locals->Named(what->value))
        {
            // Check if the test is an identity
            if (Name *nt = test->AsName())
            {
                if (nt->code == xl_identity || data)
                {
                    if (nt->value == what->value)
                        return what;
                    return NULL;
                }
            }

            if (existing->Kind() == NAME ||
                existing == locals->Named(what->value, false))
            {
                // Insert a dynamic tree comparison test
                Tree *testCode = Compile(test, false);
                if (!testCode || !unit.IsKnown(testCode))
                    return NULL;
                Tree *thisCode = Compile(existing, false);
                if (!thisCode || !unit.IsKnown(thisCode))
                    return NULL;
                unit.ShapeTest(testCode, thisCode);
                
                // Return compilation success
                return what;
            }
        }

        // Bind expression to name, not value of expression (create a closure)
        Tree *compiled = CompileClosure(test);
        if (!compiled)
            return NULL;

        // If first occurence of the name, enter it in symbol table
        locals->EnterName(what->value, compiled, Rewrite::ARG);
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
        (what->opening == "{" && what->closing == "}") ||
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
    // Check if we match an infix tree like 'x,y' with a name like 'A'
    if (what->name != ":")
    {
        if (Name *name = test->AsName())
        {
            if (!unit.IsKnown(test))
            {
                if (Tree *value = symbols->Named(name->value))
                {
                    // For non-names, evaluate the expression
                    if (!unit.IsKnown(value))
                    {
                        value = CompileValue(value, false);
                        if (!value)
                            return NULL;
                    }
                    if (unit.IsKnown(value))
                        test = value;
                }
            }

            if (unit.IsKnown(test))
            {
                // Build an infix tree corresponding to what we extract
                Name *left = new Name("left");
                Name *right = new Name("right");
                Infix *extracted = new Infix(what->name, left, right);

                // Extract the infix parameters from actual value
                unit.InfixMatchTest(test, extracted);

                // Proceed with the infix we extracted to
                // map the remaining args
                test = extracted;
            }
        }
    }

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
        Name *varName = what->left->AsName();
        if (!varName)
        {
            Ooops("Expected a name, got $1 ", what->left);
            return NULL;
        }

        // Check for types that don't require a type check
        bool needEvaluation = true;
        bool needRTTypeTest = true;
        if (Name *declTypeName = what->right->AsName())
        {
            if (Tree *namedType = symbols->Named(declTypeName->value))
            {
                if (namedType == tree_type ||
                    namedType == code_type ||
                    namedType == lazy_type)
                    return DoName(varName);
                kind tk = test->Kind();
                if ((namedType == source_type) ||
                    (namedType == name_type && tk == NAME) ||
                    (namedType == block_type && tk == BLOCK) ||
                    (namedType == infix_type && tk == INFIX) ||
                    (namedType == prefix_type && tk == PREFIX))
                {
                    needEvaluation = false;
                    needRTTypeTest = namedType != source_type;
                }
            }
        }

        // Evaluate type expression, e.g. 'integer' in example above
        Tree *typeExpr = what->right;
        if (needRTTypeTest)
        {
            typeExpr = Compile(what->right, true);
            if (!typeExpr)
                return NULL;
        }

        // Compile what we are testing against
        Tree *compiled = test;
        if (needEvaluation)
        {
            compiled = Compile(compiled, true);
            if (!compiled)
                return NULL;
        }
        else
        {
            unit.ConstantTree(compiled);
            if (!compiled->Symbols())
                compiled->SetSymbols(symbols);
        }

        // Insert a run-time type test
        if (needRTTypeTest)
        {
            if (!typeExpr->Symbols())
                typeExpr->SetSymbols(symbols);
            unit.TypeTest(compiled, typeExpr);
        }

        // Enter the compiled expression in the symbol table
        locals->EnterName(varName->value, compiled, Rewrite::ARG);

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
        if (!rr)
        {
            if (Block *br = test->AsBlock())
            {
                test = br->child;
                rr = what->right->Do(this);
            }
        }
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
        if (!lr)
        {
            if (Block *br = test->AsBlock())
            {
                test = br->child;
                lr = what->right->Do(this);
            }
        }
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
    for (Symbols *s = symbols; s && !s->is_global; s = s->Parent())
    {
        if (Tree *existing = s->Named(what->value, false))
        {
            // Found the symbol in the given symbol table
            if (!captured.count(what))
                captured[what] = existing;
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
    if (!what->IsParentheses() || what->child->Kind() != NAME)
        what->child->Do(this);
    return what;
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
    if (what->left->Kind() != NAME)
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
    if (what->right->Kind() != NAME)
        what->right->Do(this);
    what->left->Do(this);
    return what;
}



// ============================================================================
//
//   EvaluateChildren action: Build a non-leaf after evaluating children
//
// ============================================================================

Tree *EvaluateChildren::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Compile integer constants
// ----------------------------------------------------------------------------
{
    return compile->DoInteger(what);
}


Tree *EvaluateChildren::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Compile real constants
// ----------------------------------------------------------------------------
{
    return compile->DoReal(what);
}


Tree *EvaluateChildren::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Compile text constants
// ----------------------------------------------------------------------------
{
    return compile->DoText(what);
}


Tree *EvaluateChildren::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Compile names
// ----------------------------------------------------------------------------
{
    return compile->DoName(what, true);
}


Tree *EvaluateChildren::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   Evaluate children of a prefix
// ----------------------------------------------------------------------------
{
    OCompiledUnit &unit = compile->unit;
    unit.ConstantTree(what->left);
    Tree *right = what->right->Do(compile);
    if (!right)
        return NULL;
    unit.CallFillPrefix(what);
    return what;
}


Tree *EvaluateChildren::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//   Evaluate children of a postfix
// ----------------------------------------------------------------------------
{
    OCompiledUnit &unit = compile->unit;
    Tree *left = what->left->Do(compile);
    if (!left)
        return NULL;
    unit.ConstantTree(what->right);
    unit.CallFillPostfix(what);
    return what;
}


Tree *EvaluateChildren::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Evaluate children, then build an infix
// ----------------------------------------------------------------------------
{
    OCompiledUnit &unit = compile->unit;
    Tree *left = what->left->Do(compile);
    if (!left)
        return NULL;
    Tree *right = what->right->Do(compile);
    if (!right)
        return NULL;
    unit.CallFillInfix(what);
    return what;
}


Tree *EvaluateChildren::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   Evaluate children, then build a new block
// ----------------------------------------------------------------------------
{
    OCompiledUnit &unit = compile->unit;
    Tree *child = what->child->Do(compile);
    if (!child)
        return NULL;
    unit.CallFillBlock(what);
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
//   Declarations in a block belong to the child, not to us
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
    // Deal with 'data' declarations and 'load' statements
    if (Name *name = what->left->AsName())
    {
        // Check if there is some stuff that needs to be done at decl time
        // This is used for 'load' and 'import'
        declarator_table::iterator fnd = Symbols::declarators.find(name->value);
        if (fnd != Symbols::declarators.end())
            if (decl_fn fn = (*fnd).second)
                if (Tree *result = fn(symbols, what, false))
                    return result;

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


void DeclarationAction::EnterRewrite(Tree *defined,
                                     Tree *definition)
// ----------------------------------------------------------------------------
//   Add a definition in the current context
// ----------------------------------------------------------------------------
{
    if (definition)
    {
#if CREATE_NAME_FOR_PREFIX
        // When entering 'foo X,Y -> bar', also update the definition of 'foo'
        if (Prefix *prefix = defined->AsPrefix())
        {
            if (Name *left = prefix->left->AsName())
            {
                Infix *redef = new Infix("->",
                                         prefix->right, definition,
                                         prefix->Position());
                symbols->ExtendName(left->value, redef);
            }
        }
#endif // CREATE_NAME_FOR_PREFIX

#if CREATE_NAMES_FOR_POSTFIX_AND_INFIX
        // Same thing for a postfix
        if (Postfix *postfix = defined->AsPostfix())
        {
            if (Name *right = postfix->right->AsName())
            {
                Infix *redef = new Infix("->",
                                         postfix->left, definition,
                                         postfix->Position());
                symbols->ExtendName(right->value, redef);
            }
        }

        // Same thing for an infix, but use X,Y on the left of the rewrite
        if (Infix *infix = defined->AsInfix())
        {
            if (infix->name != "," && infix->name != ";" && infix->name != "\n")
            {
                if (Name *left = infix->left->AsName())
                {
                    if (Name *right = infix->right->AsName())
                    {
                        Infix *comma = new Infix(",", left, right,
                                                 infix->Position());
                        Infix *redef = new Infix("->", comma, definition,
                                                 infix->Position());
                        symbols->ExtendName(infix->name, redef);
                    }
                }
            }
        }
#endif // CREATE_NAMES_FOR_POSTFIX_AND_INFIX
    }

    if (Name *name = defined->AsName())
    {
        symbols->EnterName(name->value,
                           definition ? (Tree *) definition : (Tree *) name,
                           Rewrite::LOCAL);
    }
    else
    {
        Rewrite *rewrite = new Rewrite(symbols, defined, definition, NULL);
        symbols->EnterRewrite(rewrite);
    }
}



// ============================================================================
//
//   Compilation action - Generation of "optimized" native trees
//
// ============================================================================

CompileAction::CompileAction(Symbols *s, OCompiledUnit &u,
                             bool nib, bool ka, bool ndf)
// ----------------------------------------------------------------------------
//   Constructor
// ----------------------------------------------------------------------------
    : symbols(s), unit(u),
      nullIfBad(nib), keepAlternatives(ka), noDataForms(ndf),
      debugRewrites(0)
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
//   Normal name evaluation: don't force evaluation
// ----------------------------------------------------------------------------
{
    return DoName(what, false);
}


Tree *CompileAction::DoName(Name *what, bool forceEval)
// ----------------------------------------------------------------------------
//   Build a unique reference in the context for the entity
// ----------------------------------------------------------------------------
{
    // Normally, the name should have been declared in ParameterMatch
    if (Tree *result = symbols->Named(what->value))
    {
        // Try to compile the definition of the name
        TreeList xargs;
        if (!result->AsName())
        {
            Rewrite rw(symbols, what, result);
            if (!what->Symbols())
                what->SetSymbols(symbols);
            result = rw.Compile(xargs);
            if (!result)
                return result;
        }

        // Check if there is code we need to call
        Compiler *compiler = MAIN->compiler;
        llvm::Function *function = compiler->TreeFunction(result);
        if (function && function != unit.function)
        {
            // Case of "Name -> Foo": Invoke Name
            unit.NeedStorage(what);
            unit.Invoke(what, result, xargs);
            return what;
        }
        else if (forceEval && unit.IsKnown(result))
        {
            unit.CallEvaluate(result);
        }
        else if (unit.IsKnown(result))
        {
            // Case of "Foo(A,B) -> B" with B: evaluate B lazily
            unit.Copy(result, what, false);
            return what;
        }
        else
        {
            // Return the name itself by default
            unit.ConstantTree(result);
            unit.Copy(result, what);
            if (!result->Symbols())
                result->SetSymbols(symbols);
        }

        return result;
    }
    if (nullIfBad)
    {
        unit.ConstantTree(what);
        return what;
    }
    Ooops("Name $1 does not exist", what);
    return NULL;
}


Tree *CompileAction::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   Optimize away indent or parenthese blocks, evaluate others
// ----------------------------------------------------------------------------
{
    if ((what->opening == Block::indent && what->closing == Block::unindent) ||
        (what->opening == "{" && what->closing == "}") ||
        (what->opening == "(" && what->closing == ")"))
    {
        if (Name *name = what->child->AsName())
        {
            if (name->value == "")
            {
                unit.ConstantTree(what);
                return what;
            }
        }

        if (unit.IsKnown(what))
            unit.Copy(what, what->child, false);
        Tree *result = what->child->Do(this);
        if (!result)
            return NULL;
        if (!what->child->Symbols())
            what->child->SetSymbols(symbols);
        if (unit.IsKnown(result))
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
        // Force evaluation of names on the left of a sequence
        if (Name *leftAsName = what->left->AsName())
        {
            if (!DoName(leftAsName, true))
                return NULL;
        }
        else if (!what->left->Do(this))
        {
            return NULL;
        }
        if (unit.IsKnown(what->left))
        {
            if (!what->left->Symbols())
                what->left->SetSymbols(symbols);
        }
        if (Name *rightAsName = what->right->AsName())
        {
            if (!DoName(rightAsName, true))
                return NULL;
        }
        else if (!what->right->Do(this))
        {
            return NULL;
        }
        if (unit.IsKnown(what->right))
        {
            if (!what->right->Symbols())
                what->right->SetSymbols(symbols);
            unit.Copy(what->right, what);
        }
        else if (unit.IsKnown(what->left))
        {
            unit.Copy(what->left, what);
        }
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
//    Deal with data declarations, otherwise translate as a rewrite
// ----------------------------------------------------------------------------
{
    if (Name *name = what->left->AsName())
    {
        if (name->value == "data")
        {
            if (!what->right->Symbols())
                what->right->SetSymbols(symbols);
            return what;
        }

        // A breakpoint location for convenience
        if (name->value == Options::options->debugPrefix)
        {
            Save<char> saveDebugFlag(debugRewrites, debugRewrites+1);
            return Rewrites(what);
        }
    }

    // Special case the A[B] notation
    if (Block *br = what->right->AsBlock())
    {
        if (br->IsSquare())
        {
            what->left->SetSymbols(symbols);
            what->right->SetSymbols(symbols);
            br->child->SetSymbols(symbols);
            what->left->Do(this);
            br->child->Do(this);
            unit.CallArrayIndex(what, what->left, br->child);
            return what;
        }
    }

    return Rewrites(what);
}


Tree *CompileAction::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//    All postfix operations translate into a rewrite
// ----------------------------------------------------------------------------
{
    return Rewrites(what);
}


Tree *CompileAction::Rewrites(Tree *what)
// ----------------------------------------------------------------------------
//   Build code selecting among rewrites in current context
// ----------------------------------------------------------------------------
{
    // Compute the hash key for the form we have to match
    ulong formKey = Context::HashForm(what);
    bool foundUnconditional = false;
    bool foundSomething = false;
    ExpressionReduction reduction (unit, what);
    symbols_set visited;
    symbols_list lookups;

    // Build all the symbol tables that we are going to look into
    BuildSymbolsList(symbols, visited, lookups);

    // Iterate over all symbol tables listed above
    symbols_list::iterator li;
    for (li = lookups.begin(); !foundUnconditional && li != lookups.end(); li++)
    {
        Symbols *s = *li;

        Rewrite *candidate = s->Rewrites();
        ulong hkey = formKey;

        while (candidate && !foundUnconditional)
        {
            // Compute the hash key for the 'from' of the current rewrite
            ulong testKey = Context::Hash(candidate->from);

            // If we have an exact match for the keys, we may have a winner
            if (testKey == formKey && (!noDataForms || candidate->to))
            {
                // Create the invokation point
                reduction.NewForm();
                Symbols_p args = new Symbols(candidate->symbols);
                ArgumentMatch matchArgs(what,
                                        symbols, args, candidate->symbols,
                                        this, !candidate->to);
                Tree *argsTest = candidate->from->Do(matchArgs);
                if (debugRewrites > 0)
                {
                    std::cerr << "REWRITE" << (int) debugRewrites
                              << ": " << candidate->from
                              << (argsTest ? " MATCH" : "FAIL") << "\n";
                    debugRewrites = -debugRewrites;
                }

                if (argsTest)
                {
                    // Record that we found something
                    foundSomething = true;

                    // If this is a data form, we are done
                    if (!candidate->to)
                    {
                        // Set the symbols for the result
                        if (!what->Symbols())
                            what->SetSymbols(symbols);
                        RewriteChildren(what);
                        foundUnconditional = !unit.failbb;
                        unit.dataForm.insert(what);
                        reduction.Succeeded();
                    }
                    else
                    {
                        // We should have same number of args and parms
                        Symbols &parms = *candidate->from->Symbols();
                        ulong parmCount = parms.Count(1<<Rewrite::PARM);
                        ulong argCount = args->Count(1<<Rewrite::ARG);
                        if (argCount != parmCount)
                        {
                            symbol_iter a, p;
                            std::cerr << "Args/parms mismatch: "
                                      << parmCount << " parms, "
                                      << argCount << " args\n";
                            std::cerr << "Parms:\n";
                            debugsy(&parms);
                            std::cerr << "Args:\n";
                            debugsy(args);
                        }

                        // Map the arguments we found in parameter order
                        TreeList argsList;
                        TreeList::iterator p;
                        TreeList &order = candidate->parameters;
                        for (p = order.begin(); p != order.end(); p++)
                        {
                            Name *name = (*p)->AsName();
                            Tree *argValue = args->Named(name->value);
                            argsList.push_back(argValue);
                        }

                        // Compile the candidate
                        Tree *code = candidate->Compile(argsList);
                        if (code)
                        {
                            // Invoke the candidate
                            unit.Invoke(what, code, argsList);

                            // If there was no test code, don't keep testing
                            foundUnconditional = !unit.failbb;

                            // This is the end of a successful invokation
                            reduction.Succeeded();
                        }
                        else
                        {
                            reduction.Failed();
                        }
                    } // if (data form)

                } // Match args
                else
                {
                    // Indicate unsuccessful invokation
                    reduction.Failed();
                }

                if (debugRewrites < 0)
                    debugRewrites = -debugRewrites;

            } // Match test key

            // Otherwise, check if we have a key match in the hash table,
            // and if so follow it.
            if (foundUnconditional)
                candidate = NULL;
            else
                REWRITE_NEXT(candidate, hkey);
        } // while(candidate)
    } // for(namespaces)

    // If we didn't match anything, then emit an error at runtime
    if (!foundUnconditional)
        unit.CallTypeError(what);

    // If we didn't find anything, report it
    if (!foundSomething)
    {
        if (nullIfBad)
        {
            // Set the symbols for the result
            if (!what->Symbols())
                what->SetSymbols(symbols);
            if (!noDataForms)
                RewriteChildren(what);
            return NULL;
        }
        Ooops("No rewrite candidate for $1", what);
        return NULL;
    }

    // Set the symbols for the result
    if (!what->Symbols())
        what->SetSymbols(symbols);

    return what;
}


Tree *CompileAction::RewriteChildren(Tree *what)
// ----------------------------------------------------------------------------
//   Generate code for children of a structured tree
// ----------------------------------------------------------------------------
{
    EvaluateChildren eval(this);
    if (!what->Symbols())
        what->SetSymbols(symbols);
    return what->Do(eval);
}



// ============================================================================
//
//    Tree rewrites -- Obsolete stuff
//
// ============================================================================

Rewrite *Rewrite::Add (Rewrite *rewrite)
// ----------------------------------------------------------------------------
//   Add a new rewrite at the right place in an existing rewrite
// ----------------------------------------------------------------------------
{
    // Compute the hash key for the form we have to match
    Rewrite *parent = this;
    ulong formKey = Context::HashForm(rewrite->from);
    ulong hkey = formKey;

    while (parent)
    {
        REWRITE_HASH_SHIFT(hkey);
        if (Rewrite *child = parent->hash[hkey % REWRITE_HASH_SIZE])
        {
            parent = child;
        }
        else
        {
            parent->hash[hkey % REWRITE_HASH_SIZE] = rewrite;
            return parent;
        }
    }

    return NULL;
}


Tree *Rewrite::Do(Action &a)
// ----------------------------------------------------------------------------
//   Apply an action to the 'from' and 'to' fields and all referenced trees
// ----------------------------------------------------------------------------
{
    Tree *result = from->Do(a);
    if (to)
        result = to->Do(a);
    for (uint i = 0; i < REWRITE_HASH_SIZE; i++)
        if (Rewrite *rw = hash[i])
            result = rw->Do(a);
    for (TreeList::iterator p=parameters.begin(); p!=parameters.end(); p++)
        result = (*p)->Do(a);
    return result;
}


Tree *Rewrite::Compile(TreeList &xargs)
// ----------------------------------------------------------------------------
//   Compile code for the 'to' form
// ----------------------------------------------------------------------------
//   This is similar to Context::Compile, except that it may generate a
//   function with more parameters, i.e. Tree *f(Tree * , Tree * , ...),
//   where there is one input arg per variable in the 'from' tree or per
//   captured variable from the surrounding context
{
    assert (to || !"Rewrite::Compile called for data rewrite?");

    // Check if there are variables in the environment that we need to capture
    Symbols_p syms = from->Symbols();
    if (!syms)
        Ooops("Internal: No symbols for $1", from);
    TreeList xparms = parameters;
    EnvironmentScan envScan(syms->Parent());
    Tree *envOK = to->Do(envScan);
    if (!envOK)
        Ooops("Internal: environment capture error in $1", to);
    capture_table &ct = envScan.captured;
    for (capture_table::iterator ci = ct.begin(); ci != ct.end(); ci++)
    {
        // We only capture local arguments
        if (Name *n1 = (*ci).first->AsName())
        {
            if (Name *n2 = (*ci).second->AsName())
            {
                if (n1->value == n2->value)
                {
                    xparms.push_back(n2);
                    xargs.push_back(n2);
                }
            }
        }
    }

    // Check if already compiled
    if (to->code)
        return to;

    Compiler *compiler = MAIN->compiler;

    // Create the compilation unit and check if we are already compiling this
    OCompiledUnit unit(compiler, to, xparms, false);
    if (unit.IsForwardCall())
    {
        // Recursive compilation of that form
        return to;              // We know how to invoke it anyway
    }

    // Create local symbols
    Symbols_p locals = new Symbols (syms);

    // Record rewrites and data declarations in the current context
    DeclarationAction declare(locals);
    Tree *toDecl = to->Do(declare);
    if (!toDecl)
        Ooops("Internal: Declaration error for $1", to);

    // Compile the body of the rewrite
    CompileAction compile(locals, unit, false, false, false);
    Tree *result = to->Do(compile);
    if (!result)
    {
        Ooops("Error compiling rewrite $1", to);
        return NULL;
    }

    // Even if technically, this is not an 'eval_fn' (it has more args),
    // we still record it to avoid recompiling multiple times
    eval_fn fn = unit.Finalize();
    to->code = fn;
    to->symbols = locals; // Record symbols, fix for #1017

    return to;
}



// ============================================================================
//
//   OCompiledUnit - A particular piece of code we generate for a tree
//
// ============================================================================
//  This is the "old" version that generates relatively inefficient machine code
//  However, it is at the moment more complete than the 'new' version and is
//  therefore preferred for the beta.

using namespace llvm;

OCompiledUnit::OCompiledUnit(Compiler *comp,
                             Tree *src,
                             TreeList parms,
                             bool closure)
// ----------------------------------------------------------------------------
//   OCompiledUnit constructor
// ----------------------------------------------------------------------------
    : compiler(comp), llvm(comp->llvm), source(src),
      code(NULL), data(NULL), function(NULL),
      allocabb(NULL), entrybb(NULL), exitbb(NULL), failbb(NULL),
      contextPtr(NULL)
{
    IFTRACE(llvm)
        std::cerr << "OCompiledUnit T" << (void *) src;

    // If a compilation for that tree is alread in progress, fwd decl
    function = closure
        ? compiler->TreeClosure(src)
        : compiler->TreeFunction(src);
    if (function)
    {
        // We exit here without setting entrybb (see IsForward())
        IFTRACE(llvm)
            std::cerr << " exists F" << function << "\n";
        return;
    }

    // Create the function signature, one entry per parameter + one for source
    std::vector<const Type *> signature;
    signature.push_back(compiler->contextPtrTy);
    Type *treePtrTy = compiler->treePtrTy;
    for (ulong p = 0; p <= parms.size(); p++)
        signature.push_back(treePtrTy);
    FunctionType *fnTy = FunctionType::get(treePtrTy, signature, false);
    text label = "xl_eval";
    IFTRACE(labels)
        label += "[" + text(*src) + "]";
    function = Function::Create(fnTy, Function::InternalLinkage,
                                label.c_str(), compiler->module);

    // Save it in the compiler
    if (closure)
        compiler->SetTreeClosure(src, function);
    else
        compiler->SetTreeFunction(src, function);
    IFTRACE(llvm)
        std::cerr << " new F" << function << "\n";

    // Create function entry point, where we will have all allocas
    allocabb = BasicBlock::Create(*llvm, "allocas", function);
    data = new IRBuilder<> (allocabb);

    // Create entry block for the function
    entrybb = BasicBlock::Create(*llvm, "entry", function);
    code = new IRBuilder<> (entrybb);

    // Associate the value for the input tree
    Function::arg_iterator args = function->arg_begin();
    contextPtr = args++;
    Value *inputArg = args++;
    Value *resultStorage = data->CreateAlloca(treePtrTy, 0, "result");
    data->CreateStore(inputArg, resultStorage);
    storage[src] = resultStorage;

    // Associate the value for the additional arguments (read-only, no alloca)
    TreeList::iterator parm;
    ulong parmsCount = 0;
    for (parm = parms.begin(); parm != parms.end(); parm++)
    {
        inputArg = args++;
        value[*parm] = inputArg;
        parmsCount++;
    }

    // Create the exit basic block and return statement
    exitbb = BasicBlock::Create(*llvm, "exit", function);
    IRBuilder<> exitcode(exitbb);
    Value *retVal = exitcode.CreateLoad(resultStorage, "retval");
    exitcode.CreateRet(retVal);

    // Record current entry/exit points for the current expression
    failbb = NULL;
}


OCompiledUnit::~OCompiledUnit()
// ----------------------------------------------------------------------------
//   Delete what we must...
// ----------------------------------------------------------------------------
{
    if (entrybb && exitbb)
    {
        // If entrybb is clear, we may be looking at a forward declaration
        // Otherwise, if exitbb was not cleared by Finalize(), this means we
        // failed to compile. Make sure the compiler forgets the function
        compiler->SetTreeFunction(source, NULL);
        function->eraseFromParent();
    }

    delete code;
    delete data;
}


eval_fn OCompiledUnit::Finalize()
// ----------------------------------------------------------------------------
//   Finalize the build of the current function
// ----------------------------------------------------------------------------
{
    IFTRACE(llvm)
        std::cerr << "OCompiledUnit Finalize T" << (void *) source
                  << " F" << function;

    // Branch to the exit block from the last test we did
    code->CreateBr(exitbb);

    // Connect the "allocas" to the actual entry point
    data->CreateBr(entrybb);

    // Verify the function we built
    verifyFunction(*function);
    if (compiler->optimizer)
        compiler->optimizer->run(*function);

    IFTRACE(code)
    {
        function->print(errs());
    }

    void *result = compiler->runtime->getPointerToFunction(function);
    IFTRACE(llvm)
        std::cerr << " C" << (void *) result << "\n";

    exitbb = NULL;              // Tell destructor we were successful
    return (eval_fn) result;
}


Value *OCompiledUnit::NeedStorage(Tree *tree, Tree *source)
// ----------------------------------------------------------------------------
//    Allocate storage for a given tree
// ----------------------------------------------------------------------------
{
    Value *result = storage[tree];
    if (!result)
    {
        // Create alloca to store the new form
        text label = "loc";
        IFTRACE(labels)
            label += "[" + text(*tree) + "]";
        const char *clabel = label.c_str();
        result = data->CreateAlloca(compiler->treePtrTy, 0, clabel);
        storage[tree] = result;

        // Deal with uninitialized values
        if (!value.count(tree))
        {
            if (source && value.count(source))
            {
                value[tree] = value[source];
            }
            else
            {
                Constant *null = ConstantPointerNull::get(compiler->treePtrTy);
                data->CreateStore(null, result);
            }
        }
    }
    if (value.count(tree))
    {
        data->CreateStore(value[tree], result);
    }
    else if (Value *global = compiler->TreeGlobal(tree))
    {
        data->CreateStore(data->CreateLoad(global), result);
    }

    return result;
}


bool OCompiledUnit::IsKnown(Tree *tree, uint which)
// ----------------------------------------------------------------------------
//   Check if the tree has a known local or global value
// ----------------------------------------------------------------------------
{
    if ((which & knowLocals) && storage.count(tree) > 0)
        return true;
    else if ((which & knowValues) && value.count(tree) > 0)
        return true;
    else if (which & knowGlobals)
        if (compiler->IsKnown(tree))
            return true;
    return false;
}


Value *OCompiledUnit::Known(Tree *tree, uint which)
// ----------------------------------------------------------------------------
//   Return the known local or global value if any
// ----------------------------------------------------------------------------
{
    Value *result = NULL;
    if ((which & knowLocals) && storage.count(tree) > 0)
    {
        // Value is stored in a local variable
        result = code->CreateLoad(storage[tree], "loc");
    }
    else if ((which & knowValues) && value.count(tree) > 0)
    {
        // Immediate value of some sort, use that
        result = value[tree];
    }
    else if (which & knowGlobals)
    {
        // Check if this is a global
        result = compiler->TreeGlobal(tree);
        if (result)
        {
            text label = "glob";
            IFTRACE(labels)
                label += "[" + text(*tree) + "]";
            result = code->CreateLoad(result, label);
        }
    }
    return result;
}


Value *OCompiledUnit::ConstantInteger(Integer *what)
// ----------------------------------------------------------------------------
//    Generate an Integer tree
// ----------------------------------------------------------------------------
{
    Value *result = Known(what, knowGlobals);
    if (!result)
    {
        result = compiler->EnterConstant(what);
        result = code->CreateLoad(result, "intk");
        if (storage.count(what))
            code->CreateStore(result, storage[what]);
    }
    return result;
}


Value *OCompiledUnit::ConstantReal(Real *what)
// ----------------------------------------------------------------------------
//    Generate a Real tree
// ----------------------------------------------------------------------------
{
    Value *result = Known(what, knowGlobals);
    if (!result)
    {
        result = compiler->EnterConstant(what);
        result = code->CreateLoad(result, "realk");
        if (storage.count(what))
            code->CreateStore(result, storage[what]);
    }
    return result;
}


Value *OCompiledUnit::ConstantText(Text *what)
// ----------------------------------------------------------------------------
//    Generate a Text tree
// ----------------------------------------------------------------------------
{
    Value *result = Known(what, knowGlobals);
    if (!result)
    {
        result = compiler->EnterConstant(what);
        result = code->CreateLoad(result, "textk");
    }
    if (storage.count(what))
        code->CreateStore(result, storage[what]);
    return result;
}


Value *OCompiledUnit::ConstantTree(Tree *what)
// ----------------------------------------------------------------------------
//    Generate a constant tree
// ----------------------------------------------------------------------------
{
    Value *result = Known(what, knowGlobals);
    if (!result)
    {
        result = compiler->EnterConstant(what);
        result = data->CreateLoad(result, "treek");
        if (storage.count(what))
            data->CreateStore(result, storage[what]);
    }
    return result;
}


Value *OCompiledUnit::NeedLazy(Tree *subexpr, bool allocate)
// ----------------------------------------------------------------------------
//   Record that we need a 'computed' flag for lazy evaluation of the subexpr
// ----------------------------------------------------------------------------
{
    Value *result = computed[subexpr];
    if (!result && allocate)
    {
        text label = "computed";
        IFTRACE(labels)
            label += "[" + text(*subexpr) + "]";

        result = data->CreateAlloca(LLVM_BOOLTYPE, 0, label.c_str());
        Value *falseFlag = ConstantInt::get(LLVM_BOOLTYPE, 0);
        data->CreateStore(falseFlag, result);
        computed[subexpr] = result;
    }
    return result;
}


llvm::Value *OCompiledUnit::MarkComputed(Tree *subexpr, Value *val)
// ----------------------------------------------------------------------------
//   Record that we computed that particular subexpression
// ----------------------------------------------------------------------------
{
    // Store the value we were given as the result
    if (val)
    {
        if (storage.count(subexpr) > 0)
            code->CreateStore(val, storage[subexpr]);
    }

    // Set the 'lazy' flag or lazy evaluation
    Value *result = NeedLazy(subexpr);
    Value *trueFlag = ConstantInt::get(LLVM_BOOLTYPE, 1);
    code->CreateStore(trueFlag, result);

    // Return the test flag
    return result;
}


BasicBlock *OCompiledUnit::BeginLazy(Tree *subexpr)
// ----------------------------------------------------------------------------
//    Begin lazy evaluation of a block of code
// ----------------------------------------------------------------------------
{
    text lskip = "skip";
    text lwork = "work";
    text llazy = "lazy";
    IFTRACE(labels)
    {
        text lbl = text("[") + text(*subexpr) + "]";
        lskip += lbl;
        lwork += lbl;
        llazy += lbl;
    }
    BasicBlock *skip = BasicBlock::Create(*llvm, lskip, function);
    BasicBlock *work = BasicBlock::Create(*llvm, lwork, function);

    Value *lazyFlagPtr = NeedLazy(subexpr);
    Value *lazyFlag = code->CreateLoad(lazyFlagPtr, llazy);
    code->CreateCondBr(lazyFlag, skip, work);

    code->SetInsertPoint(work);
    return skip;
}


void OCompiledUnit::EndLazy(Tree *subexpr,
                           llvm::BasicBlock *skip)
// ----------------------------------------------------------------------------
//   Finish lazy evaluation of a block of code
// ----------------------------------------------------------------------------
{
    (void) subexpr;
    code->CreateBr(skip);
    code->SetInsertPoint(skip);
}


llvm::Value *OCompiledUnit::Invoke(Tree *subexpr, Tree *callee, TreeList args)
// ----------------------------------------------------------------------------
//    Generate a call with the given arguments
// ----------------------------------------------------------------------------
{
    // Check if the resulting form is a name or literal
    if (callee->IsConstant())
    {
        if (Value *known = Known(callee))
        {
            MarkComputed(subexpr, known);
            return known;
        }
        else
        {
            std::cerr << "No value for xl_identity tree " << callee << '\n';
        }
    }

    Function *toCall = compiler->TreeFunction(callee); assert(toCall);

    // Add the context argument
    std::vector<Value *> argV;
    argV.push_back(contextPtr);

    // Add the 'self' argument
    Value *defaultVal = ConstantTree(subexpr);
    argV.push_back(defaultVal);

    TreeList::iterator a;
    for (a = args.begin(); a != args.end(); a++)
    {
        Tree *arg = *a;
        Value *value = Known(arg);
        if (!value)
            value = ConstantTree(arg);
        argV.push_back(value);
    }

    Value *callVal = code->CreateCall(toCall, argV.begin(), argV.end());

    // Store the flags indicating that we computed the value
    MarkComputed(subexpr, callVal);

    return callVal;
}


BasicBlock *OCompiledUnit::NeedTest()
// ----------------------------------------------------------------------------
//    Indicates that we need an exit basic block to jump to
// ----------------------------------------------------------------------------
{
    if (!failbb)
        failbb = BasicBlock::Create(*llvm, "fail", function);
    return failbb;
}


Value *OCompiledUnit::Left(Tree *tree)
// ----------------------------------------------------------------------------
//    Return the value for the left of the current tree
// ----------------------------------------------------------------------------
{
    // Check that the tree has the expected kind
    assert (tree->Kind() >= BLOCK);

    // Check if we already know the result, if so just return it
    // HACK: The following code assumes Prefix, Infix and Postfix have the
    // same layout for their pointers.
    Prefix *prefix = (Prefix *) tree;
    Value *result = Known(prefix->left);
    if (result)
        return result;

    // Check that we already have a value for the given tree
    Value *parent = Known(tree);
    if (parent)
    {
        Value *ptr = NeedStorage(prefix->left);

        // WARNING: This relies on the layout of all nodes beginning the same
        Value *pptr = code->CreateBitCast(parent, compiler->prefixTreePtrTy,
                                          "pfxl");
        result = code->CreateConstGEP2_32(pptr, 0,
                                          LEFT_VALUE_INDEX, "lptr");
        result = code->CreateLoad(result, "left");
        code->CreateStore(result, ptr);
    }
    else
    {
        Ooops("Internal: Using left of uncompiled $1", tree);
    }

    return result;
}


Value *OCompiledUnit::Right(Tree *tree)
// ----------------------------------------------------------------------------
//    Return the value for the right of the current tree
// ----------------------------------------------------------------------------
{
    // Check that the tree has the expected kind
    assert(tree->Kind() > BLOCK);

    // Check if we already known the result, if so just return it
    // HACK: The following code assumes Prefix, Infix and Postfix have the
    // same layout for their pointers.
    Prefix *prefix = (Prefix *) tree;
    Value *result = Known(prefix->right);
    if (result)
        return result;

    // Check that we already have a value for the given tree
    Value *parent = Known(tree);
    if (parent)
    {
        Value *ptr = NeedStorage(prefix->right);

        // WARNING: This relies on the layout of all nodes beginning the same
        Value *pptr = code->CreateBitCast(parent, compiler->prefixTreePtrTy,
                                          "pfxr");
        result = code->CreateConstGEP2_32(pptr, 0,
                                          RIGHT_VALUE_INDEX, "rptr");
        result = code->CreateLoad(result, "right");
        code->CreateStore(result, ptr);
    }
    else
    {
        Ooops("Internal: Using right of uncompiled $14", tree);
    }
    return result;
}


Value *OCompiledUnit::Copy(Tree *source, Tree *dest, bool markDone)
// ----------------------------------------------------------------------------
//    Copy data from source to destination
// ----------------------------------------------------------------------------
{
    Value *result = Known(source); assert(result);
    Value *ptr = NeedStorage(dest, source); assert(ptr);
    code->CreateStore(result, ptr);

    if (markDone)
    {
        // Set the target flag to 'done'
        Value *doneFlag = NeedLazy(dest);
        Value *trueFlag = ConstantInt::get(LLVM_BOOLTYPE, 1);
        code->CreateStore(trueFlag, doneFlag);
    }
    else if (Value *oldDoneFlag = NeedLazy(source, false))
    {
        // Copy the flag from the source
        Value *newDoneFlag = NeedLazy(dest);
        Value *computed = code->CreateLoad(oldDoneFlag);
        code->CreateStore(computed, newDoneFlag);
    }

    return result;
}


Value *OCompiledUnit::CallEvaluate(Tree *tree)
// ----------------------------------------------------------------------------
//   Call the evaluate function for the given tree
// ----------------------------------------------------------------------------
{
    Value *treeValue = Known(tree); assert(treeValue);
    if (dataForm.count(tree))
        return treeValue;

    Value *evaluated = code->CreateCall2(compiler->xl_evaluate,
                                         contextPtr, treeValue);
    MarkComputed(tree, evaluated);
    return evaluated;
}


Value *OCompiledUnit::CallFillBlock(Block *block)
// ----------------------------------------------------------------------------
//    Compile code generating the children of the block
// ----------------------------------------------------------------------------
{
    Value *blockValue = ConstantTree(block);
    Value *childValue = Known(block->child);
    blockValue = code->CreateBitCast(blockValue, compiler->blockTreePtrTy);
    Value *result = code->CreateCall2(compiler->xl_fill_block,
                                      blockValue, childValue);
    result = code->CreateBitCast(result, compiler->treePtrTy);
    MarkComputed(block, result);
    return result;
}


Value *OCompiledUnit::CallFillPrefix(Prefix *prefix)
// ----------------------------------------------------------------------------
//    Compile code generating the children of a prefix
// ----------------------------------------------------------------------------
{
    Value *prefixValue = ConstantTree(prefix);
    Value *leftValue = Known(prefix->left);
    Value *rightValue = Known(prefix->right);
    prefixValue = code->CreateBitCast(prefixValue, compiler->prefixTreePtrTy);
    Value *result = code->CreateCall3(compiler->xl_fill_prefix,
                                      prefixValue, leftValue, rightValue);
    result = code->CreateBitCast(result, compiler->treePtrTy);
    MarkComputed(prefix, result);
    return result;
}


Value *OCompiledUnit::CallFillPostfix(Postfix *postfix)
// ----------------------------------------------------------------------------
//    Compile code generating the children of a postfix
// ----------------------------------------------------------------------------
{
    Value *postfixValue = ConstantTree(postfix);
    Value *leftValue = Known(postfix->left);
    Value *rightValue = Known(postfix->right);
    postfixValue = code->CreateBitCast(postfixValue,compiler->postfixTreePtrTy);
    Value *result = code->CreateCall3(compiler->xl_fill_postfix,
                                      postfixValue, leftValue, rightValue);
    result = code->CreateBitCast(result, compiler->treePtrTy);
    MarkComputed(postfix, result);
    return result;
}


Value *OCompiledUnit::CallFillInfix(Infix *infix)
// ----------------------------------------------------------------------------
//    Compile code generating the children of an infix
// ----------------------------------------------------------------------------
{
    Value *infixValue = ConstantTree(infix);
    Value *leftValue = Known(infix->left);
    Value *rightValue = Known(infix->right);
    infixValue = code->CreateBitCast(infixValue, compiler->infixTreePtrTy);
    Value *result = code->CreateCall3(compiler->xl_fill_infix,
                                      infixValue, leftValue, rightValue);
    result = code->CreateBitCast(result, compiler->treePtrTy);
    MarkComputed(infix, result);
    return result;
}


Value *OCompiledUnit::CallArrayIndex(Tree *self, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Compile code calling xl_index for a form like A[B]
// ----------------------------------------------------------------------------
{
    Value *leftValue = Known(left);
    Value *rightValue = Known(right);
    Value *result = code->CreateCall3(compiler->xl_array_index,
                                      contextPtr, leftValue, rightValue);
    MarkComputed(self, result);
    return result;
}


Value *OCompiledUnit::CreateClosure(Tree *callee,
                                    TreeList &parms,
                                    TreeList &args,
                                    Function*fn)
// ----------------------------------------------------------------------------
//   Create a closure for an expression we want to evaluate later
// ----------------------------------------------------------------------------
{
    std::vector<Value *> argV;
    Value *calleeVal = Known(callee);
    if (!calleeVal)
        return NULL;
    Value *countVal = ConstantInt::get(LLVM_INTTYPE(uint), args.size());
    TreeList::iterator a, p;

    // Cast given function pointer to eval_fn and create argument list
    Value *evalFn = code->CreateBitCast(fn, compiler->evalFnTy);
    argV.push_back(evalFn);
    argV.push_back(calleeVal);
    argV.push_back(countVal);
    for (a = args.begin(), p = parms.begin();
         a != args.end() && p != parms.end();
         a++, p++)
    {
        Tree *name = *p;
        Value *llvmName = ConstantTree(name);
        argV.push_back(llvmName);

        Tree *value = *a;
        Value *llvmValue = Known(value); assert(llvmValue);
        argV.push_back(llvmValue);
    }

    Value *callVal = code->CreateCall(compiler->xl_new_closure,
                                      argV.begin(), argV.end());

    // Need to store result, but not mark it as evaluated
    NeedStorage(callee);
    code->CreateStore(callVal, storage[callee]);
    // MarkComputed(callee, callVal);

    return callVal;
}


Value *OCompiledUnit::CallClosure(Tree *callee, uint ntrees)
// ----------------------------------------------------------------------------
//   Call a closure function with the given n trees
// ----------------------------------------------------------------------------
//   We build it with an indirect call so that we generate one closure call
//   subroutine per number of arguments only.
//   The input is a sequence of infix \n that looks like:
//      P1 -> V1
//      P2 -> V2
//      P3 -> V3
//      [...]
//      E
//   where P1..Pn are the parameter names, V1..Vn their values, and E is the
//   original expression to evaluate.
//   The generated function takes the 'code' field of the last infix before E,
//   and calls it using C conventions with arguments (E, V1, V2, V3, ...)
{
    // Load left tree
    Type *treeTy = compiler->treePtrTy;
    Type *infixTy = compiler->infixTreePtrTy;
    Value *ptr = Known(callee); assert(ptr);
    Value *decl = NULL;

    // Build argument list
    std::vector<Value *> argV;
    std::vector<const Type *> signature;
    argV.push_back(contextPtr);   // Pass context pointer
    signature.push_back(compiler->contextPtrTy);
    argV.push_back(ptr);          // Self is the closure expression
    signature.push_back(treeTy);
    for (uint i = 0; i < ntrees; i++)
    {
        // Load the left of the \n which is a decl of the form P->V
        Value *infix = code->CreateBitCast(ptr, infixTy);
        Value *lf = code->CreateConstGEP2_32(infix, 0, LEFT_VALUE_INDEX);
        decl = code->CreateLoad(lf);
        decl = code->CreateBitCast(decl, infixTy);

        // Load the value V out of P->V and pass it as an argument
        Value *arg = code->CreateConstGEP2_32(decl, 0, RIGHT_VALUE_INDEX);
        arg = code->CreateLoad(arg);
        argV.push_back(arg);
        signature.push_back(treeTy);

        // Load the next element in the list
        Value *rt = code->CreateConstGEP2_32(infix, 0, RIGHT_VALUE_INDEX);
        ptr = code->CreateLoad(rt);
    }

    // Load the target code
    Value *callCode = code->CreateConstGEP2_32(decl, 0, CODE_INDEX);
    callCode = code->CreateLoad(callCode);

    // Replace the 'self' argument with the expression sans closure
    argV[1] = ptr;

    // Call the resulting function
    FunctionType *fnTy = FunctionType::get(treeTy, signature, false);
    PointerType *fnPtrTy = PointerType::get(fnTy, 0);
    Value *toCall = code->CreateBitCast(callCode, fnPtrTy);
    Value *callVal = code->CreateCall(toCall, argV.begin(), argV.end());

    // Store the flags indicating that we computed the value
    MarkComputed(callee, callVal);

    return callVal;
}


Value *OCompiledUnit::CallTypeError(Tree *what)
// ----------------------------------------------------------------------------
//   Report a type error trying to evaluate some argument
// ----------------------------------------------------------------------------
{
    Value *ptr = ConstantTree(what); assert(what);
    Value *callVal = code->CreateCall2(compiler->xl_form_error,
                                       contextPtr, ptr);
    MarkComputed(what, callVal);
    return callVal;
}


BasicBlock *OCompiledUnit::TagTest(Tree *tree, ulong tagValue)
// ----------------------------------------------------------------------------
//   Test if the input tree is an integer tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();

    // Check if the tag is INTEGER
    Value *treeValue = Known(tree);
    if (!treeValue)
    {
        Ooops("No value for $1", tree);
        return NULL;
    }
    Value *tagPtr = code->CreateConstGEP2_32(treeValue, 0, 0, "tagPtr");
    Value *tag = code->CreateLoad(tagPtr, "tag");
    Value *mask = ConstantInt::get(tag->getType(), Tree::KINDMASK);
    Value *kind = code->CreateAnd(tag, mask, "tagAndMask");
    Constant *refTag = ConstantInt::get(tag->getType(), tagValue);
    Value *isRightTag = code->CreateICmpEQ(kind, refTag, "isRightTag");
    BasicBlock *isRightKindBB = BasicBlock::Create(*llvm,
                                                   "isRightKind", function);
    code->CreateCondBr(isRightTag, isRightKindBB, notGood);

    code->SetInsertPoint(isRightKindBB);
    return isRightKindBB;
}


BasicBlock *OCompiledUnit::IntegerTest(Tree *tree, longlong value)
// ----------------------------------------------------------------------------
//   Test if the input tree is an integer tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();

    // Check if the tag is INTEGER
    BasicBlock *isIntegerBB = TagTest(tree, INTEGER);
    if (!isIntegerBB)
        return isIntegerBB;

    // Check if the value is the same
    Value *treeValue = Known(tree);
    assert(treeValue);
    treeValue = code->CreateBitCast(treeValue, compiler->integerTreePtrTy);
    Value *valueFieldPtr = code->CreateConstGEP2_32(treeValue, 0,
                                                    INTEGER_VALUE_INDEX);
    Value *tval = code->CreateLoad(valueFieldPtr, "treeValue");
    Constant *rval = ConstantInt::get(tval->getType(), value, "refValue");
    Value *isGood = code->CreateICmpEQ(tval, rval, "isGood");
    BasicBlock *isGoodBB = BasicBlock::Create(*llvm,
                                              "isGood", function);
    code->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code->SetInsertPoint(isGoodBB);
    return isGoodBB;
}


BasicBlock *OCompiledUnit::RealTest(Tree *tree, double value)
// ----------------------------------------------------------------------------
//   Test if the input tree is a real tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();

    // Check if the tag is REAL
    BasicBlock *isRealBB = TagTest(tree, REAL);
    if (!isRealBB)
        return isRealBB;

    // Check if the value is the same
    Value *treeValue = Known(tree);
    assert(treeValue);
    treeValue = code->CreateBitCast(treeValue, compiler->realTreePtrTy);
    Value *valueFieldPtr = code->CreateConstGEP2_32(treeValue, 0,
                                                       REAL_VALUE_INDEX);
    Value *tval = code->CreateLoad(valueFieldPtr, "treeValue");
    Constant *rval = ConstantFP::get(tval->getType(), value);
    Value *isGood = code->CreateFCmpOEQ(tval, rval, "isGood");
    BasicBlock *isGoodBB = BasicBlock::Create(*llvm,
                                              "isGood", function);
    code->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code->SetInsertPoint(isGoodBB);
    return isGoodBB;
}


BasicBlock *OCompiledUnit::TextTest(Tree *tree, text value)
// ----------------------------------------------------------------------------
//   Test if the input tree is a text tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();

    // Check if the tag is TEXT
    BasicBlock *isTextBB = TagTest(tree, TEXT);
    if (!isTextBB)
        return isTextBB;

    // Check if the value is the same, call xl_same_text
    Value *treeValue = Known(tree);
    assert(treeValue);
    Constant *refVal = ConstantArray::get(*llvm, value);
    const Type *refValTy = refVal->getType();
    GlobalVariable *gvar = new GlobalVariable(*compiler->module, refValTy, true,
                                              GlobalValue::InternalLinkage,
                                              refVal, "str");
    Value *refPtr = code->CreateConstGEP2_32(gvar, 0, 0);
    Value *isGood = code->CreateCall2(compiler->xl_same_text,
                                      treeValue, refPtr);
    BasicBlock *isGoodBB = BasicBlock::Create(*llvm, "isGood", function);
    code->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code->SetInsertPoint(isGoodBB);
    return isGoodBB;
}


BasicBlock *OCompiledUnit::ShapeTest(Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//   Test if the two given trees have the same shape
// ----------------------------------------------------------------------------
{
    Value *leftVal = Known(left);
    Value *rightVal = Known(right);
    assert(leftVal);
    assert(rightVal);
    if (leftVal == rightVal) // How unlikely?
        return NULL;

    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();
    Value *isGood = code->CreateCall2(compiler->xl_same_shape,
                                      leftVal, rightVal);
    BasicBlock *isGoodBB = BasicBlock::Create(*llvm, "isGood", function);
    code->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code->SetInsertPoint(isGoodBB);
    return isGoodBB;
}


BasicBlock *OCompiledUnit::InfixMatchTest(Tree *actual, Infix *reference)
// ----------------------------------------------------------------------------
//   Test if the actual tree has the same shape as the given infix
// ----------------------------------------------------------------------------
{
    // Check that we know how to evaluate both
    Value *actualVal = Known(actual);           assert(actualVal);
    Value *refVal = NeedStorage(reference);     assert (refVal);

    // Extract the name of the reference
    Constant *refNameVal = ConstantArray::get(*llvm, reference->name);
    const Type *refNameTy = refNameVal->getType();
    GlobalVariable *gvar = new GlobalVariable(*compiler->module,refNameTy,true,
                                              GlobalValue::InternalLinkage,
                                              refNameVal, "infix_name");
    Value *refNamePtr = code->CreateConstGEP2_32(gvar, 0, 0);

    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();
    Value *afterExtract = code->CreateCall3(compiler->xl_infix_match_check,
                                            contextPtr, actualVal, refNamePtr);
    Constant *null = ConstantPointerNull::get(compiler->treePtrTy);
    Value *isGood = code->CreateICmpNE(afterExtract, null, "isGoodInfix");
    BasicBlock *isGoodBB = BasicBlock::Create(*llvm, "isGood", function);
    code->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code->SetInsertPoint(isGoodBB);

    // We are on the right path: extract left and right
    code->CreateStore(afterExtract, refVal);
    MarkComputed(reference, NULL);
    MarkComputed(reference->left, NULL);
    MarkComputed(reference->right, NULL);
    Left(reference);
    Right(reference);

    return isGoodBB;
}


BasicBlock *OCompiledUnit::TypeTest(Tree *value, Tree *type)
// ----------------------------------------------------------------------------
//   Test if the given value has the given type
// ----------------------------------------------------------------------------
{
    Value *valueVal = Known(value);     assert(valueVal);
    Value *typeVal = Known(type);       assert(typeVal);

    // Where we go if the tests fail
    BasicBlock *notGood = NeedTest();
    Value *afterCast = code->CreateCall3(compiler->xl_type_check,
                                         contextPtr, valueVal, typeVal);
    Constant *null = ConstantPointerNull::get(compiler->treePtrTy);
    Value *isGood = code->CreateICmpNE(afterCast, null, "isGoodType");
    BasicBlock *isGoodBB = BasicBlock::Create(*llvm, "isGood", function);
    code->CreateCondBr(isGood, isGoodBB, notGood);

    // If the value matched, we may have a type cast, remember it
    code->SetInsertPoint(isGoodBB);
    Value *ptr = NeedStorage(value);
    code->CreateStore(afterCast, ptr);

    return isGoodBB;
}



// ============================================================================
//
//    Expression reduction
//
// ============================================================================
//   An expression reduction typically compiles as:
//     if (cond1) if (cond2) if (cond3) invoke(T)
//   However, we may determine during compilation of if(cond2) that the
//   call is statically not valid. So we save the initial basic block,
//   and decide at the end to connect it or not. Let LLVM optimize branches
//   and dead code away...

ExpressionReduction::ExpressionReduction(OCompiledUnit &u, Tree *src)
// ----------------------------------------------------------------------------
//    Snapshot current basic blocks in the compiled unit
// ----------------------------------------------------------------------------
    : unit(u), source(src), llvm(u.llvm),
      storage(NULL), computed(NULL),
      savedfailbb(NULL),
      entrybb(NULL), savedbb(NULL), successbb(NULL),
      savedvalue(u.value)
{
    // We need storage and a compute flag to skip this computation if needed
    storage = u.NeedStorage(src);
    computed = u.NeedLazy(src);

    // Save compile unit's data
    savedfailbb = u.failbb;
    u.failbb = NULL;

    // Create the end-of-expression point if we find a match or had it already
    successbb = u.BeginLazy(src);
}


ExpressionReduction::~ExpressionReduction()
// ----------------------------------------------------------------------------
//   Destructor restores the prevous sucess and fail points
// ----------------------------------------------------------------------------
{
    OCompiledUnit &u = unit;

    // Mark the end of a lazy expression evaluation
    u.EndLazy(source, successbb);

    // Restore saved 'failbb' and value map
    u.failbb = savedfailbb;
    u.value = savedvalue;
}


void ExpressionReduction::NewForm ()
// ----------------------------------------------------------------------------
//    Indicate that we are testing a new form for evaluating the expression
// ----------------------------------------------------------------------------
{
    OCompiledUnit &u = unit;

    // Save previous basic blocks in the compiled unit
    savedbb = u.code->GetInsertBlock();
    assert(savedbb || !"NewForm called after unconditional success");

    // Create entry / exit basic blocks for this expression
    entrybb = BasicBlock::Create(*llvm, "subexpr", u.function);
    u.failbb = NULL;

    // Set the insertion point to the new invokation code
    u.code->SetInsertPoint(entrybb);

}


void ExpressionReduction::Succeeded(void)
// ----------------------------------------------------------------------------
//   We successfully compiled a reduction for that expression
// ----------------------------------------------------------------------------
//   In that case, we connect the basic blocks to evaluate the expression
{
    OCompiledUnit &u = unit;

    // Branch from current point (end of expression) to exit of evaluation
    u.code->CreateBr(successbb);

    // Branch from initial basic block position to this subcase
    u.code->SetInsertPoint(savedbb);
    u.code->CreateBr(entrybb);

    // If there were tests, we keep testing from that 'else' spot
    if (u.failbb)
    {
        u.code->SetInsertPoint(u.failbb);
    }
    else
    {
        // Create a fake basic block in case someone decides to add code
        BasicBlock *empty = BasicBlock::Create(*llvm, "empty", u.function);
        u.code->SetInsertPoint(empty);
    }
    u.failbb = NULL;
}


void ExpressionReduction::Failed()
// ----------------------------------------------------------------------------
//    We figured out statically that the current form doesn't apply
// ----------------------------------------------------------------------------
{
    OCompiledUnit &u = unit;

    u.CallTypeError(source);
    u.code->CreateBr(successbb);
    if (u.failbb)
    {
        IRBuilder<> failTail(u.failbb);
        u.code->SetInsertPoint(u.failbb);
        u.CallTypeError(source);
        failTail.CreateBr(successbb);
        u.failbb = NULL;
    }

    u.code->SetInsertPoint(savedbb);
}




XL_END


extern "C" void debugsy(XL::Symbols *s)
// ----------------------------------------------------------------------------
//   For the debugger, dump a symbol table
// ----------------------------------------------------------------------------
{
    using namespace XL;
    std::cerr << "SYMBOLS AT " << s << "\n";
    std::cerr << "REWRITES IN " << s << ":\n";
    if (s->rewrites)
        debugrw(s->rewrites);

}


extern "C" void debugsym(XL::Symbols *symbols)
// ----------------------------------------------------------------------------
//   For the debugger, dump a symbol table
// ----------------------------------------------------------------------------
{
    using namespace XL;
    symbols_set visited;
    symbols_list lookups;

    // Build all the symbol tables that we are going to look into
    BuildSymbolsList(symbols, visited, lookups);

    for (symbols_list::iterator i = lookups.begin(); i != lookups.end(); i++)
    {
        Symbols *s = *i;

        debugsy(s);
        symbols_set::iterator import;
        for (import = s->imported.begin(); import!=s->imported.end(); import++)
        {
            std::cerr << "IMPORT " << *import << " IN " << s << ":\n";
            debugsy(*import);
        }
        s = s->parent;
    }
}
