// ****************************************************************************
//  runtime.cpp                     (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//     Runtime functions necessary to execute XL programs
//
//
//
//
//
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "runtime.h"
#include "tree.h"
#include "parser.h"
#include "scanner.h"
#include "renderer.h"
#include "context.h"
#include "options.h"
#include "opcodes.h"
#include "compiler.h"
#include "main.h"

#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <glob.h>
#include <sys/stat.h>


XL_BEGIN

Tree *xl_identity(Tree *what)
// ----------------------------------------------------------------------------
//   Return the input tree unchanged
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *xl_error(text msg, Tree *a1, Tree *a2, Tree *a3)
// ----------------------------------------------------------------------------
//   The default runtime error message mechanism (if not overriden)
// ----------------------------------------------------------------------------
{
    Error err(msg, a1, a2, a3);
    MAIN->errors->Log(err);
    return xl_false;
}


Tree *xl_evaluate(Tree *what)
// ----------------------------------------------------------------------------
//   Compile the tree if necessary, then evaluate it
// ----------------------------------------------------------------------------
// This is similar to Context::Run, but we save stack space for recursion
{
    if (!what)
        return what;
    Symbols *symbols = what->Symbols();
    if (!symbols)
        symbols = Symbols::symbols;

    StackDepthCheck depthCheck(what);
    if (depthCheck)
        return what;

    Tree *result = symbols->Run(what);
    if (result != what)
        result->source = xl_source(what);
    return result;
}


Tree *xl_repeat(Tree *self, Tree *what, longlong count)
// ----------------------------------------------------------------------------
//   Compile the tree if necessary, then evaluate it count times
// ----------------------------------------------------------------------------
{
    if (!what)
        return what;
    Symbols *symbols = self->Symbols();
    if (!symbols)
        symbols = Symbols::symbols;
    Tree *result = what;

    while (count-- > 0)
        result = symbols->Run(what);
    if (result != what)
        result->source = xl_source(what);
    return result;
}


bool xl_same_text(Tree *what, const char *ref)
// ----------------------------------------------------------------------------
//   Compile the tree if necessary, then evaluate it
// ----------------------------------------------------------------------------
{
    Text *tval = what->AsText(); assert(tval);
    return tval->value == text(ref);
}


