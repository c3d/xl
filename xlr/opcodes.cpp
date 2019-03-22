// *****************************************************************************
// opcodes.cpp                                                        XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010-2011, Catherine Burvelle <catherine@taodyne.com>
// (C) 2003-2004,2006-2007,2009-2011,2013,2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can r redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

#include "opcodes.h"
#include "basics.h"
#include "parser.h"
#include "errors.h"
#include "types.h"
#include "symbols.h"
#include "runtime.h"
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


void xl_enter_infix(Context *context, text name, native_fn fn, Tree *rtype,
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

    Symbols *s = MAIN->globals;
    Rewrite *rw2 = s->EnterRewrite(from, to);
    rw2->type = rtype;
    to->code = fn;
    to->SetSymbols(s);
    xl_enter_builtin(MAIN, name, to, rw2->parameters, fn);

    xl_set_documentation(from, doc);
}


void xl_enter_prefix(Context *context, text name, native_fn fn, Tree *rtype,
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

        Symbols *s = MAIN->globals;
        Rewrite *rw2 = s->EnterRewrite(from, to);
        rw2->type = rtype;
        to->code = fn;
        to->SetSymbols(s);
        xl_enter_builtin(MAIN, name, to, rw2->parameters, fn);

        xl_set_documentation(from, doc);
    }
    else
    {
        Name *n  = new Name(symbol);
        n->SetInfo<PrefixDefinitionsInfo>(new PrefixDefinitionsInfo());
        Rewrite *rw = context->Define(n, n);
        rw->native = fn;
        rw->type = rtype;

        Symbols *s = MAIN->globals;
        Rewrite *rw2 = s->EnterName(symbol, n, Rewrite::GLOBAL);
        rw2->type = rtype;
        n->code = fn;
        n->SetSymbols(s);
        TreeList noparms;
        xl_enter_builtin(MAIN, name, n, noparms, fn);

        xl_set_documentation(n, doc);
    }
}


void xl_enter_postfix(Context *context, text name, native_fn fn, Tree *rtype,
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

    Symbols *s = MAIN->globals;
    Rewrite *rw2 = s->EnterRewrite(from, to);
    rw2->type = rtype;
    to->code = fn;
    to->SetSymbols(s);
    xl_enter_builtin(MAIN, name, to, rw2->parameters, fn);

    xl_set_documentation(from, doc);
}


void xl_enter_block(Context *context, text name, native_fn fn, Tree *rtype,
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

    Symbols *s = MAIN->globals;
    Rewrite *rw2 = s->EnterRewrite(from, to);
    rw2->type = rtype;
    to->code = fn;
    to->SetSymbols(s);
    xl_enter_builtin(MAIN, name, to, rw2->parameters, fn);

    xl_set_documentation(from, doc);
}


void xl_enter_form(Context *context, text name, native_fn fn,
                   Tree *rtype, text form, TreeList &parameters,
                   text doc)
// ----------------------------------------------------------------------------
//    Enter an arbitrary form in the symbol table
// ----------------------------------------------------------------------------
{
    Tree *from = xl_parse_text(form);
    Name *to = new Name(name);

    Rewrite *rw = context->Define(from, to);
    rw->native = fn;
    rw->type = rtype;

    Symbols *s = MAIN->globals;
    Rewrite *rw2 = s->EnterRewrite(from, to);
    rw2->type = rtype;
    to->code = fn;
    to->SetSymbols(s);
    xl_enter_builtin(MAIN, name, to, rw2->parameters, fn);

    ulong sz = parameters.size();
    if (sz != rw2->parameters.size())
    {
        std::cerr << "WARNING: Internal error on parameter count for "
                  << name << "\n"
                  << "         " << form << "\n";
        ulong sz2 = rw2->parameters.size();
        for (ulong i = 0; i < sz || i < sz2; i++)
        {
            std::cerr << "  #" << i << ": ";
            if (i < sz)
                std::cerr << "spec(" << parameters[i] << ") ";
            if (i < sz2)
                std::cerr << "form(" << rw2->parameters[i] << ") ";
            std::cerr << "\n";
        }
    }

    xl_set_documentation(from, doc);
}


void xl_enter_name(Symbols *symbols, Name *name)
// ----------------------------------------------------------------------------
//   Enter a global name in the symbol table
// ----------------------------------------------------------------------------
{
    name->code = xl_identity;
    name->SetSymbols(symbols);
    symbols->EnterName(name->value, name, Rewrite::ENUM);
}


void xl_enter_type(Symbols *symbols, Name *name,
                   text castfnname, typecheck_fn tc)
// ----------------------------------------------------------------------------
//   Enter a type function into the symbol table
// ----------------------------------------------------------------------------
{
    /* Enter the type name itself */
    name->code = xl_identity;
    symbols->EnterName(name->value, name, Rewrite::TYPE);
    name->SetSymbols(symbols);

    /* Type as infix : evaluates to type check, e.g. 0 : integer */
    text nv = name->value;
    Infix *from = new Infix(":", new Name("V"), new Name(nv));
    Name *to = new Name(nv);
    Rewrite *rw = symbols->EnterRewrite(from, to);
    eval_fn typeTestFn = (eval_fn) (void *) tc;
    to->code = typeTestFn;
    to->SetSymbols(symbols);
    xl_enter_builtin(MAIN, castfnname, to,rw->parameters,typeTestFn);
}

XL_END
