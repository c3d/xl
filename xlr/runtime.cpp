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
#include "tree-clone.h"
#include "utf8_fileutils.h"

#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <errno.h>
#include "winglob.h"
#include <sys/stat.h>


XL_BEGIN


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
    if (source != value && source != xl_source(value))
    {
        value->Purge<SourceInfo>();
        value->SetInfo<SourceInfo>(new SourceInfo(source));
    }
    return value;
}


Tree *xl_error(Tree *self, text msg, Tree *a1, Tree *a2, Tree *a3)
// ----------------------------------------------------------------------------
//   The default runtime error message mechanism (if not overriden)
// ----------------------------------------------------------------------------
{
    if (a1) a1 = FormatTreeForError(a1);
    if (a2) a2 = FormatTreeForError(a2);
    if (a3) a3 = FormatTreeForError(a3);
    Error err(msg, self->Position());
    if (a1) err.Arg(a1);
    if (a2) err.Arg(a2);
    if (a3) err.Arg(a3);
    MAIN->errors->Log(err);

    return self;
}


Tree *xl_form_error(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//   Raise an error if we have a form error
// ----------------------------------------------------------------------------
{
    bool quickExit = false;     // For debugging purpose
    if (quickExit)
        return what;

    static bool recursive = false;
    if (recursive)
    {
        std::cerr << "ABORT - Recursive error during error handling\n"
                  << "Error tree: " << what << "\n";
        return XL::xl_false;
    }
    Save<bool> saveRecursive(recursive, true);

    ADJUST_CONTEXT_FOR_INTERPRETER(context);
    return Ooops("No form match $1", what);
}


Tree *xl_stack_overflow(Tree *what)
// ----------------------------------------------------------------------------
//   Return an error evaluating a tree
// ----------------------------------------------------------------------------
{
    return Ooops("Stack overflow evaluating $1", what);
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


Tree *xl_parse_text(text source)
// ----------------------------------------------------------------------------
//   Generate a tree from text
// ----------------------------------------------------------------------------
{
    std::istringstream input(source);
    Parser parser(input, MAIN->syntax, MAIN->positions, *MAIN->errors);
    return parser.Parse();
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


Tree *xl_infix_match_check(Context *context, Tree *value, kstring name)
// ----------------------------------------------------------------------------
//   Check if the value is matching the given infix
// ----------------------------------------------------------------------------
{
    // The following test is a hack to detect closures
    if (xl_closure(value))
        value = context->Evaluate(value);
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
        assert(typeExpr->code && "xl_type_check needs compiled type check");
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
    result->SetSymbols(source->Symbols());
    result->code = xl_identity;
    return result;
}


Prefix *xl_new_prefix(Prefix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Prefix
// ----------------------------------------------------------------------------
{
    Prefix *result = new Prefix(source, left, right);
    result->SetSymbols(source->Symbols());
    result->code = xl_identity;
    return result;
}


Postfix *xl_new_postfix(Postfix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Postfix
// ----------------------------------------------------------------------------
{
    Postfix *result = new Postfix(source, left, right);
    result->SetSymbols(source->Symbols());
    result->code = xl_identity;
    return result;
}


Infix *xl_new_infix(Infix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Infix
// ----------------------------------------------------------------------------
{
    Infix *result = new Infix(source, left, right);
    result->SetSymbols(source->Symbols());
    result->code = xl_identity;
    return result;
}


Block *xl_fill_block(Block *source, Tree *child)
// ----------------------------------------------------------------------------
//    Called by generated code to replace child of a Block
// ----------------------------------------------------------------------------
{
    if (child)  source->child = child;
    return source;
}


Prefix *xl_fill_prefix(Prefix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to replace children of a Prefix
// ----------------------------------------------------------------------------
{
    if (left)   source->left = left;
    if (right)  source->right = right;
    return source;
}


Postfix *xl_fill_postfix(Postfix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to replace children of a Postfix
// ----------------------------------------------------------------------------
{
    if (left)   source->left = left;
    if (right)  source->right = right;
    return source;
}


Infix *xl_fill_infix(Infix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to replace children of an Infix
// ----------------------------------------------------------------------------
{
    if (left)   source->left = left;
    if (right)  source->right = right;
    return source;
}


Tree *xl_real_list(Tree *self, uint n, double *values)
// ----------------------------------------------------------------------------
//   Build an infix from a list of real numbers
// ----------------------------------------------------------------------------
{
    Tree *result = NULL;
    while (n)
    {
        Tree *left = new Real(values[--n], self->Position());
        if (result)
            result = new Infix(",", left, result, self->Position());
        else
            result = left;
        result->code = xl_identity;
        result->SetSymbols(self->Symbols());
    }
    return result;
}


Tree *xl_integer_list(Tree *self, uint n, longlong *values)
// ----------------------------------------------------------------------------
//   Build an infix from a list of real numbers
// ----------------------------------------------------------------------------
{
    Tree *result = NULL;
    while (n)
    {
        Tree *left = new Integer(values[--n], self->Position());
        if (result)
            result = new Infix(",", left, result, self->Position());
        else
            result = left;
        result->code = xl_identity;
        result->SetSymbols(self->Symbols());
    }
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

    // Build the list of parameter names and associated arguments
    Tree_p result = NULL;
    Tree_p *treePtr = &result;
    Infix *decl = NULL;
    va_list va;
    va_start(va, ntrees);
    for (uint i = 0; i < ntrees; i++)
    {
        Name *name = va_arg(va, Name *);
        Tree *arg = va_arg(va, Tree *);
        IFTRACE(closure)
            std::cerr << "  PARM: " << name << " ARG: " << arg << '\n';
        decl = new Infix("->", name, arg, arg->Position());
        if (*treePtr)
        {
            Infix *infix = new Infix("\n", *treePtr, decl);
            *treePtr = infix;
            treePtr = &infix->right;
        }
        else
        {
            *treePtr = decl;
        }
    }
    va_end(va);

    // Build the final infix with the original expression
    *treePtr = new Infix("\n", *treePtr, expr);
    decl->code = toCall;

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


Tree *xl_reference_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Any reference argument can be accepted
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
//   Build an infix name:type, except if type is "lazy"
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


Tree *xl_list_to_tree(TreeList v, text infix, Infix ** deepest)
// ----------------------------------------------------------------------------
//   Builds a tree from a list of tree with the given infix.
// ----------------------------------------------------------------------------
// The deepest infix carries the 2 last elements of TreeList. If latest is
// provided then the deepest infix is returned and its right leg carries xl_nil.
{
    if (v.size() <= 0) return XL::xl_nil;
    if (deepest)
    {
        *deepest = NULL;
        v.push_back(XL::xl_nil);
    }
    if (v.size() == 1) return v[0];

    Tree* result = NULL;
    TreeList::reverse_iterator rit;
    // First build the bottom of the tree
    rit = v.rbegin();
    result = *rit;
    rit++;
    XL::Infix * resInf = NULL;
    // Build the common part
    for (; rit < v.rend(); ++rit)
    {
        result = resInf = new XL::Infix(infix, *rit, result);
        // Assign only the first time to remember the bottomest Infix
        if (deepest && *deepest == NULL)
            *deepest = resInf;
    }
    return result;
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

static bool isAbsolute(text path)
// ----------------------------------------------------------------------------
//    Test absolute/relative path
// ----------------------------------------------------------------------------
{
    if (path == "")
        return false;
#if defined (CONFIG_MINGW)
    if (path[0] == '/' || path[0] == '\\')
        return true;
    if (path.length() >= 2 && path[1] == ':')
        return true;
    return false;
#else
    return (path[0] == '/');
#endif
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


bool xl_file_exists(Context *context, Tree_p self, text path)
// ----------------------------------------------------------------------------
//   Check if a file exists
// ----------------------------------------------------------------------------
{
    if (!isAbsolute(path))
    {
        path = context->ResolvePrefixedPath(path);
        if (!isAbsolute(path))
        {
            // Relative path: look in same directory as parent
            if (Tree * dir = self->Symbols()->Named("module_dir"))
            {
                if (Text * txt = dir->AsText())
                    path = txt->value + "/" + path;
            }
        }
   }

#if defined(CONFIG_MINGW)
                struct _stat st;
#else
                struct stat st;
#endif

    return (utf8_stat (path.c_str(), &st) < 0) ? false : true;
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


bool xl_write_tree(Tree *tree)
// ----------------------------------------------------------------------------
//   Write a text value
// ----------------------------------------------------------------------------
{
    std::cout << tree;
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

struct ImportedFileInfo : Info
// ----------------------------------------------------------------------------
//   Information about a file that was imported (save full path)
// ----------------------------------------------------------------------------
{
    ImportedFileInfo(text path)
        : path(path) {}
    text      path;
};


Tree *xl_import(Context *context, Tree *self, text name, int phase)
// ----------------------------------------------------------------------------
//    Load a file from disk without evaluating it
// ----------------------------------------------------------------------------
{
    text path = "";
    ImportedFileInfo *info = self->GetInfo<ImportedFileInfo>();
    if (info)
    {
        path = info->path;
    }
    else
    {
        if (path == "")
            path = MAIN->SearchFile(name);
        if (path == "" && !isAbsolute(name))
        {
            // Relative path: look in same directory as parent
            // Ensure we lookup module_dir in current module first (#2660)
            Symbols *syms = self->Symbols();
            XL::Save<float> savePriority(syms->priority, 1e6);
            if (Tree * dir = syms->Named("module_dir"))
            {
                if (Text * txt = dir->AsText())
                {
                    path = txt->value + "/" + name;
#if defined(CONFIG_MINGW)
                    struct _stat st;
#else
                    struct stat st;
#endif
                    if (utf8_stat (path.c_str(), &st) < 0)
                        path = "";
                }
            }
        }
        if (path == "")
            return Ooops("Source file $1 not found", new Text(name));
        info = new ImportedFileInfo(path);
        self->SetInfo<ImportedFileInfo> (info);
    }

    // Check if the file has already been loaded somehwere.
    // If so, return the loaded file
    bool exists = MAIN->files.count(path);
    if (!exists)
    {
        IFTRACE(fileload)
                std::cout << "Loading: " << path << "\n";
        bool hadError = MAIN->LoadFile(path, false, context, self->Symbols());
        if (hadError)
            return Ooops("Unable to load file $1", new Text(path));
    }

    SourceFile &sf = MAIN->files[path];
    Tree *result = sf.tree;
    if (phase == EXECUTION_PHASE && result)
        result = context->Evaluate(result);
    return result;
}


struct LoadDataInfo : Info
// ----------------------------------------------------------------------------
//   Information about the data that was loaded
// ----------------------------------------------------------------------------
{
    LoadDataInfo(): files() {}
    struct PerFile
    {
        PerFile(): data(), loaded(), mtime(0) {}
        struct Row
        {
            TreeList        args;
        };
        typedef std::vector<Row> Data;
        Data        data;
        Tree_p      loaded;
        time_t      mtime;
    };
    std::map<text, PerFile> files;
};


Tree *xl_load_data(Context *context, Tree *self,
                   text name, text prefix, text fieldSeps, text recordSeps)
// ----------------------------------------------------------------------------
//    Load a comma-separated or tab-separated file from disk
// ----------------------------------------------------------------------------
{
    // Open data file
    text path = MAIN->SearchFile(name);
    if (path == "")
        return Ooops("CSV file $1 not found", new Text(name));

    utf8_ifstream input(path.c_str(), std::ifstream::in);
    if (!input.good())
        return Ooops("Unable to load data for $1.\n"
                     "(Accessing $2 resulted in the following error: $3)",
                     self,
                     new Text(path, "\"", "\"", self->Position()),
                     new Text(strerror(errno), "", "", self->Position()));

    return xl_load_data(context, self, path,
                        input, true, true,
                        prefix, fieldSeps, recordSeps);
}


Tree *xl_load_data(Context *context, Tree *self, text inputName,
                   std::istream &input, bool cached, bool statTime,
                   text prefix, text fieldSeps, text recordSeps)
// ----------------------------------------------------------------------------
//   Variant reading from a stream directly
// ----------------------------------------------------------------------------
{
    // Check if the file has already been loaded somehwere.
    // If so, return the loaded data
    Symbols *syms = self->Symbols();
    LoadDataInfo *info = self->GetInfo<LoadDataInfo>();
    if (!info)
    {
        // Create cache
        info = new LoadDataInfo();
        self->SetInfo<LoadDataInfo> (info);
    }

    // Load per-file cached data
    LoadDataInfo::PerFile &perFile = info->files[inputName];
    if (perFile.loaded)
    {
        if (statTime)
        {
            // Check if we need to reload the contents of the file
            // because it has been modified
#if defined(CONFIG_MINGW)
            struct _stat st;
#else
            struct stat st;
#endif
            utf8_stat (inputName.c_str(), &st);
            if (perFile.mtime != st.st_mtime)
                cached = false;
            perFile.mtime = st.st_mtime;
        }

        if (cached)
        {
            Tree_p result = xl_false;
            if (prefix.size())
            {
                ulong i, max = perFile.data.size();
                for (i = 0; i < max; i++)
                {
                    LoadDataInfo::PerFile::Row &row = perFile.data[i];
                    result = syms->CompileCall(context, prefix, row.args);
                }
                return result;
            }
            return perFile.loaded;
        }

        // Restart with clean data
        perFile.data.clear();
        perFile.loaded = NULL;
    }

    // Read data from file
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
    bool     hasPrefix = prefix.size() != 0;

    LoadDataInfo::PerFile::Row row;
    *end = 0;
    while (input.good())
    {
        int c = input.get();

        if (!hasQuote && ptr == buffer && c != '\n' && isspace(c))
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
        {
            c = 0;
        }
        else if (c == '"')
        {
            int next = input.peek();
            bool escapedQuote = (hasQuote && next == '"');
            if (escapedQuote)
                input.get();
            else
                hasQuote = !hasQuote;
        }

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
                if (buffer[0] == '"' && ptr > buffer+2 && ptr[-2] == '"')
                    token = text(buffer+1, ptr - buffer - 3);
                else
                    token = text(buffer, ptr - buffer - 1);
                child = new Text(token);
            }

            // Combine data that we just read to current line
            child->SetSymbols(syms);
            if (hasPrefix)
            {
                row.args.push_back(child);
            }
            else
            {
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
            }

            // Combine as line if necessary
            if (hasRecord)
            {
                if (hasPrefix)
                {
                    perFile.data.push_back(row);
                    tree = syms->CompileCall(context, prefix, row.args);
                    row.args.clear();
                }
                else
                {
                    if (*treePtr)
                    {
                        Infix *infix = new Infix("\n", *treePtr, line);
                        *treePtr = infix;
                        treePtr = &infix->right;
                    }
                    else
                    {
                        *treePtr = line;
                    }
                    line = NULL;
                    linePtr = &line;
                }
            }
            ptr = buffer;
        }
    }
    if (!tree)
        tree = xl_false;
perFile.loaded = tree;

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
    Tree *callee = syms->CompileCall(MAIN->context, name->value, args);
    return callee;
}


Tree *XLCall::build(Symbols *syms)
// ----------------------------------------------------------------------------
//    Perform the given call in the given context
// ----------------------------------------------------------------------------
{
    assert(syms);
    Tree *callee = syms->CompileCall(MAIN->context, name->value, args, false);
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


Tree *xl_enter_properties(Context *context, Tree *self,
                          Tree *storage, Tree *declarations)
// ----------------------------------------------------------------------------
//   Enter properties in interpreted mode
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_FOR_INTERPRETER(context);
    if (Symbols *symbols = self->Symbols())
        return symbols->EnterProperty(context, self, storage, declarations)
            ? xl_true : xl_false;

    // Probably broken after #1635
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


Tree *xl_read_property_default(Context *context, Tree *self)
// ----------------------------------------------------------------------------
//   Return the property associated with the given name
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_FOR_INTERPRETER(context);
    Symbols *symbols = self->Symbols();
    Name *name = self->AsName();
    if (!symbols || !name)
        return xl_false;
    Rewrite *prop = symbols->Entry(name->value, false);
    if (!prop)
        return self;
    return prop->to;
}


Tree *xl_write_property_default(Context *context, Tree *self, Tree *value)
// ----------------------------------------------------------------------------
//   Set the property associated with the given name
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_FOR_INTERPRETER(context);
    Symbols *symbols = self->Symbols();
    Prefix *prefix = self->AsPrefix();
    if (!symbols || !prefix)
        return xl_false;
    Name *name = prefix->left->AsName();
    if (!name)
        return xl_false;
    Rewrite *prop = symbols->Entry(name->value, true);
    kind k = value->Kind();
    Tree *type = prop->type;
    if ((type == integer_type && k != INTEGER) ||
        (type == real_type    && k != REAL)    ||
        (type == text_type    && k != TEXT)    ||
        (type == name_type    && k != NAME))
    {
        Ooops("Internal error: inconsistent type in property $1", self);
        Ooops("Received value is $1", value);
        Ooops("Expected type is $1", type);
        return value;
    }

    prop->to = value;
    return value;
}


// Default callbacks for reading and writing properties
xl_read_property_fn      xl_read_property = xl_read_property_default;
xl_write_property_fn     xl_write_property = xl_write_property_default;



// ============================================================================
//
//   Apply a code recursively to a data set (temporary / obsolete)
//
// ============================================================================

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
        else if (Name *name = toCompile->AsName())
        {
            // We have a single name: consider it as a prefix to all elements
            Name *parameter = new Name("_");
            parameters.push_back(parameter);
            toCompile = new Prefix(name, parameter);
        }
        else
        {
            // OK, we don't know what to do with this stuff...
            return Ooops("Malformed map/reduce code $1", toCompile);
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
        assert(toDecl); (void) toDecl;

        // Compile the body we generated
        CompileAction compile(symbols, unit, true, true, true);
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

        // Report compile error the first time
        if (!compiled)
            return Ooops("Cannot compile map/reduce code $1", toCompile);
    }

    Tree *result = data;
    if (fninfo->function)
        result = fninfo->Apply(result);
    else
        result = Ooops("Invalid map/reduce code $1", code);
    return result;
}



// ============================================================================
//
//  Iterator on lists of items
//
// ============================================================================

ListIterator::ListIterator(Context *context, Symbols *symbols, Tree *what)
// ----------------------------------------------------------------------------
//    Initialize the list iterator
// ----------------------------------------------------------------------------
    : context(context), symbols(symbols), data(what), separator(),
      startI(0), endI(0), stepI(0),
      startF(0), endF(0), stepF(0)
{
    if (Block *block = data->AsBlock())
    {
        // Check if we got () or (1,2,3). If so, extract child
        data = block->child;
    }

    // Check the empty list case
    if (Name *name = data->AsName())
        if (name->value == "")
            data = NULL;
}


static Tree *make(longlong i) { return new Integer(i); }
static Tree *make(double r)   { return new Real(r); }


template<class num>
Tree *ListIterator::Next(num &start, num &end, num &step)
// ----------------------------------------------------------------------------
//    Optimize special numeric operators
// ----------------------------------------------------------------------------
{
    if (step)
    {
        if (step > 0 && start <= end)
        {
            Tree *result = make(start);
            start += step;
            return result;
        }
        else if (step < 0 && start >= end)
        {
            Tree *result = make(start);
            start += step;
            return result;
        }
        step = 0;
    }
    return NULL;
}


Tree *ListIterator::EvaluateRange(Tree *input)
// ----------------------------------------------------------------------------
//   Check ranges like 1..5, otherwise evaluate input
// ----------------------------------------------------------------------------
{
    if (Infix *infix = input->AsInfix())
    {
        text sep = infix->name;

        // Check stepped numerical ranges
        if (sep == "by")
        {
            Tree_p left = infix->left;
            if (Infix *inner = left->AsInfix())
            {
                if (inner->name == "..")
                {
                    sep = inner->name;

                    Tree_p right = infix->right;
                    Integer *si = right->AsInteger();
                    Real *sr = right->AsReal();
                    if (!si && !sr)
                    {
                        if (!right->Symbols())
                            right->SetSymbols(symbols);
                        right = xl_evaluate(context, right);
                        si = right->AsInteger();
                        sr = right->AsReal();
                    }
                    infix = inner;
                    if (si)
                        stepI = si->value;
                    else if (sr)
                        stepF = sr->value;
                    else
                        sep = "";
                }
            }
        }

        // Check numerical ranges
        if (sep == "..")
        {
            Tree_p left = infix->left;
            Tree_p right = infix->right;
            Integer *li = left->AsInteger();
            Integer *ri = right->AsInteger();
            Real *lr = left->AsReal();
            Real *rr = right->AsReal();
            if (!li && !lr)
            {
                if (!left->Symbols())
                    left->SetSymbols(symbols);
                left = xl_evaluate(context, left);
                li = left->AsInteger();
                lr = left->AsReal();
            }
            if (!ri && !rr)
            {
                if (!right->Symbols())
                    right->SetSymbols(symbols);
                right = xl_evaluate(context, right);
                ri = right->AsInteger();
                rr = right->AsReal();
            }
            if (li && ri)
            {
                startI = li->value;
                endI = ri->value;
                if (!stepI)
                    stepI = 1;
                if (separator == "")
                    separator = ",";
                return Next(startI, endI, stepI);
            }
            if (lr && rr)
            {
                startF = lr->value;
                endF = rr->value;
                if (!stepF)
                    stepF = 1.0;
                if (separator == "")
                    separator = ",";
                return Next(startF, endF, stepF);
            }
        }
    }

    // Other cases: evaluate
    if (!input->Symbols())
        input->SetSymbols(symbols);
    input = xl_evaluate(context, input);
    return input;
}


Tree * ListIterator::Next()
// ----------------------------------------------------------------------------
//   Compute the next element in a list
// ----------------------------------------------------------------------------
{
    // Check if we are in an integer or float iterator
    if (Tree *ival = Next(startI, endI, stepI))
        return ival;
    if (Tree *rval = Next(startF, endF, stepF))
        return rval;

    // Check empty list
    if (!data)
        return NULL;

    // The following test is a hack to detect closures
    if (xl_closure(data))
        data = context->Evaluate(data);

    if (Infix *infix = data->AsInfix())
    {
        text sep = infix->name;

        // Check which separator we may have in the list
        if (separator == "" && (sep == "," || sep == ";" || sep == "\n"))
            separator = sep;

        // Check other cases
        if (separator == sep)
        {
            Tree *result = infix->left;
            data = infix->right;
            result = EvaluateRange(result);
            if (!result)
                result = Next();
            return result;
        }
    }

    // Last item in a list: evaluate and return
    Tree *result = data;
    data = NULL;
    result = EvaluateRange(result);
    if (separator == "" && result->Kind() == INFIX)
    {
        data = result;
        result = Next();
    }
    return result;
}



// ============================================================================
//
//   Map an operation on all elements
//
// ============================================================================

typedef Tree * (*map_fn) (Context *context, Tree *self, Tree *arg);

Tree *MapFunctionInfo::Apply(Tree *what)
// ----------------------------------------------------------------------------
//   Apply a map operation
// ----------------------------------------------------------------------------
{
    ListIterator li(context, symbols, what);
    map_fn map = (map_fn) function;
    Tree_p result = NULL;
    Tree_p *parent = &result;

    // Loop on all elements
    while (Tree *next = li.Next())
    {
        next = map(context, next, next);
        if (*parent != NULL)
        {
            Infix *newList = new Infix(li.separator,
                                       *parent, next,
                                       next->Position());
            *parent = newList;
            parent = &newList->right;
            newList->code = xl_identity;
            newList->SetSymbols(symbols);
        }
        else
        {
            *parent = next;
        }
    }

    // Return built list
    if (!result)
        return what;
    return result;
}



// ============================================================================
//
//   Reduce by applying operations to consecutive elements
//
// ============================================================================

typedef Tree * (*reduce_fn) (Context *, Tree *self, Tree *t1,Tree *t2);
Tree *ReduceFunctionInfo::Apply(Tree *what)
// ----------------------------------------------------------------------------
//   Apply a reduce operation to the tree
// ----------------------------------------------------------------------------
{
    ListIterator li(context, symbols, what);
    reduce_fn reduce = (reduce_fn) function;
    Tree_p result = NULL;

    // Loop on all elements
    while (Tree *next = li.Next())
    {
        if (result)
            result = reduce(context, next, result, next);
        else
            result = next;
    }

    // Return built list
    if (!result)
        result = xl_nil;
    return result;
}



// ============================================================================
//
//   Filter by selecting elements that match a given condition
//
// ============================================================================

typedef Tree * (*filter_fn) (Context *, Tree *self, Tree *arg);
Tree *FilterFunctionInfo::Apply(Tree *what)
// ----------------------------------------------------------------------------
//   Apply a filter operation to the tree
// ----------------------------------------------------------------------------
{
    ListIterator li(context, symbols, what);
    filter_fn filter = (map_fn) function;
    Tree_p result = NULL;
    Tree_p *parent = &result;

    // Loop on all elements
    while (Tree *next = li.Next())
    {
        Tree *test = filter(context, next, next);
        if (test == xl_true)
        {
            if (*parent != NULL)
            {
                Infix *newList = new Infix(li.separator,
                                           *parent, next,
                                           next->Position());
                *parent = newList;
                parent = &newList->right;
            }
            else
            {
                *parent = next;
            }
        }
    }

    // Return built list
    if (!result)
        return xl_false;

    return result;
}



// ============================================================================
//
//   References, indexing and assignment
//
// ============================================================================

Rewrite *xl_data_reference(Context *context, Tree *expr)
// ----------------------------------------------------------------------------
//   Build a symbol table for a list
// ----------------------------------------------------------------------------
{
    Symbols *exprSymbols = expr->Symbols();
    Symbols *symbols = exprSymbols;

    // Check if already built
    if (symbols)
    {
        if (Rewrite *rewrite = symbols->Rewrites())
        {
            if (rewrite->kind == Rewrite::METADATA)
            {
                if (Name *name = rewrite->from->AsName())
                    if (name->value == ":List:")
                        return rewrite;
            }
        }
    }

    // No symbol table, or invalid one: create data symbol table
    symbols = new Symbols(symbols);
    expr->SetSymbols(symbols);

    // Create the original entry for the symbol table
    longlong count = 0;
    TreePosition pos = expr->Position();
    Name_p listMarker = new Name(":List:", pos);
    Integer_p listSize = new Integer(count, pos);
    Rewrite_p top = symbols->EnterRewrite(listMarker, listSize);
    top->kind = Rewrite::METADATA;
    top->symbols = symbols;
    top->from->SetSymbols(exprSymbols);

    // Evacuate block
    if (Block *block = expr->AsBlock())
        expr = block->child;

    // Check the empty name case
    if (Name *name = expr->AsName())
        if (name->value == "")
            return top;

    // Populate the symbol table with the entries
    while (expr)
    {
        Tree *next = NULL;

        if (Infix *infix = expr->AsInfix())
        {
            if (infix->name == ",")
            {
                expr = infix->left;
                next = infix->right;
            }
        }

        Integer_p index = new Integer(++count, pos);
        Rewrite *rw = symbols->EnterRewrite(index, expr);
        expr = next;
        rw->from->SetSymbols(exprSymbols);
    }

    // Update size of the list
    listSize->value = count;

    // Return the top-level entry
    return top;
}


Rewrite *xl_reference(Context *context, Tree *expr, bool create)
// ----------------------------------------------------------------------------
//   Find the property associated with the given expression
// ----------------------------------------------------------------------------
{
    bool outer = true;
    Symbols *symbols = expr->Symbols();
    Tree *type = NULL;

    IFTRACE(references)
        std::cerr << "Reference: " << expr << " in " << symbols << "\n";

    // Deal with non-list cases
    while (expr)
    {
        Tree *left = NULL;
        Tree *right = NULL;

        IFTRACE(references)
            std::cerr << " looking at " << expr << "\n";

        // Lookup terminals
        if (expr->IsLeaf())
        {
            Rewrite *entry = outer
                ? symbols->LookupEntry(expr, create)
                : symbols->Entry(expr, create);
            if (entry && type)
            {
                if (!entry->type)
                {
                    entry->type = type;
                }
                else if (entry->type != type)
                {
                    Ooops("Invalid type override with $1", type);
                    Ooops("Previous type was $1", entry->type);
                }
            }

            IFTRACE(references)
                if (entry)
                    std::cerr << " found " << entry << ": "
                              << entry->from << "->" << entry->to << "\n";

            return entry;
        }

        // Check special infix notations
        else if (Infix *infix = expr->AsInfix())
        {
            if (infix->name == ":")
            {
                // A:B = record that we have a type
                if (!type)
                {
                    type = xl_evaluate(context, infix->right);
                }
                else
                {
                    Ooops("Extraneous type specification $1", infix->right);
                    Ooops("Previous type specification $1", type);
                }
            }
            else if (infix->name == ".")
            {
                // A.B = Dereference A, then lookup B
                left = infix->left;
                right = infix->right;
            }
            else if (infix->name == ",")
            {
                // A,B = Infix list
                return xl_data_reference(context, infix);
            }
        }

        // Check special prefix notation A[B]
        else if (Prefix *prefix = expr->AsPrefix())
        {
            if (Block *br = prefix->right->AsBlock())
            {
                if (br->IsSquare())
                {
                    left = prefix->left;
                    if (!br->child->Symbols())
                    {
                        IFTRACE(references)
                            std::cerr << " child " << br->child
                                      << " receives symbols " << symbols
                                      << "\n";
                        br->child->SetSymbols(symbols);
                    }
                    right = xl_evaluate(context, br->child);
                    IFTRACE(references)
                        std::cerr << "Array " << left << " index " << right
                                  << " symbols " << symbols << "\n";
                }
            }
        }

        // Evacuate blocks
        else if (Block *block = expr->AsBlock())
        {
            expr = block->child;
            continue;           // Do not evaluate anything
        }

        // Check empty names
        else if (Name *name = expr->AsName())
        {
            if (name->value == "")
                return xl_data_reference(context, expr);
        }

        // Check constants
        else if (expr->IsConstant())
        {
            return xl_data_reference(context, expr);
        }

        // Check if we were in a A.B or A[B] case
        if (left && right)
        {
            // Find the reference on the left
            if (!left->Symbols())
                left->SetSymbols(symbols);

            Rewrite *ref = xl_reference(context, left, true);
            IFTRACE(references)
            {
                std::cerr << "Reference " << left
                          << " symbols " << symbols
                          << "\n";
                if (ref)
                    std::cerr << " ref " << ref << ":"
                              << ref->from << "->" << ref->to
                              << "\n";
            }

            // Create value and symbol table if they don't exist
            Symbols *s = ref->symbols;
            if (!s)
            {
                if (ref->to)
                {
                    s = ref->symbols = new Symbols(ref->to->Symbols());
                    IFTRACE(references)
                        std::cerr << " gets symbols " << s
                                  << " from " << ref->to << "\n";
                }
                else
                {
                    s = ref->symbols = new Symbols(symbols);
                }
                expr->SetSymbols(s);
            }
            IFTRACE(references)
                std::cerr << "  inner symbol table " << s << "\n";
            symbols = s;
            expr = right;
            if (!expr->Symbols())
                expr->SetSymbols(symbols);
        }
        else
        {
            // All other cases: evaluate
            Tree *evaluated = xl_evaluate(context, expr);
            if (evaluated == expr)
                return NULL;
            expr = evaluated;
        }

        // From now on, we are looking at inner symbols, don't lookup
        outer = false;

    } // While (expr)

    return NULL;
}


Tree *xl_assign_constant(Context *context, Tree *old, Tree *value)
// ----------------------------------------------------------------------------
//    Assign in place when the left is an integer, real or text
// ----------------------------------------------------------------------------
//    This is a backward compatible behavior
{
    kind ok = old->Kind();
    kind vk = value->Kind();
    if (ok == INTEGER && vk == INTEGER)
    {
        ((Integer *) old)->value = ((Integer *) value)->value;
    }
    else if (ok == REAL && vk == REAL)
    {
        ((Real *) old)->value = ((Real *) value)->value;
    }
    else if (ok == REAL && vk == INTEGER)
    {
        ((Real *) old)->value = ((Integer *) value)->value;
    }
    else if (ok == TEXT && vk == TEXT)
    {
        ((Text *) old)->value = ((Text *) value)->value;
    }
    else if (ok == NAME && vk == NAME)
    {
        Ooops("Assigning to constant name $1", old);
        Ooops("New value was $1", value);
    }
    else
    {
        Ooops("Incompatible assignment of $1", value);
        Ooops("Old value was $1", old);
    }
    return value;
}


Tree *xl_assign(Context *context, Tree *var, Tree *value)
// ----------------------------------------------------------------------------
//   Assignment in interpreted mode
// ----------------------------------------------------------------------------
{
    IFTRACE(references)
        std::cerr << "Assigning " << var << ":=" << value << "\n";

    ADJUST_CONTEXT_FOR_INTERPRETER(context);
    if (XL::Options::options->optimize_level == 1)
    {
        value = xl_evaluate(context, value);
        if (var->IsConstant())
            return xl_assign_constant(context, var, value);

        Rewrite *rw = xl_reference(context, var, true);
        if (rw)
        {
            IFTRACE(references)
                std::cerr << " assign ref " << rw << ": "
                          << rw->from << "->" << rw->to << "\n";

            // Writing to an explicit rewrite: only works if same type
            if (rw->kind == Rewrite::LOCAL)
                return xl_assign_constant(context, rw->to, value);

            rw->to = value;
            if (rw->kind == Rewrite::UNKNOWN)
                rw->kind = Rewrite::ASSIGNED;
        }
        else
        {
            IFTRACE(references)
                std::cerr << "WARNING: No reference for " << var << "\n";
        }
        return value;
    }
    return context->Assign(var, value);
}


Tree *xl_index(Context *context, Tree *data, Tree *indexTree)
// ----------------------------------------------------------------------------
//   Find the given element in a data set or return false, e.g. X.Y
// ----------------------------------------------------------------------------
{
    IFTRACE(references)
        std::cerr << "Index " << data << "[" << indexTree << "]\n";

    // Identify the reference for X
    Rewrite *rw = xl_reference(context, data, false);
    IFTRACE(references)
        std::cerr << " index ref " << rw << "\n";
    if (!rw)
        return xl_false;

    // Get the symbol table for X
    Symbols *symbols = rw->symbols;
    IFTRACE(references)
        std::cerr << " rewrite " << rw << ": "
                  << rw->from << "->" << rw->to
                  << " symbols " << symbols << "\n";
    if (!symbols)
    {
        symbols = rw->to->Symbols();
        if (!symbols)
            return xl_false;
        rw->symbols = symbols;
    }

    // In X's symbol table, find Y (don't look up, we want only local results)
    rw = symbols->Entry(indexTree, false);
    IFTRACE(references)
    {
        std::cerr << " index inner " << rw;
        if (rw)
            std::cerr << ": "
                      << rw->from << "->" << rw->to;
        std::cerr << "\n";
    }
    if (!rw || !rw->to)
        return xl_false;
    return rw->to;
}


Tree *xl_array_index(Context *context, Tree *data, Tree *indexTree)
// ----------------------------------------------------------------------------
//   Evaluate things like A[B]
// ----------------------------------------------------------------------------
{
    indexTree = xl_evaluate(context, indexTree);
    return xl_index(context, data, indexTree);
}


Integer *xl_size(Context *context, Tree *data)
// ----------------------------------------------------------------------------
//   Return the length / size of a given list
// ----------------------------------------------------------------------------
{
    Rewrite *rw = xl_data_reference(context, data);
    ulong count = 0;
    if (rw)
    {
        // Check if this is an already computed list, if so return size
        Symbols *symbols = rw->symbols;
        if (symbols)
            if (rw->kind == Rewrite::METADATA)
                if (Name *name = rw->from->AsName())
                    if (name->value == ":List:")
                        return rw->to->AsInteger();

        count = symbols->Count(~(1<<Rewrite::METADATA));
    }
    return new Integer(count, data->Position());
}



// ============================================================================
//
//   File search path
//
// ============================================================================

Tree *xl_add_search_path(Context *context, text prefix, text dir)
// ----------------------------------------------------------------------------
//   Add directory to the search path for prefix for the current context
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_FOR_INTERPRETER(context);
    context->AddSearchPath(prefix, dir);
    return XL::xl_true;
}


Text *xl_find_in_search_path(Context *context, text prefix, text file)
// ----------------------------------------------------------------------------
//   Add directory to the search path for prefix for the current context
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_FOR_INTERPRETER(context);
    return new Text(context->FindInSearchPath(prefix, file));
}


void xl_enter_declarator(text name, decl_fn fn)
// ----------------------------------------------------------------------------
//   Enter a declarator
// ----------------------------------------------------------------------------
//   The declarators are keywords that need to be processed during the
//   declaration phase, e.g. 'import'
{
    Symbols::declarators[name] = fn;
}


Name *xl_set_override_priority(Tree *self, float priority)
// ----------------------------------------------------------------------------
//   Set the override level for the current symbol table
// ----------------------------------------------------------------------------
{
    if (Symbols *syms = self->Symbols())
    {
        syms->priority = priority;
        return xl_true;
    }
    return xl_false;
}



// ============================================================================
//
//    Loops
//
// ============================================================================

Tree *xl_integer_for_loop(Context *context, Tree *self,
                          Tree *Variable,
                          longlong low, longlong high, longlong step,
                          Tree *body)
// ----------------------------------------------------------------------------
//    Loop over an integer variable
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_FOR_INTERPRETER(context);

    if (!body->Symbols())
        body->SetSymbols(self->Symbols());

    Tree_p result = xl_false;
    Integer_p ival = new Integer(low, self->Position());
    if (step >= 0)
    {
        for (longlong i = low; i <= high; i += step)
        {
            ival->value = i;
            xl_assign(context, Variable, ival);
            result = context->Evaluate(body);
        }
    }
    else
    {
        for (longlong i = low; i >= high; i += step)
        {
            ival->value = i;
            xl_assign(context, Variable, ival);
            result = context->Evaluate(body);
        }
    }
    return result;
}


Tree *xl_real_for_loop(Context *context, Tree *self,
                       Tree *Variable,
                       double low, double high, double step, Tree *body)
// ----------------------------------------------------------------------------
//    Loop over a real variable
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_FOR_INTERPRETER(context);

    if (!body->Symbols())
        body->SetSymbols(self->Symbols());

    Tree_p result = xl_false;
    Real_p rval = new Real(low, self->Position());
    if (step >= 0)
    {
        for (double i = low; i <= high; i += step)
        {
            rval->value = i;
            xl_assign(context, Variable, rval);
            result = context->Evaluate(body);
        }
    }
    else
    {
        for (double i = low; i >= high; i += step)
        {
            rval->value = i;
            xl_assign(context, Variable, rval);
            result = context->Evaluate(body);
        }
    }
    return result;
}


Tree *xl_list_for_loop(Context *context, Tree *self,
                       Tree *Variable,
                       Tree *list, Tree *body)
// ----------------------------------------------------------------------------
//   Loop over a list
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_FOR_INTERPRETER(context);

    // Extract list value
    Tree_p result = xl_false;
    if (Block *block = list->AsBlock())
        list = block->child;
    if (Name *name = list->AsName())
        if (name->value == "")
            return result;      // empty list

    // Loop on list items
    Tree *next = list;
    while (next)
    {
        Tree *value = next;
        Infix *infix = value->AsInfix();
        if (infix && (infix->name == "," ||
                      infix->name == ";" ||
                      infix->name == "\n"))
        {
            value = infix->left;
            next = infix->right;
        }
        else
        {
            next = NULL;
        }

        // Set the loop index
        xl_assign(context, Variable, value);
        result = context->Evaluate(body);
    }

    return result;
}


Tree *xl_while_loop(Context *context, Tree *self,
                    Tree *condition, Tree *body, bool TestValue)
// ----------------------------------------------------------------------------
//   Conditional loop (while and until)
// ----------------------------------------------------------------------------
{
    ADJUST_CONTEXT_FOR_INTERPRETER(context);

    if (!condition->Symbols())
        condition->SetSymbols(self->Symbols());
    if (!body->Symbols())
        body->SetSymbols(self->Symbols());

    Tree_p result = xl_false;
    Tree_p test = TestValue ? xl_true : xl_false;
    while (true)
    {
        Tree_p value = context->Evaluate(condition);
        if (value != test)
            break;
        result = context->Evaluate(body);
    }
    return result;
}

XL_END


uint xl_recursion_count = 0;
