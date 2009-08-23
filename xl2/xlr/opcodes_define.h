// ****************************************************************************
//  opcodes_define.h               (C) 1992-2009 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Macros used to declare built-ins.
// 
//     Usage:
//     #include "opcodes_declare.h"
//     #include "builtins.tbl"
// 
//     #include "opcodes_define.h"
//     #include "builtins.tbl"
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

#undef INFIX
#undef PREFIX
#undef POSTFIX
#undef NAME
#undef TYPE
#undef PARM

#define INFIX(t1, symbol, t2, name, _code)                              \
    do                                                                  \
    {                                                                   \
        Infix *ldecl = new Infix(":", new Name("l"), new Name(#t1));    \
        Infix *rdecl = new Infix(":", new Name("r"), new Name(#t2));    \
        Infix *from = new Infix(symbol, ldecl, rdecl);                  \
        Name *to = new Name(symbol);                                    \
        eval_fn fn = (eval_fn) xl_##name;                               \
        Rewrite *rw = c->EnterRewrite(from, to);                        \
        to->code = fn;                                                  \
        to->symbols = c;                                                \
        compiler->EnterBuiltin("xl_" #name,                             \
                               from, to, rw->parameters, fn);           \
    } while(0);

#define PARM(symbol, type)                                      \
        if (text(#type) == "tree")                              \
        {                                                       \
            Name *symbol##_decl = new Name(#symbol);            \
            parameters.push_back(symbol##_decl);                \
        }                                                       \
        else                                                    \
        {                                                       \
            Infix *symbol##_decl = new Infix(":",               \
                                             new Name(#symbol), \
                                             new Name(#type));  \
            parameters.push_back(symbol##_decl);                \
        }

#define PREFIX(symbol, parms, name, _code)                              \
    do                                                                  \
    {                                                                   \
        tree_list parameters;                                           \
        parms;                                                          \
        eval_fn fn = (eval_fn) xl_##name;                               \
        if (parameters.size())                                          \
        {                                                               \
            Tree *parmtree = ParametersTree(parameters);                \
            Prefix *from = new Prefix(new Name(symbol), parmtree);      \
            Name *to = new Name(symbol);                                \
            Rewrite *rw = c->EnterRewrite(from, to);                    \
            to->code = fn;                                              \
            to->symbols = c;                                            \
            compiler->EnterBuiltin("xl_" #name,                         \
                                   from, to, rw->parameters, fn);       \
        }                                                               \
        else                                                            \
        {                                                               \
            Name *n  = new Name(symbol);                                \
            n->code = fn;                                               \
            n->symbols = c;                                             \
            c->EnterName(symbol, n);                                    \
            tree_list noparms;                                          \
            compiler->EnterBuiltin("xl_" #name, n, n, noparms, fn);     \
        }                                                               \
    } while(0);

#define POSTFIX(t1, symbol, name, _code)                                \
    do                                                                  \
    {                                                                   \
        tree_list parameters;                                           \
        parms;                                                          \
        Tree *parmtree = ParametersTree(parameters);                    \
        Prefix *from = new Postfix(parmtree, new Name(symbol));         \
        Name *to = new Name(symbol);                                    \
        eval_fn fn = (eval_fn) xl_##name;                               \
        Rewrite *rw = c->EnterRewrite(from, to);                        \
        to->code = fn;                                                  \
        to->symbols = c;                                                \
        compiler->EnterBuiltin("xl_" #name,                             \
                               from, to, rw->parameters, to->code);     \
    } while(0);

#define NAME(symbol)                            \
    do                                          \
    {                                           \
        Name *n = new Name(#symbol);            \
        n->code = xl_identity;                  \
        n->symbols = c;                         \
        c->EnterName(#symbol, n);               \
        xl_##symbol = n;                        \
        compiler->EnterGlobal(n, &xl_##symbol); \
    } while (0);

#define TYPE(symbol)                                    \
    do                                                  \
    {                                                   \
        Name *n = new Name(#symbol);                    \
        eval_fn fn = (eval_fn) xl_##symbol;             \
        n->code = fn;                                   \
        n-> symbols = c;                                \
        c->EnterName(#symbol, n);                       \
        xl_##symbol##_name = n;                         \
        compiler->EnterGlobal(n, &xl_##symbol##_name);  \
    } while(0);
