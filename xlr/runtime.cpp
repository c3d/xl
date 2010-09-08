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
#include "types.h"

#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <sys/stat.h>


XL_BEGIN

Tree *xl_evaluate(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//   Compile the tree if necessary, then evaluate it
// ----------------------------------------------------------------------------
// This is similar to Context::Run, but we save stack space for recursion
{
    return context->Evaluate(what);
}


Tree *xl_evaluate_children(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//   Evaluate the children for a given node
// ----------------------------------------------------------------------------
{
    switch(what->Kind())
    {
    case INTEGER:
    case REAL:
    case TEXT:
        return what;
    case NAME:
        if (Tree *bound = context->Bound((Name *) what, false))
            return context->Evaluate(bound);
        return what;
    case INFIX:
    {
        Infix *infix = (Infix *) what;
        Tree *left = xl_evaluate_children(context, infix->left);
        Tree *right = xl_evaluate_children(context, infix->right);
        Infix *result = new Infix(infix, left, right);
        return result;
    }
    case PREFIX:
    {
        Prefix *prefix = (Prefix *) what;
        Tree *left = xl_evaluate_children(context, prefix->left);
        Tree *right = xl_evaluate_children(context, prefix->right);
        Prefix *result = new Prefix(prefix, left, right);
        return result;
    }
    case POSTFIX:
    {
        Postfix *postfix = (Postfix *) what;
        Tree *left = xl_evaluate_children(context, postfix->left);
        Tree *right = xl_evaluate_children(context, postfix->right);
        Postfix *result = new Postfix(postfix, left, right);
        return result;
    }
    case BLOCK:
    {
        Block *block = (Block *) what;
        Tree *child = xl_evaluate_children(context, block->child);
        Block *result = new Block(block, child);
        return result;
    }
    }
    return NULL;
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


Tree *xl_form_error(Tree *what)
// ----------------------------------------------------------------------------
//   Display message if we have a type error
// ----------------------------------------------------------------------------
{
    bool quickExit = false;
    if (quickExit)
        return what;
    return Ooops("No form matches $1", what);
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
    return EqualTrees(left, right);
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


Tree *xl_type_check(Context *context, Tree *value, Tree *type)
// ----------------------------------------------------------------------------
//   Check if value has the type of 'type'
// ----------------------------------------------------------------------------
{
    IFTRACE(typecheck)
        std::cerr << "Type check " << value << " against " << type << ':';

    if (Tree *works = ValueMatchesType(context, value, type, true))
    {
        IFTRACE(typecheck)
            std::cerr << "Success\n";
        return works;
    }

    IFTRACE(typecheck)
        std::cerr << "Failed (mismatch)\n";

    return NULL;
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
    return result;
}


Tree *xl_new_real(double value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Real
// ----------------------------------------------------------------------------
{
    Tree *result = new Real (value);
    return result;
}


Tree *xl_new_character(kstring value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new single-quoted Text
// ----------------------------------------------------------------------------
{
    Tree *result = new Text(value, "'", "'");
    return result;
}


Tree *xl_new_text(kstring value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new double-quoted Text
// ----------------------------------------------------------------------------
{
    Tree *result = new Text(text(value));
    return result;
}


Tree *xl_new_xtext(kstring value, kstring open, kstring close)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new arbitrarily-quoted Text
// ----------------------------------------------------------------------------
{
    Tree *result = new Text(value, open, close);
    return result;
}


Tree *xl_new_block(Block *source, Tree *child)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new block
// ----------------------------------------------------------------------------
{
    Tree *result = new Block(source, child);
    return result;
}


Tree *xl_new_prefix(Prefix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Prefix
// ----------------------------------------------------------------------------
{
    Tree *result = new Prefix(source, left, right);
    return result;
}


Tree *xl_new_postfix(Postfix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Postfix
// ----------------------------------------------------------------------------
{
    Tree *result = new Postfix(source, left, right);
    return result;
}


Tree *xl_new_infix(Infix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Infix
// ----------------------------------------------------------------------------
{
    Tree *result = new Infix(source, left, right);
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
    if (!expr || expr->IsConstant() || !ntrees)
        return expr;

    IFTRACE(closure)
        std::cerr << "CLOSURE: Arity " << ntrees
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

    return result;
}



// ============================================================================
//
//    Type matching
//
// ============================================================================

#pragma GCC diagnostic ignored "-Wunused-parameter"

Tree *xl_boolean_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a boolean value (true/false)
// ----------------------------------------------------------------------------
{
    value = xl_evaluate(context, value);
    if (value == xl_true || value == xl_false)
        return value;
    return NULL;
}


Tree *xl_integer_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an integer
// ----------------------------------------------------------------------------
{
    value = xl_evaluate(context, value);
    if (Integer *it = value->AsInteger())
        return it;
    return NULL;
}


Tree *xl_real_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a real
// ----------------------------------------------------------------------------
{
    value = xl_evaluate(context, value);
    if (Real *rt = value->AsReal())
        return rt;
    if (Integer *it = value->AsInteger())
        return new Real(it->value);
    return NULL;
}


Tree *xl_text_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a text
// ----------------------------------------------------------------------------
{
    value = xl_evaluate(context, value);
    if (Text *tt = value->AsText())
        if (tt->opening != "'")
            return tt;
    return NULL;
}


Tree *xl_character_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a character
// ----------------------------------------------------------------------------
{
    value = xl_evaluate(context, value);
    if (Text *tt = value->AsText())
        if (tt->opening == "'")
            return tt;
    return NULL;
}


Tree *xl_tree_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Don't really check the argument
// ----------------------------------------------------------------------------
{
    return value;
}


Tree *xl_symbol_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a symbol
// ----------------------------------------------------------------------------
{
    if (Name *nt = value->AsName())
        return nt;
    value = xl_evaluate(context, value);
    if (Name *afterEval = value->AsName())
        return afterEval;
    return NULL;
}


Tree *xl_name_symbol_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a name
// ----------------------------------------------------------------------------
{
    if (Name *nt = value->AsName())
        if (nt->value.length() && isalpha(nt->value[0]))
            return nt;
    value = xl_evaluate(context, value);
    if (Name *afterEval = value->AsName())
        if (afterEval->value.length() && isalpha(afterEval->value[0]))
            return afterEval;
    return NULL;
}


Tree *xl_operator_symbol_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an operator symbol
// ----------------------------------------------------------------------------
{
    if (Name *nt = value->AsName())
        if (nt->value.length() && !isalpha(nt->value[0]))
            return nt;
    value = xl_evaluate(context, value);
    if (Name *afterEval = value->AsName())
        if (afterEval->value.length() && !isalpha(afterEval->value[0]))
            return afterEval;
    return NULL;
}


Tree *xl_infix_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an infix
// ----------------------------------------------------------------------------
{
    if (Infix *it = value->AsInfix())
        return it;
    return NULL;
}


Tree *xl_prefix_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a prefix
// ----------------------------------------------------------------------------
{
    if (Prefix *it = value->AsPrefix())
        return it;
    return NULL;
}


Tree *xl_postfix_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a postfix
// ----------------------------------------------------------------------------
{
    if (Postfix *it = value->AsPostfix())
        return it;
    return NULL;
}


Tree *xl_block_cast(Context *context, Tree *source, Tree *value)
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
//   Write a tree to standard output (temporary hack)
//
// ============================================================================

Tree *xl_write(Context *context, Tree *tree, text sep)
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
            xl_write(context, infix->left, "");
            xl_write(context, infix->right, sep);
            break;
        }
    }
    default:
    {
        // Evaluate argument
        Tree *value = context->Evaluate(tree);
        if (value != tree)
            return xl_write(context, value, sep);
        std::cout << "Unknown tree " << tree << sep;
        return XL::xl_false;
    }
    }

    return XL::xl_true;
}



// ============================================================================
//
//    Loading trees from external files
//
// ============================================================================

Tree *xl_load(Context *context, text name)
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
        return sf.tree;
    }

    IFTRACE(fileLoad)
        std::cout << "Loading: " << path << "\n";

    Parser parser(path.c_str(), MAIN->syntax, MAIN->positions, *MAIN->errors);
    Tree *tree = parser.Parse();
    if (!tree)
        return Ooops("Unable to load file $1", new Text(path));

    context = new Context(context);
    MAIN->files[path] = SourceFile(path, tree, context);

    return tree;
}


Tree *xl_import(Context *context, text name)
// ----------------------------------------------------------------------------
//   Load a tree and evaluate the result
// ----------------------------------------------------------------------------
{
    Tree *result = xl_load(context, name);
    result = context->Evaluate(result);
    return result;
}


Tree *xl_load_data(Context *context,
                   text name, text prefix, text fieldSeps, text recordSeps)
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
    context = new Context(context);
    MAIN->files[path] = SourceFile(path, tree, context);

    return tree;
}

XL_END
