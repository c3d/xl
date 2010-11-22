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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
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
        if (Tree *bound = context->Bound((Name *) what, Context::LOCAL_LOOKUP))
            what = bound;
        return what;
    case INFIX:
    {
        Infix *infix = (Infix *) what;
        if (infix->name == ":")
            return xl_evaluate_children(context, infix->left);
        Tree *left = xl_evaluate_children(context, infix->left);
        if (MAIN->HadErrors())
            return left;
        Tree *right = xl_evaluate_children(context, infix->right);
        if (MAIN->HadErrors())
            return right;
        if (left != infix->left || right != infix->right)
            infix = new Infix(infix, left, right);
        return infix;
    }
    case PREFIX:
    {
        Prefix *prefix = (Prefix *) what;
        Tree *left = xl_evaluate_children(context, prefix->left);
        if (MAIN->HadErrors())
            return left;
        Tree *right = xl_evaluate_children(context, prefix->right);
        if (MAIN->HadErrors())
            return right;
        if (left != prefix->left || right != prefix->right)
            prefix = new Prefix(prefix, left, right);
        return prefix;
    }
    case POSTFIX:
    {
        Postfix *postfix = (Postfix *) what;
        Tree *left = xl_evaluate_children(context, postfix->left);
        if (MAIN->HadErrors())
            return left;
        Tree *right = xl_evaluate_children(context, postfix->right);
        if (MAIN->HadErrors())
            return right;
        if (left != postfix->left || right != postfix->right)
            postfix = new Postfix(postfix, left, right);
        return postfix;
    }
    case BLOCK:
    {
        Block *block = (Block *) what;
        Tree *child = xl_evaluate_children(context, block->child);
        if (MAIN->HadErrors())
            return child;
        if (child != block->child)
            block = new Block(block, child);
        return block;
    }
    }
    return NULL;
}


Tree *xl_assigned_value(Context *context, Tree *value)
// ----------------------------------------------------------------------------
//   An assigned value is returned as is
// ----------------------------------------------------------------------------
{
    (void) context;
    return value;
}


Tree *xl_named_value(Context *context, Tree *value)
// ----------------------------------------------------------------------------
//   An named value is returned as is
// ----------------------------------------------------------------------------
{
    (void) context;
    return value;
}


struct SourceInfo : Info
// ----------------------------------------------------------------------------
//   Record the source for a given tree
// ----------------------------------------------------------------------------
{
    SourceInfo(Tree *source) : Info(), source(source) {}
    Tree_p source;
};


Tree *xl_source(Tree *value)
// ----------------------------------------------------------------------------
//   Return the source for the given value
// ----------------------------------------------------------------------------
{
    while (SourceInfo *info = value->GetInfo<SourceInfo>())
        value = info->source;
    return value;
}


Tree *xl_set_source(Tree *value, Tree *source)
// ----------------------------------------------------------------------------
//   Set the source associated with the value (e.g. for integer->real casts)
// ----------------------------------------------------------------------------
{
    if (source != value)
        value->SetInfo<SourceInfo>(new SourceInfo(source));
    return value;
}


Tree *xl_error(text msg, Tree *a1, Tree *a2, Tree *a3)
// ----------------------------------------------------------------------------
//   The default runtime error message mechanism (if not overriden)
// ----------------------------------------------------------------------------
{
    static Name_p errorName = new Name("error");
    Tree *args = new Text(msg);
    if (a3)
        a2 = new Infix(",", a2, a3);
    if (a2)
        a1 = new Infix(",", a1, a2);
    if (a1)
        args = new Infix(",", args, a1);
    Tree *result = new Prefix(errorName, args);

    if (a1) a1 = FormatTreeForError(a1);
    if (a2) a2 = FormatTreeForError(a2);
    if (a3) a3 = FormatTreeForError(a3);
    Error err(msg, a1, a2, a3);
    MAIN->errors->Log(err);

    return result;
}


