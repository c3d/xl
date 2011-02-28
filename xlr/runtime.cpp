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
#include "symbols.h"
#include "options.h"
#include "opcodes.h"
#include "compiler.h"
#include "compiler-gc.h"
#include "main.h"
#include "types.h"
#include "save.h"

#include <iostream>
#include <cstdarg>
#include <cstdio>
#include "winglob.h"
#include <sys/stat.h>


XL_BEGIN

// In interpreted mode, the context actually holds the stack.
// However, in compiled mode, there's only one level of context,
// so attempting to unwind it ultimately causes a NULL-deref
#define ADJUST_CONTEXT_FOR_INTERPRETER(context) \
    if (Options::options->optimize_level == 0)  \
        context = context->stack;
#define ADJUST_CONTEXT_MUST_BE_IN_INTERPRETER(context)                  \
    assert (Options::options->optimize_level == 0 &&                    \
            "This routine should only be called in interpreted mode");  \
    context = context->stack;


Tree *xl_identity(Context *, Tree *what)
// ----------------------------------------------------------------------------
//   Return the input tree unchanged
// ----------------------------------------------------------------------------
{
    return what;
}


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
    Tree *result = what;
    if (Symbols *s = what->Symbols())
    {
        EvaluateChildren eval(context, s);
        result = what->Do(eval);
        if (!result->Symbols())
            result->SetSymbols(what->Symbols());
        return result;
    }

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
    bool quickExit = false;     // For debugging purpose
    if (quickExit)
        return what;
    ADJUST_CONTEXT_FOR_INTERPRETER(context);
    static Name_p errorName = new Name("error");
    static Text_p errorText = new Text("No form matches $1");
    Infix *args = new Infix(",", errorText, what, what->Position());
    Prefix *error = new Prefix(errorName, args, what->Position());
    Symbols *symbols = what->Symbols();
    if (!symbols) symbols = MAIN->globals;
    error->SetSymbols(symbols);
    return context->Evaluate(error);
}


Tree *xl_parse_tree_inner(Context *context, Tree *tree)
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
        Tree *left = xl_parse_tree_inner(context, infix->left);
        Tree *right = xl_parse_tree_inner(context, infix->right);
        Infix *result = new Infix(infix, left, right);
        return result;
    }
    case PREFIX:
    {
        Prefix *prefix = (Prefix *) tree;
        Tree *left = xl_parse_tree_inner(context, prefix->left);
        Tree *right = xl_parse_tree_inner(context, prefix->right);
        Prefix *result = new Prefix(prefix, left, right);
        return result;
    }
    case POSTFIX:
    {
        Postfix *postfix = (Postfix *) tree;
        Tree *left = xl_parse_tree_inner(context, postfix->left);
        Tree *right = xl_parse_tree_inner(context, postfix->right);
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
                result = xl_parse_tree_inner(context, child->child);
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
        result = xl_parse_tree_inner(context, result);
        result = new Block(block, result);
        return result;
    }
    }
    return tree;
}


Tree *xl_parse_tree(Context *context, Tree *code)
// ----------------------------------------------------------------------------
//   Entry point for parse_tree
// ----------------------------------------------------------------------------
{
    if (Block *block = code->AsBlock())
        code = block->child;
    ADJUST_CONTEXT_FOR_INTERPRETER(context);
    return xl_parse_tree_inner(context, code);
}


