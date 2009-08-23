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

    IFTRACE(eval)
        std::cerr << "EVAL: " << what << '\n';

    Tree *result = what;
    if (!result->IsConstant())
    {
        if (!what->code)
        {
            Symbols *symbols = what->symbols;
            if (!symbols)
            {
                std::cerr << "WARNING: No symbols for '" << what << "'\n";
                symbols = Symbols::symbols;
            }
            what = symbols->CompileAll(what);
        }

        assert(what->code);
        result = what->code(what);

        IFTRACE(eval)
            std::cerr << "EVAL= " << result << '\n';
    }

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
    XL::TreeMatch compareForEquality(right);
    if (left->Do(compareForEquality))
        return true;
    return false;
}


bool xl_type_check(Tree *value, Tree *type)
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

    Tree *afterTypeCast = type->code(value);
    if (afterTypeCast != value)
    {
        IFTRACE(typecheck)
            std::cerr << "Failed (not same type)\n";
        return false;
    }
    IFTRACE(typecheck)
        std::cerr << "Success\n";
    return true;
}



// ========================================================================
// 
//    Creating entities (callbacks for compiled code)
// 
// ========================================================================

Tree *xl_new_integer(longlong value)
{
    Tree *result = new Integer(value);
    result->code = xl_identity;
    return result;
}

Tree *xl_new_real(double value)
{
    Tree *result = new Real (value);
    result->code = xl_identity;
    return result;
}

Tree *xl_new_character(kstring value)
{
    Tree *result = new Text(value, "'", "'");
    result->code = xl_identity;
    return result;
}

Tree *xl_new_text(kstring value)
{
    Tree *result = new Text(text(value));
    result->code = xl_identity;
    return result;
}

Tree *xl_new_xtext(kstring value, kstring open, kstring close)
{
    Tree *result = new Text(value, open, close);
    result->code = xl_identity;
    return result;
}
Tree *xl_new_block(Block *source, Tree *child)
{
    Tree *result = new Block(source, child);
    result->code = xl_identity;
    return result;
}
Tree *xl_new_prefix(Prefix *source, Tree *left, Tree *right)
{
    Tree *result = new Prefix(source, left, right);
    result->code = xl_identity;
    return result;
}
Tree *xl_new_postfix(Postfix *source, Tree *left, Tree *right)
{
    Tree *result = new Postfix(source, left, right);
    result->code = xl_identity;
    return result;
}
Tree *xl_new_infix(Infix *source, Tree *left, Tree *right)
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
    return Error("No form matches '$1'", what);
}



// ========================================================================
// 
//    Type matching
// 
// ========================================================================

Tree *xl_boolean(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a boolean value (true/false)
// ----------------------------------------------------------------------------
{
    if (value == xl_true || value == xl_false)
        return value;
    return NULL;
}
    
    
Tree *xl_integer(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an integer
// ----------------------------------------------------------------------------
{
    if (Integer *it = value->AsInteger())
        return it;
    return NULL;
}
    
    
Tree *xl_real(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a real
// ----------------------------------------------------------------------------
{
    if (Real *rt = value->AsReal())
        return rt;
    return NULL;
}
    
    
Tree *xl_text(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a text
// ----------------------------------------------------------------------------
{
    if (Text *tt = value->AsText())
        if (tt->opening != "'")
            return tt;
    return NULL;
}
    
    
Tree *xl_character(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a character
// ----------------------------------------------------------------------------
{
    if (Text *tt = value->AsText())
        if (tt->opening == "'")
            return tt;
    return NULL;
}
    
    
Tree *xl_tree(Tree *value)
// ----------------------------------------------------------------------------
//   Don't really check the argument
// ----------------------------------------------------------------------------
{
    return value;
}

    
Tree *xl_infix(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an infix
// ----------------------------------------------------------------------------
{
    if (Infix *it = value->AsInfix())
        return it;
    return NULL;
}
    
    
Tree *xl_prefix(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a prefix
// ----------------------------------------------------------------------------
{
    if (Prefix *it = value->AsPrefix())
        return it;
    return NULL;
}
    
    
Tree *xl_postfix(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a postfix
// ----------------------------------------------------------------------------
{
    if (Postfix *it = value->AsPostfix())
        return it;
    return NULL;
}
    
    
Tree *xl_block(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a block
// ----------------------------------------------------------------------------
{
    if (Block *it = value->AsBlock())
        return it;
    return NULL;
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
    tree->SetSymbols(syms);
    tree = syms->CompileAll(tree);
    Symbols::symbols = old;
    old->Import(syms);

    return tree;
}

XL_END