Tree *xl_form_error(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//   Raise an error if we have a form error
// ----------------------------------------------------------------------------
{
    bool quickExit = false;
    if (quickExit)
        return what;
    static Name_p errorName = new Name("error");
    static Text_p errorText = new Text("No form matches $1");
    Infix *args = new Infix(",", errorText, what, what->Position());
    Prefix *error = new Prefix(errorName, args, what->Position());
    return context->Evaluate(error);
}


Tree *xl_parse_tree(Context *context, Tree *tree)
// ----------------------------------------------------------------------------
//   Build a parse tree in the current context
// ----------------------------------------------------------------------------
{
    switch(tree->Kind())
    {
    case INTEGER:
    case REAL:
    case TEXT:
    case NAME:
        return tree;
    case INFIX:
    {
        Infix *infix = (Infix *) tree;
        Tree *left = xl_parse_tree(context, infix->left);
        Tree *right = xl_parse_tree(context, infix->right);
        Infix *result = new Infix(infix, left, right);
        return result;
    }
    case PREFIX:
    {
        Prefix *prefix = (Prefix *) tree;
        Tree *left = xl_parse_tree(context, prefix->left);
        Tree *right = xl_parse_tree(context, prefix->right);
        Prefix *result = new Prefix(prefix, left, right);
        return result;
    }
    case POSTFIX:
    {
        Postfix *postfix = (Postfix *) tree;
        Tree *left = xl_parse_tree(context, postfix->left);
        Tree *right = xl_parse_tree(context, postfix->right);
        Postfix *result = new Postfix(postfix, left, right);
        return result;
    }
    case BLOCK:
    {
        Block *block = (Block *) tree;
        Tree *result = block->child;
        if (block->opening == "{" && block->closing == "}")
        {
            Block *child = result->AsBlock();
            if (child && child->opening == "{" && child->closing == "}")
            {
                // Case where we have parse_tree {{x}}: Return {x}
                result = xl_parse_tree(context, child->child);
                result = new Block(block, result);
                return result;
            }

            // Name or expression in { }
            if (Name *name = result->AsName())
                if (Tree *bound = context->Bound(name))
                    return bound;
            result = context->Evaluate(result);
            return result;
        }
        result = xl_parse_tree(context, result);
        result = new Block(block, result);
        return result;
    }
    }
    return tree;
}


Tree *xl_bound(Context *context, Name *name)
// ----------------------------------------------------------------------------
//   Return the bound value for a name or nil
// ----------------------------------------------------------------------------
{
    if (Tree *bound = context->Bound(name))
        return bound;
    return XL::xl_nil;
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
//   Any input tree can be accepted
// ----------------------------------------------------------------------------
{
    return value;
}


Tree *xl_source_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Any source can be accepted
// ----------------------------------------------------------------------------
{
    return value;
}


Tree *xl_code_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Any code can be accepted
// ----------------------------------------------------------------------------
{
    return value;
}


Tree *xl_lazy_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Any lazy argument can be accepted
// ----------------------------------------------------------------------------
{
    return value;
}


Tree *xl_value_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Any value can be accepted (we evaluate first)
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


Tree *xl_name_cast(Context *context, Tree *source, Tree *value)
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


Tree *xl_operator_cast(Context *context, Tree *source, Tree *value)
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

Tree *xl_parameter(text symbol, text type)
// ----------------------------------------------------------------------------
//   Build an infix name:type, except if type is "source"
// ----------------------------------------------------------------------------
{
    Name *n = new Name(symbol);
    if (type == lazy_type->value)
        return n;
    Name *t = new Name(type);
    Infix *i = new Infix(":", n, t);
    return i;
}


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

void xl_enter_builtin(Main *main, text name, Tree *to, TreeList parms)
// ----------------------------------------------------------------------------
//   Add a new builtin
// ----------------------------------------------------------------------------
{
    main->compiler->EnterBuiltin(name, to, parms, NULL);
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

    // Evaluate tree (and get rid of possible closures)
    tree = context->Evaluate(tree);

    // Format contents
    if (Infix *infix = tree->AsInfix())
    {
        if (infix->name == ",")
        {
            xl_write(context, infix->left, "");
            return xl_write(context, infix->right, sep);
        }
    }

    // Evaluate the input argument
    switch(tree->Kind())
    {
    case INTEGER: std::cout << ((Integer *) tree)->value << sep; break;
    case REAL:    std::cout << ((Real *) tree)->value << sep; break;
    case TEXT:    std::cout << ((Text *) tree)->value << sep; break;
    default:      std::cout << tree << sep; break;
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

    IFTRACE(fileload)
        std::cout << "Loading: " << path << "\n";

    Parser parser(path.c_str(), MAIN->syntax, MAIN->positions, *MAIN->errors);
    Tree *tree = parser.Parse();
    if (!tree)
        return Ooops("Unable to load file $1", new Text(path));

    Context *imported = new Context(context, NULL);
    MAIN->files[path] = SourceFile(path, tree, imported);
    context->imported.insert(imported);

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

    char  buffer[256];
    char   *ptr       = buffer;
    char   *end       = buffer + sizeof(buffer) - 1;
    Tree_p  tree      = NULL;
    Tree_p  line      = NULL;
    Tree_p *treePtr   = &tree;
    Tree_p *linePtr   = &line;
    bool    hasQuote  = false;
    bool    hasRecord = false;
    bool    hasField  = false;
    FILE   *f         = fopen(path.c_str(), "r");

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

            // Combine data that we just read to current line
            if (*linePtr)
            {
                Infix *infix = new Infix(",", *linePtr, child);
                *linePtr = infix;
                linePtr = &infix->right;
            }
            else
            {
                *linePtr = child;
            }

            // Combine as line if necessary
            if (hasRecord)
            {
                if (prefix != "")
                    line = new Prefix(new Name(prefix), line);
                if (*treePtr)
                {
                    Infix *infix = new Infix("\n", *treePtr, line);
                    *treePtr = infix;
                    treePtr = &infix->right;
                }
                else
                {
                    *treePtr = child;
                }
                line = NULL;
                linePtr = &line;
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
    Context *imported = new Context(context, NULL);
    MAIN->files[path] = SourceFile(path, tree, imported);
    context->imported.insert(imported);

    return tree;
}

XL_END
