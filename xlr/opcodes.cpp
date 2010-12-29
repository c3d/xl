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
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "opcodes.h"
#include "basics.h"
#include "parser.h"
#include "errors.h"
#include "types.h"
#include <typeinfo>

XL_BEGIN

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


void xl_enter_infix(Context *context, native_fn fn, Tree *rtype,
                    text t1, text symbol, text t2, text doc)
// ----------------------------------------------------------------------------
//   Enter an infix into the context (called from .tbl files)
// ----------------------------------------------------------------------------
{
    Tree *ldecl = xl_parameter("l", t1);
    Tree *rdecl = xl_parameter("r", t2);
    Infix *from = new Infix(symbol, ldecl, rdecl);
    Name *to = new Name(symbol);

    Rewrite *rw = context->Define(from, to);
    rw->native = fn;
    rw->type  = rtype;

    xl_set_documentation(from, doc);
}


void xl_enter_prefix(Context *context, native_fn fn, Tree *rtype,
                     TreeList &parameters, text symbol, text doc)
// ----------------------------------------------------------------------------
//   Enter a prefix into the context (called from .tbl files)
// ----------------------------------------------------------------------------
{
    if (parameters.size())
    {
        Tree *parmtree = xl_parameters_tree(parameters);

        Prefix *from = new Prefix(new Name(symbol), parmtree);
        Name *to = new Name(symbol);

        Rewrite *rw = context->Define(from, to);
        rw->native = fn;
        rw->type = rtype;

        xl_set_documentation(from, doc);
    }
    else
    {
        Name *n  = new Name(symbol);

        Rewrite *rw = context->Define(n, n);
        rw->native = fn;
        rw->type = rtype;

        xl_set_documentation(n, doc);
    }
}


void xl_enter_postfix(Context *context, native_fn fn, Tree *rtype,
                      TreeList &parameters, text symbol, text doc)
// ----------------------------------------------------------------------------
//   Enter a postfdix into the context (called from .tbl files)
// ----------------------------------------------------------------------------
{
    Tree *parmtree = xl_parameters_tree(parameters);
    Postfix *from = new Postfix(parmtree, new Name(symbol));
    Name *to = new Name(symbol);
    
    Rewrite *rw = context->Define(from, to);
    rw->native = (native_fn) fn;
    rw->type = rtype;
    
    xl_set_documentation(from, doc);
}


void xl_enter_block(Context *context, native_fn fn, Tree *rtype,
                    text open, text type, text close,
                    text doc)
// ----------------------------------------------------------------------------
//    Enter a block into the context (called from .tbl files)
// ----------------------------------------------------------------------------
{
    Tree *parms = xl_parameter("c", type);
    Block *from = new Block(parms, open, close);
    Name *to = new Name(open + close);

    from = new Block(from, open, close); // Extra block removed by Define

    Rewrite *rw = context->Define(from, to);
    rw->native = (native_fn) fn;
    rw->type = rtype;

    xl_set_documentation(from, doc);
}

XL_END