Tree *xl_bound(Context *context, Tree *form)
// ----------------------------------------------------------------------------
//   Return the bound value for a name/form, or nil if not bound
// ----------------------------------------------------------------------------
{
    // TODO: Equivalent in compiled mode
    ADJUST_CONTEXT_MUST_BE_IN_INTERPRETER(context);
    if (Tree *bound = context->Bound(form))
        return bound;
    return XL::xl_false;
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
    return Tree::Equal(left, right);
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

    // Check if we are using the old compiler
    if (Symbols *symbols = type->Symbols())
    {
        assert(value && "xl_type_check needs a valid value");

        // Check if this is a closure or something we want to evaluate
        Tree *original = value;
        StackDepthCheck typeDepthCheck(value);
        if (typeDepthCheck)
            return NULL;
        
        Infix *typeExpr = symbols->CompileTypeTest(type);
        assert(type->code && "xl_type_check needs compiled type check");
        typecheck_fn typecheck = (typecheck_fn) typeExpr->code;
        Tree *afterTypeCast = typecheck(context, typeExpr, value);
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

Integer *xl_new_integer(longlong value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Integer
// ----------------------------------------------------------------------------
{
    Integer *result = new Integer(value);
    return result;
}


Real *xl_new_real(double value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Real
// ----------------------------------------------------------------------------
{
    Real *result = new Real (value);
    return result;
}


Text *xl_new_character(char value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new single-quoted Text
// ----------------------------------------------------------------------------
{
    Text *result = new Text(text(value, 1), "'", "'");
    return result;
}


Text *xl_new_text(text value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new double-quoted Text
// ----------------------------------------------------------------------------
{
    Text *result = new Text(value);
    return result;
}


Text *xl_new_ctext(kstring value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new double-quoted Text
// ----------------------------------------------------------------------------
{
    Text *result = new Text(text(value));
    return result;
}


Text *xl_new_xtext(kstring value, longlong len, kstring open, kstring close)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new arbitrarily-quoted Text
// ----------------------------------------------------------------------------
{
    Text *result = new Text(text(value, len), open, close);
    return result;
}


Block *xl_new_block(Block *source, Tree *child)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new block
// ----------------------------------------------------------------------------
{
    Block *result = new Block(source, child);
    return result;
}


Prefix *xl_new_prefix(Prefix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Prefix
// ----------------------------------------------------------------------------
{
    Prefix *result = new Prefix(source, left, right);
    return result;
}


Postfix *xl_new_postfix(Postfix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Postfix
// ----------------------------------------------------------------------------
{
    Postfix *result = new Postfix(source, left, right);
    return result;
}


Infix *xl_new_infix(Infix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Infix
// ----------------------------------------------------------------------------
{
    Infix *result = new Infix(source, left, right);
    return result;
}



// ============================================================================
//
//    Closure management
//
// ============================================================================

Tree *xl_new_closure(eval_fn toCall, Tree *expr, uint ntrees, ...)
// ----------------------------------------------------------------------------
//   Create a new closure at runtime, capturing the various trees
// ----------------------------------------------------------------------------
{
    // Immediately return anything we could evaluate at no cost
    if (!ntrees || !expr || expr->IsConstant())
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

    // Add a tree that indicates we have a closure
    Name *target = new Name("[closure]", expr->Position());
    target->code = toCall;
    parent->right = target;

    // Generate the code for the arguments
    Compiler *compiler = MAIN->compiler;
    eval_fn fn = compiler->closures[ntrees];
    if (!fn)
    {
        TreeList noParms;
        OCompiledUnit unit(compiler, result, noParms, false);
        unit.CallClosure(result, ntrees);
        fn = unit.Finalize();
        compiler->closures[ntrees] = fn;
        compiler->SetTreeFunction(result, NULL); // Now owned by closures[n]
    }
    result->code = fn;
    xl_set_source(result, expr);

    return result;
}


Tree *xl_tree_copy(Tree *from, Tree *to)
// ----------------------------------------------------------------------------
//    Copy information from one tree to the other
// ----------------------------------------------------------------------------
{
    if (from && to && from != to)
    {
        if (Symbols *symbols = from->Symbols())
        {
            to->SetSymbols(symbols);
            symbols->Clear();
        }
#if 0
        if (CompilerInfo *cinfo = from->Remove<CompilerInfo>())
            to->SetInfo<CompilerInfo> (cinfo);
        if (eval_fn code = from->code)
        {
            to->code = code;
            from->code = NULL;
        }
#endif
    }
    return to;
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

    // Evaluate closures if we have any
    Prefix *prefix = tree->AsPrefix();
    if (prefix)
        tree = xl_evaluate(context, prefix);
        
    // Format contents
    if (Infix *infix = tree->AsInfix())
    {
        if (infix->name == ",")
        {
            xl_write(context, infix->left, "");
            return xl_write(context, infix->right, sep);
        }
    }

    // Evaluate non-prefix trees
    if (!prefix)
        tree = xl_evaluate(context, tree);

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


static void xl_list_files(Context *context, Tree *patterns, Tree_p *&parent)
// ----------------------------------------------------------------------------
//   Append all files found in the parent
// ----------------------------------------------------------------------------
{
    if (Block *block = patterns->AsBlock())
    {
        xl_list_files(context, block->child, parent);
        return;
    }
    if (Infix *infix = patterns->AsInfix())
    {
        if (infix->name == "," || infix->name == ";" || infix->name == "\n")
        {
            xl_list_files(context, infix->left, parent);
            xl_list_files(context, infix->right, parent);
            return;
        }
    }

    patterns = context->Evaluate(patterns);
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


Tree *xl_list_files(Context *context, Tree *patterns)
// ----------------------------------------------------------------------------
//   List all files in the given pattern
// ----------------------------------------------------------------------------
{
    Tree_p result = NULL;
    Tree_p *parent = &result;
    xl_list_files(context, patterns, parent);
    if (!result)
        result = xl_nil;
    else
        result->SetSymbols(patterns->Symbols());
    return result;
}


bool xl_write_integer(longlong x)
// ----------------------------------------------------------------------------
//   Write an integer value
// ----------------------------------------------------------------------------
{
    std::cout << x;
    return true;
}


bool xl_write_real(double x)
// ----------------------------------------------------------------------------
//   Write a double value
// ----------------------------------------------------------------------------
{
    std::cout << x;
    return true;
}


bool xl_write_text(kstring x)
// ----------------------------------------------------------------------------
//   Write a text value
// ----------------------------------------------------------------------------
{
    std::cout << x;
    return true;
}


bool xl_write_character(char x)
// ----------------------------------------------------------------------------
//   Write a character value
// ----------------------------------------------------------------------------
{
    std::cout << x;
    return true;
}


bool xl_write_cr()
// ----------------------------------------------------------------------------
//   Write a line separator
// ----------------------------------------------------------------------------
{
    std::cout << '\n';
    return true;
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
    bool exists = MAIN->files.count(path);
    if (!exists)
    {
        IFTRACE(fileload)
                std::cout << "Loading: " << path << "\n";

        bool hadError = MAIN->LoadFile(path, false);
        if (hadError)
            return Ooops("Unable to load file $1", new Text(path));
    }

    SourceFile &sf = MAIN->files[path];
    context->Import(sf.context);
    return sf.tree;
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


Tree *xl_load_data(Context *context, Tree *self,
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
        context->Import(sf.context);
        Tree *tree = sf.tree;
        if (prefix != "")
        {
            while (Infix *infix = tree->AsInfix())
            {
                xl_evaluate(context, infix->left);
                tree = infix->right;
            }
            return xl_evaluate(context, tree);
        }
        return sf.tree;
    }

    char     buffer[256];
    char    *ptr       = buffer;
    char    *end       = buffer + sizeof(buffer) - 1;
    Tree_p   tree      = NULL;
    Tree_p   line      = NULL;
    Tree_p  *treePtr   = &tree;
    Tree_p  *linePtr   = &line;
    bool     hasQuote  = false;
    bool     hasRecord = false;
    bool     hasField  = false;
    FILE    *f         = fopen(path.c_str(), "r");
    Symbols *old       = self->Symbols(); assert(old);
    Symbols *syms      = new Symbols(old);
    Tree    *result    = NULL;

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

            if (isdigit(buffer[0]) ||
                ((buffer[0] == '-' || buffer[0] == '+') && isdigit(buffer[1])))
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
                if (*buffer == 0)
                    continue;
                token = text(buffer, ptr - buffer - 1);
                child = new Text(buffer);
            }

            // Combine data that we just read to current line
            child->SetSymbols(syms);
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
                {
                    line = new Prefix(new Name(prefix), line);
                    line->SetSymbols(syms);
                    result = xl_evaluate(context, line);
                }
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
    Context *importCtx = new Context(context, NULL);
    Symbols *importSyms = new Symbols(MAIN->globals);
    MAIN->files[path] = SourceFile(path, tree, importCtx, importSyms);
    context->Import(importCtx);
    MAIN->globals->Import(importSyms);

    return tree;
}



// ============================================================================
//
//   Managing calls to/from XL
//
// ============================================================================

Tree *XLCall::operator() (SourceFile *sf)
// ----------------------------------------------------------------------------
//   Invoke the call in the context of a given source file
// ----------------------------------------------------------------------------
{
    if (Symbols *symbols = sf->symbols)
    {
        return operator() (symbols, false, true);
    }
    else
    {
        Context *context = sf->context;
        Tree *call = name;
        if (arguments)
            call = new Prefix(call, arguments);
        return context->Evaluate(call);
    }
}


Tree *XLCall::operator() (Symbols *syms, bool nullIfBad, bool cached)
// ----------------------------------------------------------------------------
//    Perform the given call in the given context
// ----------------------------------------------------------------------------
{
    assert(syms);
    Tree *callee = syms->CompileCall(name->value, args, nullIfBad, cached);
    if (callee && callee->code)
        callee = callee->code(MAIN->context, callee);
    return callee;
}


Tree *XLCall::build(Symbols *syms)
// ----------------------------------------------------------------------------
//    Perform the given call in the given context
// ----------------------------------------------------------------------------
{
    assert(syms);
    Tree *callee = syms->CompileCall(name->value, args);
    return callee;
}


// ============================================================================
//
//    Interfaces to make old and new compiler compatible (temporary)
//
// ============================================================================

Tree *xl_define(Context *context, Tree *self, Tree *form, Tree *definition)
// ----------------------------------------------------------------------------
//    Define a form in interpreted mode
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_MUST_BE_IN_INTERPRETER(context);
    context->Define(form, definition);
    return self;
}


Tree *xl_assign(Context *context, Tree *form, Tree *definition)
// ----------------------------------------------------------------------------
//   Assignment in interpreted mode
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_MUST_BE_IN_INTERPRETER(context);
    return context->Assign(form, definition);
}


Tree *xl_evaluate_sequence(Context *context, Tree *first, Tree *second)
// ----------------------------------------------------------------------------
//   Evaluate a sequence
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_FOR_INTERPRETER(context);
    xl_evaluate(context, first);
    return xl_evaluate(context, second);
}


Tree *xl_evaluate_any(Context *context, Tree *form)
// ----------------------------------------------------------------------------
//   Evaluation in both stack and scope (any lookup)
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_MUST_BE_IN_INTERPRETER(context);
    return context->Evaluate(form, Context::ANY_LOOKUP);
}


Tree *xl_evaluate_block(Context *context, Tree *child)
// ----------------------------------------------------------------------------
//   Evaluate a block in interpreted mode
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_MUST_BE_IN_INTERPRETER(context);
    return context->EvaluateBlock(child);
}


Tree *xl_evaluate_code(Context *context, Tree *self, Tree *code)
// ----------------------------------------------------------------------------
//   Evaluate a code tree in interpreted mode
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_MUST_BE_IN_INTERPRETER(context);
    return context->EvaluateCode(self, code);
}


Tree *xl_evaluate_lazy(Context *context, Tree *self, Tree *code)
// ----------------------------------------------------------------------------
//   Evaluate a lazy tree in interpreted mode
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_MUST_BE_IN_INTERPRETER(context);
    return context->EvaluateLazy(self, code);
}


Tree *xl_evaluate_in_caller(Context *context, Tree *code)
// ----------------------------------------------------------------------------
//   Evaluate code in the caller's context (interpreted mode only)
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_MUST_BE_IN_INTERPRETER(context);
    return context->EvaluateInCaller(code);
}


Tree *xl_enter_properties(Context *context, Tree *self, Tree *declarations)
// ----------------------------------------------------------------------------
//   Enter properties in interpreted mode
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_MUST_BE_IN_INTERPRETER(context);
    (void) declarations;
    return context->EnterProperty(self) ? xl_true : xl_false;
}


Tree *xl_enter_constraints(Context *context, Tree *self, Tree *constraints)
// ----------------------------------------------------------------------------
//   Enter constraints in interpreted mode
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_MUST_BE_IN_INTERPRETER(context);
    (void) constraints;
    return context->EnterConstraint(self) ? xl_true : xl_false;
}


Tree *xl_attribute(Context *context, text name, Tree *form)
// ----------------------------------------------------------------------------
//   Return the attribute associated to a given tree
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_FOR_INTERPRETER(context);
    Tree *found = context->Attribute(form, Context::SCOPE_LOOKUP, name);
    if (!found)
        found = xl_nil;
    return found;
}




