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

#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <sys/stat.h>

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


XL_BEGIN

Tree *xl_identity(Tree *what)
// ----------------------------------------------------------------------------
//   Return the input tree unchanged
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *xl_evaluate(Tree *what)
// ----------------------------------------------------------------------------
//   Compile the tree if necessary, then evaluate it
// ----------------------------------------------------------------------------
// This is similar to Context::Run, but we save stack space for recursion
{
    if (!what)
        return what;
    Symbols *symbols = what->Get<SymbolsInfo>();
    if (!symbols)
        symbols = Symbols::symbols;
    return symbols->Run(what);
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


Tree *xl_infix_match_check(Tree *value, Infix *ref)
// ----------------------------------------------------------------------------
//   Check if the value is matching the given infix
// ----------------------------------------------------------------------------
{
    while (Block *block = value->AsBlock())
        if (block->opening == "(" && block->closing == ")")
            value = block->child;
    if (Infix *infix = value->AsInfix())
        if (infix->name == ref->name)
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
        return false;
    }

    // Check if this is a closure or something we want to evaluate
    if (!value->IsConstant() && value->code)
        value = value->code(value);

    Infix *typeExpr = Symbols::symbols->CompileTypeTest(type);
    typecheck_fn typecheck = (typecheck_fn) typeExpr->code;
    Tree *afterTypeCast = typecheck(typeExpr, value);
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
    result->code = xl_identity;
    return result;
}