bool xl_same_shape(Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//   Check equality of two trees
// ----------------------------------------------------------------------------
{
    if (right)
    {
        XL::TreeMatch compareForEquality(right);
        if (left && left->Do(compareForEquality))
            return true;
    }
    return false;
}


Tree *xl_infix_match_check(Tree *value, kstring name)
// ----------------------------------------------------------------------------
//   Check if the value is matching the given infix
// ----------------------------------------------------------------------------
{
    while (Block *block = value->AsBlock())
        if (block->opening == "(" && block->closing == ")")
            value = block->child;
    if (Infix *infix = value->AsInfix())
        if (infix->name == name)
            return infix;
    return NULL;
}


Tree *xl_type_check(Tree *value, Tree *type)
// ----------------------------------------------------------------------------
//   Check if value has the type of 'type'
// ----------------------------------------------------------------------------
{
    IFTRACE(typecheck)
        std::cerr << "Type check " << value << " against " << type << ':';
    if (!value || !type->code)
    {
        IFTRACE(typecheck)
            std::cerr << "Failed (no value / no code)\n";
        return NULL;
    }

    // Check if this is a closure or something we want to evaluate
    Tree *original = value;
    StackDepthCheck typeDepthCheck(value);
    if (typeDepthCheck)
        return NULL;

    Infix *typeExpr = Symbols::symbols->CompileTypeTest(type);
    typecheck_fn typecheck = (typecheck_fn) typeExpr->code;
    Tree *afterTypeCast = typecheck(typeExpr, value);
    if (afterTypeCast && afterTypeCast != original)
        xl_set_source(afterTypeCast, value);
    IFTRACE(typecheck)
    {
        if (afterTypeCast)
            std::cerr << "Success\n";
        else
            std::cerr << "Failed (not same type)\n";
    }
    return afterTypeCast;
}



// ========================================================================
//
//    Creating entities (callbacks for compiled code)
//
// ========================================================================

Tree *xl_new_integer(longlong value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Integer
// ----------------------------------------------------------------------------
{
    Tree *result = new Integer(value);
    result->code = xl_identity;
    return result;
}


Tree *xl_new_real(double value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Real
// ----------------------------------------------------------------------------
{
    Tree *result = new Real (value);
    result->code = xl_identity;
    return result;
}


Tree *xl_new_character(kstring value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new single-quoted Text
// ----------------------------------------------------------------------------
{
    Tree *result = new Text(value, "'", "'");
    result->code = xl_identity;
    return result;
}


Tree *xl_new_text(kstring value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new double-quoted Text
// ----------------------------------------------------------------------------
{
    Tree *result = new Text(text(value));
    result->code = xl_identity;
    return result;
}


Tree *xl_new_xtext(kstring value, kstring open, kstring close)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new arbitrarily-quoted Text
// ----------------------------------------------------------------------------
{
    Tree *result = new Text(value, open, close);
    result->code = xl_identity;
    return result;
}


Tree *xl_new_block(Block *source, Tree *child)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new block
// ----------------------------------------------------------------------------
{
    Tree *result = new Block(source, child);
    result->code = xl_evaluate_children;
    return result;
}


Tree *xl_new_prefix(Prefix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Prefix
// ----------------------------------------------------------------------------
{
    Tree *result = new Prefix(source, left, right);
    result->code = xl_evaluate_children;
    return result;
}


Tree *xl_new_postfix(Postfix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Postfix
// ----------------------------------------------------------------------------
{
    Tree *result = new Postfix(source, left, right);
    result->code = xl_evaluate_children;
    return result;
}


Tree *xl_new_infix(Infix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Infix
// ----------------------------------------------------------------------------
{
    Tree *result = new Infix(source, left, right);
    result->code = xl_evaluate_children;
    return result;
}



// ============================================================================
//
//    Closure management
//
// ============================================================================

Tree *xl_new_closure(Tree *expr, uint ntrees, ...)
// ----------------------------------------------------------------------------
//   Create a new closure at runtime, capturing the various trees
// ----------------------------------------------------------------------------
{
    // Immediately return anything we could evaluate at no cost
    if (!expr || expr->IsConstant() || !expr->code || !ntrees)
        return expr;

    IFTRACE(closure)
        std::cerr << "CLOSURE: Arity " << ntrees
                  << " code " << (void *) expr->code
                  << " [" << expr << "]\n";

    // Build the prefix with all the arguments
    Prefix *result = new Prefix(expr, NULL);
    Prefix *parent = result;
    va_list va;
    va_start(va, ntrees);
    for (uint i = 0; i < ntrees; i++)
    {
        Tree *arg = va_arg(va, Tree *);
        IFTRACE(closure)
            std::cerr << "  ARG: " << arg << '\n';
        Prefix *item = new Prefix(arg, NULL);
        parent->right = item;
        parent = item;
    }
    va_end(va);
    parent->right = xl_false;

    // Generate the code for the arguments
    Compiler *compiler = MAIN->compiler;
    eval_fn fn = compiler->closures[ntrees];
    if (!fn)
    {
        TreeList noParms;
        CompiledUnit unit(compiler, result, noParms);
        unit.CallClosure(result, ntrees);
        fn = unit.Finalize();
        compiler->closures[ntrees] = fn;
        compiler->SetTreeFunction(result, NULL); // Now owned by closures[n]
    }
    result->code = fn;
    xl_set_source(result, expr);

    return result;
}


Tree *xl_type_error(Tree *what)
// ----------------------------------------------------------------------------
//   Display message if we have a type error
// ----------------------------------------------------------------------------
{
    bool quickExit = false;
    if (quickExit)
        return what;
    Symbols *syms = what->Symbols();
    if (!syms)
        syms = Symbols::symbols;
    LocalSave<Symbols_p> saveSyms(Symbols::symbols, syms);
    return Ooops("No form matches $1", what);
}


Tree *xl_evaluate_children(Tree *what)
// ----------------------------------------------------------------------------
//   Reconstruct a similar tree evaluating children
// ----------------------------------------------------------------------------
{
    Tree *result = what;
    if (Symbols *s = what->Symbols())
    {
        EvaluateChildren eval(s);
        result = what->Do(eval);
        if (!result->Symbols())
            result->SetSymbols(what->Symbols());
    }
    return result;
}



// ============================================================================
//
//    Type matching
//
// ============================================================================

#pragma GCC diagnostic ignored "-Wunused-parameter"

Tree *xl_boolean_cast(Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a boolean value (true/false)
// ----------------------------------------------------------------------------
{
    value = xl_evaluate(value);
    if (value == xl_true || value == xl_false)
        return value;
    return NULL;
}


Tree *xl_integer_cast(Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an integer
// ----------------------------------------------------------------------------
{
    value = xl_evaluate(value);
    if (Integer *it = value->AsInteger())
        return it;
    return NULL;
}


Tree *xl_real_cast(Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a real
// ----------------------------------------------------------------------------
{
    value = xl_evaluate(value);
    if (Real *rt = value->AsReal())
        return rt;
    if (Integer *it = value->AsInteger())
        return new Real(it->value);
    return NULL;
}


Tree *xl_text_cast(Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a text
// ----------------------------------------------------------------------------
{
    value = xl_evaluate(value);
    if (Text *tt = value->AsText())
        if (tt->opening != "'")
            return tt;
    return NULL;
}


Tree *xl_character_cast(Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a character
// ----------------------------------------------------------------------------
{
    value = xl_evaluate(value);
    if (Text *tt = value->AsText())
        if (tt->opening == "'")
            return tt;
    return NULL;
}


Tree *xl_tree_cast(Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Don't really check the argument
// ----------------------------------------------------------------------------
{
    return value;
}


Tree *xl_symbol_cast(Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a symbol
// ----------------------------------------------------------------------------
{
    if (Name *nt = value->AsName())
        return nt;
    value = xl_evaluate(value);
    if (Name *afterEval = value->AsName())
        return afterEval;
    return NULL;
}


Tree *xl_name_symbol_cast(Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a name
// ----------------------------------------------------------------------------
{
    if (Name *nt = value->AsName())
        if (nt->value.length() && isalpha(nt->value[0]))
            return nt;
    value = xl_evaluate(value);
    if (Name *afterEval = value->AsName())
        if (afterEval->value.length() && isalpha(afterEval->value[0]))
            return afterEval;
    return NULL;
}


Tree *xl_operator_symbol_cast(Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an operator symbol
// ----------------------------------------------------------------------------
{
    if (Name *nt = value->AsName())
        if (nt->value.length() && !isalpha(nt->value[0]))
            return nt;
    value = xl_evaluate(value);
    if (Name *afterEval = value->AsName())
        if (afterEval->value.length() && !isalpha(afterEval->value[0]))
            return afterEval;
    return NULL;
}


Tree *xl_infix_cast(Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an infix
// ----------------------------------------------------------------------------
{
    if (Infix *it = value->AsInfix())
        return it;
    return NULL;
}


Tree *xl_prefix_cast(Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a prefix
// ----------------------------------------------------------------------------
{
    if (Prefix *it = value->AsPrefix())
        return it;
    return NULL;
}


Tree *xl_postfix_cast(Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a postfix
// ----------------------------------------------------------------------------
{
    if (Postfix *it = value->AsPostfix())
        return it;
    return NULL;
}


Tree *xl_block_cast(Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a block
// ----------------------------------------------------------------------------
{
    if (Block *it = value->AsBlock())
        return it;
    return NULL;
}



// ============================================================================
//
//   Animation utilities
//
// ============================================================================

Real *xl_springify(Real &value, Real &target, Real &time,
                   Real &damp, Real &kspring, Real &lt, Real &ls)
// ----------------------------------------------------------------------------
//   Add a "spring" effect to a value
// ----------------------------------------------------------------------------
{
    double distance = target.value - value.value;
    double t = time.value;
    double interval = t - lt.value;
    if (interval > 1.0)
        interval = 1.0;
    double v = value.value + ls.value * interval;
    double accel = kspring * distance;
    ls.value *= (1.0 - interval * damp.value);
    ls.value += accel * interval;
    lt.value = t;
    value.value = v;

    return &value;
}



// ============================================================================
//
//   Adding a builtin or a global
//
// ============================================================================

void xl_enter_builtin(Main *main, text name, Tree *to, TreeList parms,
                      eval_fn code)
// ----------------------------------------------------------------------------
//   Add a new builtin
// ----------------------------------------------------------------------------
{
    main->compiler->EnterBuiltin(name, to, parms, code);
}


void xl_enter_global(Main *main, Name *name, Name_p *address)
// ----------------------------------------------------------------------------
//   Add a new global
// ----------------------------------------------------------------------------
{
    main->compiler->EnterGlobal(name, address);
}


// ============================================================================
//
//   Managing calls to/from XL
//
// ============================================================================

Tree *XLCall::operator() (Symbols *syms, bool nullIfBad, bool cached)
// ----------------------------------------------------------------------------
//    Perform the given call in the given context
// ----------------------------------------------------------------------------
{
    assert(syms);
    Tree *callee = syms->CompileCall(name, args, nullIfBad, cached);
    if (callee && callee->code)
        callee = callee->code(callee);
    return callee;
}


Tree *XLCall::build(Symbols *syms)
// ----------------------------------------------------------------------------
//    Perform the given call in the given context
// ----------------------------------------------------------------------------
{
    assert(syms);
    Tree *callee = syms->CompileCall(name, args);
    return callee;
}


Tree *xl_invoke(eval_fn toCall, Tree *src, TreeList &args)
// ----------------------------------------------------------------------------
//   Invoke a callback with the right number of arguments
// ----------------------------------------------------------------------------
//   We generate a function with the right signature using LLVM
{
    Compiler *compiler = MAIN->compiler;
    adapter_fn fn = compiler->EnterArrayToArgsAdapter(args.size());

    // REVISIT: The following assumes that Tree_p and Tree * have the
    // same memory representation in a std::vector<Tree_p>
    return fn (toCall, src, (Tree **) &args[0]);
}



// ============================================================================
//
//   Write a tree to standard output (temporary hack)
//
// ============================================================================

Tree *xl_write(Symbols *symbols, Tree *tree, text sep)
// ----------------------------------------------------------------------------
//   Write elements of the tree to the console
// ----------------------------------------------------------------------------
{
    // Special case when we have no argument
    if (!tree)
    {
        std::cout << "NULL" << sep;
        return XL::xl_false;
    }

    // Format contents
    switch(tree->Kind())
    {
    case INTEGER: std::cout << tree->AsInteger()->value << sep; break;
    case REAL:    std::cout << tree->AsReal()->value << sep;    break;
    case TEXT:    std::cout << tree->AsText()->value << sep;    break;

    case INFIX:
    {
        Infix *infix = tree->AsInfix();
        if (infix->name == ",")
        {
            xl_write(symbols, infix->left, "");
            xl_write(symbols, infix->right, sep);
            break;
        }
    }
    default:
    {
        // Evaluate argument
        if (!tree->Symbols())
            tree->SetSymbols(symbols);
        Tree *result = xl_evaluate(tree);
        if (result != tree)
            return xl_write(symbols, result, sep);

        std::cout << "Unknown tree " << tree << sep;
        return XL::xl_false;
    }
    }

    return XL::xl_true;
}


static void xl_list_files(Tree *patterns, Tree_p *&parent)
// ----------------------------------------------------------------------------
//   Append all files found in the parent
// ----------------------------------------------------------------------------
{
    if (Block *block = patterns->AsBlock())
    {
        xl_list_files(block->child, parent);
        return;
    }
    if (Infix *infix = patterns->AsInfix())
    {
        if (infix->name == "," || infix->name == ";" || infix->name == "\n")
        {
            xl_list_files(infix->left, parent);
            xl_list_files(infix->right, parent);
            return;
        }
    }
    if (Text *regexp = patterns->AsText())
    {
        glob_t files;
        text filename = regexp->value;
        glob(filename.c_str(), GLOB_MARK, NULL, &files);
        for (uint i = 0; i < files.gl_pathc; i++)
        {
            std::string entry(files.gl_pathv[i]);
            Text *listed = new Text(entry);
            if (*parent)
            {
                Infix *added = new Infix(",", *parent, listed);
                *parent = added;
                parent = &added->right;
            }
            else
            {
                *parent = listed;
            }
        }
        globfree(&files);
        return;
    }
    Ooops("Malformed files list $1", patterns);
}


Tree *xl_list_files(Tree *patterns)
// ----------------------------------------------------------------------------
//   List all files in the given pattern
// ----------------------------------------------------------------------------
{
    Tree_p result = NULL;
    Tree_p *parent = &result;
    xl_list_files(patterns, parent);
    if (!result)
        result = xl_empty;
    return result;
}




// ============================================================================
//
//    Loading trees from external files
//
// ============================================================================

Tree *xl_load(text name)
// ----------------------------------------------------------------------------
//    Load a file from disk without evaluating it
// ----------------------------------------------------------------------------
{
    text path = MAIN->SearchFile(name);
    if (path == "")
        return Ooops("Source file $1 not found", new Text(name));

    // Check if the file has already been loaded somehwere.
    // If so, return the loaded file
    if (MAIN->files.count(path) > 0)
    {
        SourceFile &sf = MAIN->files[path];
        Symbols::symbols->Import(sf.symbols);
        return sf.tree;
    }

    if (MAIN->options.fileLoad)
        std::cout << "Loading: " << path << "\n";

    Parser parser(path.c_str(), MAIN->syntax, MAIN->positions, *MAIN->errors);
    Tree *tree = parser.Parse();
    if (!tree)
        return Ooops("Unable to load file $1", new Text(path));

    Symbols_p old = Symbols::symbols;
    Symbols_p syms = new Symbols(old);
    MAIN->files[path] = SourceFile(path, tree, syms);
    Symbols::symbols = syms;
    tree->SetSymbols(syms);
    tree = syms->CompileAll(tree);
    Symbols::symbols = old;
    old->Import(syms);

    return tree;
}


Tree *xl_import(text name)
// ----------------------------------------------------------------------------
//   Load a tree and evaluate the result
// ----------------------------------------------------------------------------
{
    Tree *result = xl_load(name);
    result = xl_evaluate(result);
    return result;
}


Tree *xl_load_data(text name, text prefix, text fieldSeps, text recordSeps)
// ----------------------------------------------------------------------------
//    Load a comma-separated or tab-separated file from disk
// ----------------------------------------------------------------------------
{
    text path = MAIN->SearchFile(name);
    if (path == "")
        return Ooops("CSV file $1 not found", new Text(name));

    // Check if the file has already been loaded somehwere.
    // If so, return the loaded file
    if (MAIN->files.count(path) > 0)
    {
        SourceFile &sf = MAIN->files[path];
        Symbols::symbols->Import(sf.symbols);
        return sf.tree;
    }

    Tree *tree = NULL;
    Tree *line = NULL;
    char buffer[256];
    char *ptr = buffer;
    char *end = buffer + sizeof(buffer) - 1;
    bool hasQuote = false;
    bool hasRecord = false;
    bool hasField = false;
    FILE *f = fopen(path.c_str(), "r");

    *end = 0;
    while (!feof(f))
    {
        int c = getc(f);

        if (c == 0xA0)          // Hard space generated by Numbers, skip
            continue;

        if (hasQuote)
        {
            hasRecord = false;
            hasField = false;
        }
        else
        {
            hasRecord = recordSeps.find(c) != recordSeps.npos || c == EOF;
            hasField = fieldSeps.find(c) != fieldSeps.npos;
        }
        if (hasRecord || hasField)
            c = 0;
        else if (c == '"')
            hasQuote = !hasQuote;

        if (ptr < end)
            *ptr++ = c;

        if (!c)
        {
            text token;
            Tree *child = NULL;

            if (isdigit(ptr[0]))
            {
                char *ptr2 = NULL;
                longlong l = strtoll(buffer, &ptr2, 10);
                if (ptr2 == ptr-1)
                {
                    child = new Integer(l);
                }
                else
                {
                    double d = strtod (buffer, &ptr2);
                    if (ptr2 == ptr-1)
                        child = new Real(d);
                }
            }
            if (child == NULL)
            {
                token = text(buffer, ptr - buffer - 1);
                child = new Text(buffer);
            }

            // Combine to line
            if (line)
                line = new Infix(",", line, child);
            else
                line = child;

            // Combine as line if necessary
            if (hasRecord)
            {
                if (prefix != "")
                    line = new Prefix(new Name(prefix), line);
                if (tree)
                    tree = new Infix("\n", tree, line);
                else
                    tree = line;
                line = NULL;
            }
            ptr = buffer;
        }
    }
    fclose(f);
    if (!tree)
        return Ooops("Unable to load data from $1", new Text(path));

    // Store that we use the file
    struct stat st;
    stat(path.c_str(), &st);
    Symbols_p old = Symbols::symbols;
    Symbols_p syms = new Symbols(old);
    MAIN->files[path] = SourceFile(path, tree, syms);
    Symbols::symbols = syms;
    tree->SetSymbols(syms);
    tree = syms->CompileAll(tree);
    Symbols::symbols = old;
    old->Import(syms);

    return tree;
}



// ============================================================================
//
//   Apply a code recursively to a data set
//
// ============================================================================

Tree *xl_apply(Tree *code, Tree *data)
// ----------------------------------------------------------------------------
//   Apply the input code on each piece of data
// ----------------------------------------------------------------------------
//   We deal with the following cases:
//   - Code is a name: We map it as a prefix to a single-argument function
//   - Code is in the form X->f(X): We map the right-hand side
//   - Code is in the form X,Y->f(X,Y): We reduce using the right-hand side
//   - Code is in the form X where f(X): We filter based on f(X)
{
    // Check if we got (1,2,3,4) or something like f(3) as 'data'
    Block *block = data->AsBlock();
    if (!block)
    {
        // We got f(3) or Hello as input: evaluate it
        data = xl_evaluate(data);

        // The returned data may itself be something like (1,2,3,4,5)
        block = data->AsBlock();
    }
    if (block)
    {
        // We got (1,2,3,4): Extract 1,2,3,4
        data = block->child;
        if (!data->Symbols())
            data->SetSymbols(block->Symbols());
        if (!data->code)
            data->code = xl_evaluate_children;
    }

    // Check if we already compiled that code
    FunctionInfo *fninfo = code->GetInfo<FunctionInfo>();
    if (!fninfo)
    {
        // Identify what operation we want to perform
        Tree *toCompile = code;
        TreeList parameters;
        bool reduce = false;
        bool filter = false;

        // For syntactic convenience, the code is generally in a block
        if (Block *codeBlock = toCompile->AsBlock())
        {
            toCompile = codeBlock->child;
            if (!toCompile->Symbols())
                toCompile->SetSymbols(codeBlock->Symbols());
        }

        // Define default data separators
        std::set<text> separators;
        separators.insert(",");
        separators.insert(";");
        separators.insert("\n");

        // Check the case where code is x->sin x  (map) or x,y->x+y (reduce)
        if (Infix *infix = toCompile->AsInfix())
        {
            Tree *ileft = infix->left;
            if (infix->name == "->")
            {
                // Case of x -> sin x
                if (Name *name = ileft->AsName())
                {
                    parameters.push_back(name);
                    toCompile = infix->right;
                }

                // Case of x,y -> x+y
                else if (Infix *op = ileft->AsInfix())
                {
                    // This defines the separator we use for data
                    separators.insert(op->name);

                    Name *first = op->left->AsName();
                    Name *second = op->right->AsName();
                    if (first && second)
                    {
                        parameters.push_back(first);
                        parameters.push_back(second);
                        reduce = true;
                        toCompile = infix->right;
                    }
                }
            }
            else if (infix->name == "where")
            {
                // Case of x where x < 3
                if (Name *name = ileft->AsName())
                {
                    parameters.push_back(name);
                    toCompile = infix->right;
                    filter = true;
                }
            }
        }
        else if (Name *name = code->AsName())
        {
            // We have a single name: consider it as a prefix to all elements
            Name *parameter = new Name("_");
            parameters.push_back(parameter);
            toCompile = new Prefix(name, parameter);
        }
        else
        {
            // OK, we don't know what to do with this stuff...
            return Ooops("Malformed map/reduce code $1", code);
        }

        // We have now decided what this is, so we compile the code
        Symbols *symbols = new Symbols(code->Symbols());
        eval_fn fn = NULL;

        // Record all the parameters in the symbol table
        for (TreeList::iterator p=parameters.begin(); p!=parameters.end(); p++)
            if (Name *parmName = (*p)->AsName())
                symbols->Allocate(parmName);

        // Create a compile unit with the right number of parameters
        Compiler *compiler = MAIN->compiler;
        CompiledUnit unit(compiler, toCompile, parameters);
        assert (!unit.IsForwardCall() || !"Forward call in map/reduce code");

        // Record internal declarations if any
        DeclarationAction declare(symbols);
        Tree *toDecl = toCompile->Do(declare);
        assert(toDecl);

        // Compile the body we generated
        CompileAction compile(symbols, unit, true, true);
        Tree *compiled = toCompile->Do(compile);

        // Generate code if compilation was successful
        if (compiled)
            fn = unit.Finalize();

        // Generate appropriate function info
        if (filter)
            fninfo = new FilterFunctionInfo;
        else if (reduce)
            fninfo = new ReduceFunctionInfo;
        else
            fninfo = new MapFunctionInfo;

        // Record generated code (or NULL in case of compilation failure)
        code->SetInfo<FunctionInfo>(fninfo);
        fninfo->function = fn;
        fninfo->symbols = symbols;
        fninfo->compiled = toCompile;
        fninfo->separators = separators;

        // Report compile error the first time
        if (!compiled)
            return Ooops("Cannot compile map/reduce code $1", code);

        if (!toCompile->code)
            toCompile->code = xl_evaluate_children;
    }

    LocalSave<Symbols_p> saveSyms(Symbols::symbols, code->Symbols());
    Tree *result = data;
    if (fninfo->function)
        result = fninfo->Apply(result);
    else
        result = Ooops("Invalid map/reduce code $1", code);
    return result;
}


Tree *xl_range(longlong low, longlong high)
// ----------------------------------------------------------------------------
//   Return a range of values between low and high
// ----------------------------------------------------------------------------
//   This is so ugly, but lazy evalation doesn't work quite right yet
{
    Tree *result = new Integer(low);
    for (longlong i = low+1; i <= high; i++)
        result = new Infix(",", result, new Integer(i));
    result->code = xl_identity;
    return result;
}


void xl_infix_to_list(Infix *infix, TreeList &list)
// ----------------------------------------------------------------------------
//   Convert an infix to a list, whether left or right associative
// ----------------------------------------------------------------------------
{
    Infix *left = infix->left->AsInfix();
    if (left && left->name == infix->name)
        xl_infix_to_list(left, list);
    else
        list.push_back(infix->left);
    
    Infix *right = infix->right->AsInfix();
    if (right && right->name == infix->name)
        xl_infix_to_list(right, list);
    else
        list.push_back(infix->right);
}
            

Tree *xl_nth(Tree *data, longlong index)
// ----------------------------------------------------------------------------
//   Find the nth element in a data set
// ----------------------------------------------------------------------------
{
    Tree *source = data;

    // Check if we got (1,2,3,4) or something like f(3) as 'data'
    Block *block = data->AsBlock();
    if (!block)
    {
        // We got f(3) or Hello as input: evaluate it
        data = xl_evaluate(data);

        // The returned data may itself be something like (1,2,3,4,5)
        block = data->AsBlock();
    }
    if (block)
    {
        // We got (1,2,3,4): Extract 1,2,3,4
        data = block->child;
        if (!data->Symbols())
            data->SetSymbols(block->Symbols());
        if (!data->code)
            data->code = xl_evaluate_children;
    }

    // Now loop on the top-level infix
    Tree *result = data;
    if (Infix *infix = result->AsInfix())
    {
        TreeList list;
        xl_infix_to_list(infix, list);
        if (index < 1 || index > (longlong) list.size())
            return Ooops("Index $2 for $1 out of range",
                         source, new Integer(index));
        result = list[index-1];
    }

    return result;
}



// ============================================================================
// 
//   Map an operation on all elements
// 
// ============================================================================

Tree *MapFunctionInfo::Apply(Tree *what)
// ----------------------------------------------------------------------------
//   Apply a map operation
// ----------------------------------------------------------------------------
{
    MapAction map(function, separators);
    return what->Do(map);
}


Tree *MapAction::Do(Tree *what)
// ----------------------------------------------------------------------------
//   Apply the code to the given tree
// ----------------------------------------------------------------------------
{
    if (!what->Symbols())
        what->SetSymbols(Symbols::symbols);
    what = xl_evaluate(what);
    return function(what, what);
}


Tree *MapAction::DoInfix(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if this is a separator, if so evaluate left and right
// ----------------------------------------------------------------------------
{
    if (separators.count(infix->name))
    {
        Tree *left = infix->left->Do(this);
        Tree *right = infix->right->Do(this);
        if (left != infix->left || right != infix->right)
        {
            infix = new Infix(infix->name, left, right, infix->Position());
            infix->code = xl_evaluate_children;
        }
        return infix;
    }

    // Otherwise simply apply the function to the infix
    return Do(infix);
}


Tree *MapAction::DoPrefix(Prefix *prefix)
// ----------------------------------------------------------------------------
//   Apply to the whole prefix (don't decompose)
// ----------------------------------------------------------------------------
{
    return Do(prefix);
}


Tree *MapAction::DoPostfix(Postfix *postfix)
// ----------------------------------------------------------------------------
//   Apply to the whole postfix (don't decompose)
// ----------------------------------------------------------------------------
{
    return Do(postfix);
}


Tree *MapAction::DoBlock(Block *block)
// ----------------------------------------------------------------------------
//   Apply to the whole block (don't decompose)
// ----------------------------------------------------------------------------
{
    return Do(block);
}



// ============================================================================
// 
//   Reduce by applying operations to consecutive elements
// 
// ============================================================================

Tree *ReduceFunctionInfo::Apply(Tree *what)
// ----------------------------------------------------------------------------
//   Apply a reduce operation to the tree
// ----------------------------------------------------------------------------
{
    ReduceAction reduce(function, separators);
    return what->Do(reduce);
}


Tree *ReduceAction::Do(Tree *what)
// ----------------------------------------------------------------------------
//   By default, reducing non-list elements returns these elements
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *ReduceAction::DoInfix(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if this is a separator, if so combine left and right
// ----------------------------------------------------------------------------
{
    if (separators.count(infix->name))
    {
        Tree *left = infix->left->Do(this);
        Tree *right = infix->right->Do(this);
        if (!infix->Symbols())
            infix->SetSymbols(Symbols::symbols);
        return function(infix, left, right);
    }

    // Otherwise simply apply the function to the infix
    return Do(infix);
}


Tree *ReduceAction::DoPrefix(Prefix *prefix)
// ----------------------------------------------------------------------------
//   Apply to the whole prefix (don't decompose)
// ----------------------------------------------------------------------------
{
    return Do(prefix);
}


Tree *ReduceAction::DoPostfix(Postfix *postfix)
// ----------------------------------------------------------------------------
//   Apply to the whole postfix (don't decompose)
// ----------------------------------------------------------------------------
{
    return Do(postfix);
}


Tree *ReduceAction::DoBlock(Block *block)
// ----------------------------------------------------------------------------
//   Apply to the whole block (don't decompose)
// ----------------------------------------------------------------------------
{
    return Do(block);
}


// ============================================================================
// 
//   Filter by selecting elements that match a given condition
// 
// ============================================================================

Tree *FilterFunctionInfo::Apply(Tree *what)
// ----------------------------------------------------------------------------
//   Apply a filter operation to the tree
// ----------------------------------------------------------------------------
{
    FilterAction filter(function, separators);
    Tree *result = what->Do(filter);
    if (!result)
        result = xl_false;
    return result;
}


Tree *FilterAction::Do(Tree *what)
// ----------------------------------------------------------------------------
//   By default, reducing non-list elements returns these elements
// ----------------------------------------------------------------------------
{
    if (!what->Symbols())
        what->SetSymbols(Symbols::symbols);
    if (function(what, what) == xl_true)
        return what;
    return NULL;
}


Tree *FilterAction::DoInfix(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if this is a separator, if so combine left and right
// ----------------------------------------------------------------------------
{
    if (separators.count(infix->name))
    {
        Tree *left = infix->left->Do(this);
        Tree *right = infix->right->Do(this);
        if (left && right)
        {
            infix = new Infix(infix->name, left, right, infix->Position());
            infix->code = xl_evaluate_children;
            return infix;
        }
        if (left)
            return left;
        return right;
    }

    // Otherwise simply apply the function to the infix
    return Do(infix);
}


Tree *FilterAction::DoPrefix(Prefix *prefix)
// ----------------------------------------------------------------------------
//   Apply to the whole prefix (don't decompose)
// ----------------------------------------------------------------------------
{
    return Do(prefix);
}


Tree *FilterAction::DoPostfix(Postfix *postfix)
// ----------------------------------------------------------------------------
//   Apply to the whole postfix (don't decompose)
// ----------------------------------------------------------------------------
{
    return Do(postfix);
}


Tree *FilterAction::DoBlock(Block *block)
// ----------------------------------------------------------------------------
//   Apply to the whole block (don't decompose)
// ----------------------------------------------------------------------------
{
    return Do(block);
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
        LocalSave<bool> overflow(in_error_handler, true);
        LocalSave<uint> depth(stack_depth, 1);
        Ooops("Stack overflow evaluating $1", what);
    }
}

XL_END