// ============================================================================
//
//   Apply a code recursively to a data set (temporary / obsolete)
//
// ============================================================================

Tree *xl_apply(Context *context, Tree *code, Tree *data)
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
        data = xl_evaluate(context, data);

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
        OCompiledUnit unit(compiler, toCompile, parameters, false);
        assert (!unit.IsForwardCall() || !"Forward call in map/reduce code");

        // Record internal declarations if any
        DeclarationAction declare(symbols);
        Tree *toDecl = toCompile->Do(declare);
        assert(toDecl);

        // Compile the body we generated
        CompileAction compile(symbols, unit, true, true, false);
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
        fninfo->context = context;
        fninfo->symbols = symbols;
        fninfo->compiled = toCompile;
        fninfo->separators = separators;

        // Report compile error the first time
        if (!compiled)
            return Ooops("Cannot compile map/reduce code $1", code);

        if (!toCompile->code)
            toCompile->code = xl_evaluate_children;
    }

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
    Tree *result = new Integer(high);
    for (longlong i = high-1; i >= low; i--)
        result = new Infix(",", new Integer(i), result);
    result->code = xl_identity;
    return result;
}


Tree *xl_nth(Context *context, Tree *data, longlong index)
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
        data = xl_evaluate(context, data);

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
    MapAction map(context, function, separators);
    return what->Do(map);
}


Tree *MapAction::Do(Tree *what)
// ----------------------------------------------------------------------------
//   Apply the code to the given tree
// ----------------------------------------------------------------------------
{
    what = xl_evaluate(context, what);
    return function(context, what, what);
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
    ReduceAction reduce(context, function, separators);
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
        return function(context, infix, left, right);
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
    FilterAction filter(context, function, separators);
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
    if (function(context, what, what) == xl_true)
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


Tree *xl_add_search_path(Context *context, text prefix, text dir)
// ----------------------------------------------------------------------------
//   Add directory to the search path for prefix for the current context
// ----------------------------------------------------------------------------
{
    context->stack->AddSearchPath(prefix, dir);
    return XL::xl_true;
}


Text *xl_find_in_search_path(Context *context, text prefix, text file)
// ----------------------------------------------------------------------------
//   Add directory to the search path for prefix for the current context
// ----------------------------------------------------------------------------
{
    return new Text(context->stack->FindInSearchPath(prefix, file));
}

XL_END