Tree *xl_new_prefix(Prefix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Prefix
// ----------------------------------------------------------------------------
{
    Tree *result = new Prefix(source, left, right);
    result->code = xl_identity;
    return result;
}


Tree *xl_new_postfix(Postfix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Postfix
// ----------------------------------------------------------------------------
{
    Tree *result = new Postfix(source, left, right);
    result->code = xl_identity;
    return result;
}


Tree *xl_new_infix(Infix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Infix
// ----------------------------------------------------------------------------
{
    Tree *result = new Infix(source, left, right);
    result->code = xl_identity;
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
    Compiler * compiler = Context::context->compiler;
    eval_fn fn = compiler->closures[ntrees];
    if (!fn)
    {
        tree_list noParms;
        CompiledUnit unit(compiler, result, noParms);
        unit.CallClosure(result, ntrees);
        fn = unit.Finalize();
        compiler->closures[ntrees] = fn;
    }
    result->code = fn;
    compiler->closet.insert(result);

    return result;
}


Tree *xl_type_error(Tree *what)
// ----------------------------------------------------------------------------
//   Display message if we have a type error
// ----------------------------------------------------------------------------
{
    return Ooops("No form matches '$1'", what);
}



// ========================================================================
//
//    Type matching
//
// ========================================================================

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


Tree *xl_symbolicname_cast(Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a name
// ----------------------------------------------------------------------------
{
    if (Name *nt = value->AsName())
        return nt;
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


Tree *xl_invoke(eval_fn toCall, Tree *src, uint numargs, Tree **args)
// ----------------------------------------------------------------------------
//   Invoke a callback with the right number of arguments
// ----------------------------------------------------------------------------
//   We generate a function with the right signature using LLVM
{
    Compiler * compiler = Context::context->compiler;
    adapter_fn fn = compiler->EnterArrayToArgsAdapter(numargs);
    return fn (toCall, src, args);
}


Tree *xl_call(text name)
// ----------------------------------------------------------------------------
//   Invoke the tree with the given name
// ----------------------------------------------------------------------------
{
    tree_list args;
    Tree *callee = Symbols::symbols->CompileCall(name, args);
    if (callee && callee->code)
        callee = callee->code(callee);
    return callee;
}


Tree *xl_call(text name, double x, double y, double w, double h)
// ----------------------------------------------------------------------------
//   Invoke the tree with the given name
// ----------------------------------------------------------------------------
{
    tree_list args;
    args.push_back(new Real(x));
    args.push_back(new Real(y));
    args.push_back(new Real(w));
    args.push_back(new Real(h));
    Tree *callee = Symbols::symbols->CompileCall(name, args);
    if (callee && callee->code)
        callee = callee->code(callee);
    return callee;
}


Tree *xl_call(text name, double x, double y)
// ----------------------------------------------------------------------------
//   Invoke the tree with the given name
// ----------------------------------------------------------------------------
{
    tree_list args;
    args.push_back(new Real(x));
    args.push_back(new Real(y));
    Tree *callee = Symbols::symbols->CompileCall(name, args);
    if (callee && callee->code)
        callee = callee->code(callee);
    return callee;
}


Tree *xl_call(text name, text arg)
// ----------------------------------------------------------------------------
//   Invoke the tree with the given name
// ----------------------------------------------------------------------------
{
    tree_list args;
    args.push_back(new Text(arg));
    Tree *callee = Symbols::symbols->CompileCall(name, args);
    if (callee && callee->code)
        callee = callee->code(callee);
    return callee;
}


Tree *xl_load(text name)
// ----------------------------------------------------------------------------
//    Load a file from disk
// ----------------------------------------------------------------------------
{
    // Check if the file has already been loaded somehwere.
    // If so, return the loaded file
    if (MAIN->files.count(name) > 0)
    {
        SourceFile &sf = MAIN->files[name];
        Symbols::symbols->Import(sf.symbols);
        return sf.tree.tree;
    }

    Parser parser(name.c_str(), MAIN->syntax, MAIN->positions, MAIN->errors);
    Tree *tree = parser.Parse();
    if (!tree)
        return NULL;

    Symbols *old = Symbols::symbols;
    Symbols *syms = new Symbols(Context::context);
    MAIN->files[name] = SourceFile(name, tree, syms);
    Symbols::symbols = syms;
    tree->Set<SymbolsInfo>(syms);
    tree = syms->CompileAll(tree);
    Symbols::symbols = old;
    old->Import(syms);

    return tree;
}

Tree *xl_load_csv(text name)
// ----------------------------------------------------------------------------
//    Load a comma-separated file from disk
// ----------------------------------------------------------------------------
{
    // Check if the file has already been loaded somehwere.
    // If so, return the loaded file
    if (MAIN->files.count(name) > 0)
    {
        SourceFile &sf = MAIN->files[name];
        Symbols::symbols->Import(sf.symbols);
        return sf.tree.tree;
    }

    Tree *tree = NULL;
    Tree *line = NULL;
    char buffer[256];
    char *ptr = buffer;
    char *end = buffer + sizeof(buffer) - 1;
    bool has_quote = false;
    bool has_nl = false;
    FILE *f = fopen(name.c_str(), "r");

    *end = 0;
    while (!feof(f))
    {
        int c = getc(f);

        if (c == 0xA0)          // Hard space generated by Numbers, skip
            continue;

        has_nl = c == '\n' || c == EOF;
        if (has_nl || (c == ',' && !has_quote))
            c = 0;
        else if (c == '"')
            has_quote = !has_quote;

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
            if (has_nl)
            {
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
        return NULL;

    // Store that we use the file
    struct stat st;
    stat(name.c_str(), &st);
    Symbols *old = Symbols::symbols;
    Symbols *syms = new Symbols(Context::context);
    MAIN->files[name] = SourceFile(name, tree, syms);
    Symbols::symbols = syms;
    tree->Set<SymbolsInfo>(syms);
    tree = syms->CompileAll(tree);
    Symbols::symbols = old;
    old->Import(syms);

    return tree;
}


Tree *xl_load_tsv(text name)
// ----------------------------------------------------------------------------
//    Load a tab-separated file from disk
// ----------------------------------------------------------------------------
{
    // Check if the file has already been loaded somehwere.
    // If so, return the loaded file
    if (MAIN->files.count(name) > 0)
    {
        SourceFile &sf = MAIN->files[name];
        Symbols::symbols->Import(sf.symbols);
        return sf.tree.tree;
    }

    Tree *tree = NULL;
    Tree *line = NULL;
    char buffer[256];
    char *ptr = buffer;
    char *end = buffer + sizeof(buffer) - 1;
    bool has_nl = false;
    FILE *f = fopen(name.c_str(), "r");

    *end = 0;
    while (!feof(f))
    {
        int c = getc(f);

        if (c == 0xA0)          // Hard space generated by Numbers, skip
            continue;

        has_nl = c == '\n' || c == EOF;
        if (has_nl || c == '\t')
            c = 0;

        if (ptr < end)
            *ptr++ = c;

        if (!c)
        {
            text token;
            Tree *child = NULL;
            if (ptr > buffer + 1 && buffer[0] == '"' && ptr[-2] == '"')
            {
                token = text(buffer+1, ptr-buffer-2);
                child = new Text(token);
            }
            else if (isdigit(buffer[0]))
            {
                char *ptr2 = NULL;
                longlong l = strtoll(buffer, &ptr2, 10);
                if (ptr2 == ptr-1 || *ptr2 == '%')
                {
                    child = new Integer(l);
                    if (*ptr2 == '%')
                        child = new Postfix (child, new Name ("%"));
                }
                else
                {
                    double d = strtod (buffer, &ptr2);
                    if (ptr2 == ptr-1 || *ptr2 == '%')
                    {
                        child = new Real(d);
                        if (*ptr2 == '%')
                            child = new Postfix (child, new Name ("%"));
                    }
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
            if (has_nl)
            {
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
        return NULL;

    // Store that we use the file
    struct stat st;
    stat(name.c_str(), &st);
    Symbols *old = Symbols::symbols;
    Symbols *syms = new Symbols(Context::context);
    MAIN->files[name] = SourceFile(name, tree, syms);
    Symbols::symbols = syms;
    tree->Set<SymbolsInfo>(syms);
    tree = syms->CompileAll(tree);
    Symbols::symbols = old;
    old->Import(syms);

    return tree;
}

XL_END
