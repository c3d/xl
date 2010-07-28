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
#undef BLOCK
#undef NAME
#undef TYPE
#undef PARM

#ifndef XL_SCOPE
#define XL_SCOPE "xl_"
#endif // XL_SCOPE


#define INFIX(name, rtype, t1, symbol, t2, _code)       \
    xl_enter_infix_##name(c, compiler);



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


#define PREFIX(name, rtype, symbol, parms, _code)               \
    do                                                          \
    {                                                           \
        TreeList parameters;                                    \
        parms;                                                  \
        xl_enter_prefix_##name(c, compiler, parameters);        \
    } while(0);


#define POSTFIX(name, rtype, parms, symbol, _code)              \
    do                                                          \
    {                                                           \
        TreeList  parameters;                                   \
        parms;                                                  \
        xl_enter_postfix_##name(c, compiler, parameters);       \
    } while(0);


#define BLOCK(name, rtype, open, type, close, _code)    \
    xl_enter_block_##name(c, compiler);


#define NAME(symbol)                            \
    do                                          \
    {                                           \
        Name *n = new Name(#symbol);            \
        n->code = xl_identity;                  \
        n->SetSymbols(c);                       \
        c->EnterName(#symbol, n);               \
        xl_##symbol = n;                        \
        compiler->EnterGlobal(n, &xl_##symbol); \
    } while (0);


#define TYPE(symbol)                                                    \
    do                                                                  \
    {                                                                   \
        /* Type alone evaluates as self */                              \
        Name *n = new Name(#symbol);                                    \
        eval_fn fn = (eval_fn) xl_identity;                             \
        n->code = fn;                                                   \
        n->SetSymbols(c);                                               \
        c->EnterName(#symbol, n);                                       \
        symbol##_type = n;                                              \
        compiler->EnterGlobal(n, &symbol##_type);                       \
                                                                        \
        /* Type as infix : evaluates to type check, e.g. 0 : integer */ \
        Infix *from = new Infix(":", new Name("V"), new Name(#symbol)); \
        Name *to = new Name(#symbol);                                   \
        Rewrite *rw = c->EnterRewrite(from, to);                        \
        eval_fn typeTestFn = (eval_fn) xl_##symbol##_cast;              \
        to->code = typeTestFn;                                          \
        to->SetSymbols(c);                                              \
        compiler->EnterBuiltin(XL_SCOPE #symbol,                        \
                               to, rw->parameters, typeTestFn);         \
                                                                        \
    } while(0);
