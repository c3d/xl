// ****************************************************************************
//  opcodes.cpp                     (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//    Opcodes are native trees generated as part of compilation/optimization
//    to speed up execution. They represent a step in the evaluation of
//    the code.
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

#include "opcodes.h"
#include "basics.h"
#include "parser.h"
#include "errors.h"
#include "types.h"
#include "runtime.h"
#include <typeinfo>

XL_BEGIN


Name_p xl_native = NULL;


// ============================================================================
//
//    Helper functions for native code
//
// ============================================================================

longlong xl_integer_arg(Tree *value)
// ----------------------------------------------------------------------------
//    Return an integer value
// ----------------------------------------------------------------------------
{
    if (Integer *ival = value->AsInteger())
        return ival->value;
    Ooops("Value $1 is not an integer", value);
    return 0;
}


double xl_real_arg(Tree *value)
// ----------------------------------------------------------------------------
//    Return a real value
// ----------------------------------------------------------------------------
{
    if (Real *rval = value->AsReal())
        return rval->value;
    Ooops("Value $1 is not a real", value);
    return 0.0;
}


text xl_text_arg(Tree *value)
// ----------------------------------------------------------------------------
//    Return a text value
// ----------------------------------------------------------------------------
{
    if (Text *tval = value->AsText())
        if (tval->opening != "'")
            return tval->value;
    Ooops("Value $1 is not a text", value);
    return "";
}


int xl_character_arg(Tree *value)
// ----------------------------------------------------------------------------
//    Return a character value
// ----------------------------------------------------------------------------
{
    if (Text *tval = value->AsText())
        if (tval->opening == "'" && tval->value.length() == 1)
            return tval->value[0];
    Ooops("Value $1 is not a character", value);
    return 0;
}


bool xl_boolean_arg(Tree *value)
// ----------------------------------------------------------------------------
//    Return a boolean truth value
// ----------------------------------------------------------------------------
{
    if (value == xl_true)
        return true;
    else if (value == xl_false)
        return false;
    Ooops("Value $1 is not a boolean value", value);
    return false;
}


Tree *xl_parameters_tree(TreeList parameters)
// ----------------------------------------------------------------------------
//   Create a comma-separated parameter list
// ----------------------------------------------------------------------------
{
    ulong i, max = parameters.size(), last = max;
    Tree *result = NULL;
    for (i = 0; i < max; i++)
    {
        Tree *parm = parameters[--last];
        if (result)
            result = new Infix(",", parm, result);
        else
            result = parm;
    }
    return result;
}


void xl_set_documentation(Tree *node, text doc)
// ----------------------------------------------------------------------------
//   Attach the documentation to the node as a comment
// ----------------------------------------------------------------------------
{
    if (!doc.empty())
    {
        std::vector<text> com(1,doc);
        CommentsInfo *cinfo = new CommentsInfo();
        cinfo->after = com;
        node->SetInfo<CommentsInfo> (cinfo);
    }
}


Tree *xl_native_function(text symbol)
// ----------------------------------------------------------------------------
//   Return a native function for the given address
// ----------------------------------------------------------------------------
{
    Name *result = new Name(symbol);
    return result;
}


void xl_enter_infix(Context *context, text name, eval_fn fn, Tree *rtype,
                    text t1, text symbol, text t2, text doc)
// ----------------------------------------------------------------------------
//   Enter an infix into the context (called from .tbl files)
// ----------------------------------------------------------------------------
{
    Tree *ldecl = xl_parameter("l", t1);
    Tree *rdecl = xl_parameter("r", t2);
    Tree *to = xl_native_function(symbol);
    Infix *from = new Infix(symbol, ldecl, rdecl);
    if (rtype && rtype != tree_type)
        from = new Infix("as", from, rtype, from->Position());
    context->Define(from, to);
    xl_enter_builtin(MAIN, name, from, to, fn);
    xl_set_documentation(from, doc);
}


void xl_enter_prefix(Context *context, text name, eval_fn fn, Tree *rtype,
                     TreeList &parameters, text symbol, text doc)
// ----------------------------------------------------------------------------
//   Enter a prefix into the context (called from .tbl files)
// ----------------------------------------------------------------------------
{
    Tree *to = xl_native_function(symbol);
    if (parameters.size())
    {
        Tree *parmtree = xl_parameters_tree(parameters);
        Tree *from = new Prefix(new Name(symbol), parmtree);
        if (rtype && rtype != tree_type)
            from = new Infix("as", from, rtype, from->Position());
        context->Define(from, to);
        xl_enter_builtin(MAIN, name, from, to, fn);
        xl_set_documentation(from, doc);
    }
    else
    {
        Tree *from = new Name(symbol);
        if (rtype && rtype != tree_type)
            from = new Infix("as", from, rtype, from->Position());
        context->Define(from, to);
        xl_enter_builtin(MAIN, name, from, to, fn);
        xl_set_documentation(from, doc);
    }
}


void xl_enter_postfix(Context *context, text name, eval_fn fn, Tree *rtype,
                      TreeList &parameters, text symbol, text doc)
// ----------------------------------------------------------------------------
//   Enter a postfdix into the context (called from .tbl files)
// ----------------------------------------------------------------------------
{
    Tree *to = xl_native_function(symbol);
    Tree *parmtree = xl_parameters_tree(parameters);
    Tree *from = new Postfix(parmtree, new Name(symbol));
    if (rtype && rtype != tree_type)
        from = new Infix("as", from, rtype, from->Position());
    context->Define(from, to);
    xl_enter_builtin(MAIN, name, from, to, fn);
    xl_set_documentation(from, doc);
}


void xl_enter_block(Context *context, text name, eval_fn fn, Tree *rtype,
                    text open, text type, text close,
                    text doc)
// ----------------------------------------------------------------------------
//    Enter a block into the context (called from .tbl files)
// ----------------------------------------------------------------------------
{
    Tree *to = xl_native_function(open + close);
    Tree *parms = xl_parameter("child", type);
    Tree *from = new Block(parms, open, close);
    if (rtype && rtype != tree_type)
        from = new Infix("as", from, rtype, from->Position());
    from = new Block(from, open, close); // Extra block removed by Define
    context->Define(from, to);
    xl_enter_builtin(MAIN, name, from, to, fn);
    xl_set_documentation(from, doc);
}


void xl_enter_form(Context *context, text name, eval_fn fn,
                   Tree *rtype, text form, TreeList &parameters,
                   text doc)
// ----------------------------------------------------------------------------
//    Enter an arbitrary form in the symbol table
// ----------------------------------------------------------------------------
{
    Tree *to = xl_native_function(form);
    Tree *from = xl_parse_text(form);
    if (rtype && rtype != tree_type)
        from = new Infix("as", from, rtype, from->Position());
    context->Define(from, to);
    xl_enter_builtin(MAIN, name, from, to, fn);
    xl_set_documentation(from, doc);
}


void xl_enter_name(Name *name)
// ----------------------------------------------------------------------------
//   Enter a global name in the symbol table
// ----------------------------------------------------------------------------
{
    (void) name;
}


void xl_enter_type(Name *name, text castfnname, typecheck_fn tc)
// ----------------------------------------------------------------------------
//   Enter a type function into the symbol table
// ----------------------------------------------------------------------------
{
    /* Type as infix : evaluates to type check, e.g. 0 : integer */
    text nv = name->value;
    Infix *from = new Infix("as", new Name("V"), new Name(nv));
    Name *to = new Name(nv);
    xl_enter_builtin(MAIN, castfnname, from, to, (eval_fn) tc);
}

XL_END
