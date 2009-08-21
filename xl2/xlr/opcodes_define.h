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
#undef MPARM

#define INFIX(t1, symbol, t2, name, _code)                              \
    do                                                                  \
    {                                                                   \
        Infix *ldecl = new Infix(":", new Name("l"), new Name(#t1));    \
        Infix *rdecl = new Infix(":", new Name("r"), new Name(#t2));    \
        Infix *from = new Infix(symbol, ldecl, rdecl);                  \
        Name *to = new Name(symbol);                                    \
        c->EnterRewrite(from, to);                                      \
        to->code = (eval_fn) xl_##name;                                 \
        compiler->EnterBuiltin("xl_" #name, from, to->code);            \
    } while(0);

#define PARM(symbol, type)                                      \
        Infix *symbol##_decl = new Infix(":",                   \
                                         new Name(#symbol),     \
                                         new Name(#type));      \
        parameters.push_back(symbol##_decl);

#define MPARM(symbol, type) PARM(symbol, type)

#define PREFIX(symbol, parms, name, _code)                              \
    do                                                                  \
    {                                                                   \
        tree_list parameters;                                           \
        parms;                                                          \
        if (parameters.size())                                          \
        {                                                               \
            Tree *parmtree = ParametersTree(parameters);                \
            Prefix *from = new Prefix(new Name(symbol), parmtree);      \
            Name *to = new Name(symbol);                                \
            c->EnterRewrite(from, to);                                  \
            to->code = (eval_fn) xl_##name;                             \
            compiler->EnterBuiltin("xl_" #name, from, to->code);        \
        }                                                               \
        else                                                            \
        {                                                               \
            Name *n  = new Name(#symbol);                               \
            n->code = (eval_fn) xl_##name;                              \
            c->EnterName(symbol, n);                                    \
        }                                                               \
    } while(0);

#define POSTFIX(t1, symbol, name, _code)                        \
    do                                                          \
    {                                                           \
        tree_list parameters;                                   \
        parms;                                                  \
        Tree *parmtree = ParametersTree(parameters);            \
        Prefix *from = new Postfix(parmtree, new Name(symbol)); \
        Name *to = new Name(symbol);                            \
        c->EnterRewrite(from, to);                              \
        to->code = (eval_fn) xl_##name;                         \
        compiler->EnterBuiltin("xl_" #name, from, to->code);    \
    } while(0);

#define NAME(symbol)                            \
    do                                          \
    {                                           \
        Name *n = new Name(#symbol);            \
        n->code = xl_identity;                  \
        c->EnterName(#symbol, n);               \
        xl_##symbol = n;                        \
    } while (0);

#define TYPE(symbol)                            \
    do                                          \
    {                                           \
        Name *n = new Name(#symbol);            \
        n->code = (eval_fn) xl_##symbol;        \
        c->EnterName(#symbol, n);               \
        xl_##symbol##_name = n;                 \
    } while(0);
