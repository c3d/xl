// ****************************************************************************
//  opcodes_declare.h               (C) 1992-2009 Christophe de Dinechin (ddd) 
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
#undef DS
#undef RS

#ifndef XL_SCOPE
#define XL_SCOPE "xl_"
#endif // XL_SCOPE

#define DS(n) IFTRACE(builtins) std::cerr << "Builtin " #n ": " << self << '\n';


#define INFIX(name, rtype, t1, symbol, t2, _code, doc)                  \
    rtype##_nkp xl_##name(Tree *self,t1##_r l,t2##_r r)                 \
    {                                                                   \
        DS(symbol) _code;                                               \
    }                                                                   \
    rtype##_nkp xli_##name(Context *context, Tree *self,                \
                           t1##_r l,t2##_r r)                           \
    {                                                                   \
        (void) context; DS(symbol) _code;                               \
    }                                                                   \
    static void xl_enter_infix_##name(Symbols *c,                       \
                                      Context *context,                 \
                                      Compiler *compiler)               \
    {                                                                   \
        Infix *ldecl = new Infix(":", new Name("l"), new Name(#t1));    \
        Infix *rdecl = new Infix(":", new Name("r"), new Name(#t2));    \
        Infix *from = new Infix(symbol, ldecl, rdecl);                  \
        Name *to = new Name(symbol);                                    \
        eval_fn fn = (eval_fn) xl_##name;                               \
        setDocumentation(from, doc);                                    \
        Rewrote *rw = c->EnterRewrite(from, to);                        \
        to->code = fn;                                                  \
        to->SetSymbols(c);                                              \
        to->Set<TypeInfo> (rtype##_type);                               \
        compiler->EnterBuiltin(XL_SCOPE #name,                          \
                               to, rw->parameters, fn);                 \
                                                                        \
        Rewrite *rwo = context->Define(from, to);                       \
        rwo->native = (native_fn) xli_##name;                           \
    }

#define PARM(symbol, type)      , type##_r symbol

#define PREFIX(name, rtype, symbol, parms, _code, doc)                  \
    rtype##_nkp xl_##name(Tree *self parms)                             \
    {                                                                   \
        DS(symbol) _code;                                               \
    }                                                                   \
    rtype##_nkp xli_##name(Context *context, Tree *self parms)          \
    {                                                                   \
        (void) context; DS(symbol) _code;                               \
    }                                                                   \
    static void xl_enter_prefix_##name(Symbols *c,                      \
                                       Context *context,                \
                                       Compiler *compiler,              \
                                       TreeList &parameters)            \
    {                                                                   \
        eval_fn fn = (eval_fn) xl_##name;                               \
        if (parameters.size())                                          \
        {                                                               \
            Tree *parmtree = ParametersTree(parameters);                \
            Prefix *from = new Prefix(new Name(symbol), parmtree);      \
            Name *to = new Name(symbol);                                \
            setDocumentation(from, doc);                                \
            Rewrote *rw = c->EnterRewrite(from, to);                    \
            to->code = fn;                                              \
            to->SetSymbols(c);                                          \
            to->Set<TypeInfo> (rtype##_type);                           \
            compiler->EnterBuiltin(XL_SCOPE #name,                      \
                                   to, rw->parameters, fn);             \
                                                                        \
            Rewrite *rwo = context->Define(from, to);                   \
            rwo->native = (native_fn) xli_##name;                       \
        }                                                               \
        else                                                            \
        {                                                               \
            Name *n  = new Name(symbol);                                \
            n->code = fn;                                               \
            n->SetSymbols (c);                                          \
            n ->Set<TypeInfo> (rtype##_type);                           \
            setDocumentation(n, doc);                                   \
            c->EnterName(symbol, n);                                    \
            TreeList noparms;                                           \
            compiler->EnterBuiltin(XL_SCOPE #name, n, noparms, fn);     \
                                                                        \
            Rewrite *rwo = context->Define(n, n);                       \
            rwo->native = (native_fn) xli_##name;                       \
        }                                                               \
    }


#define POSTFIX(name, rtype, parms, symbol, _code, doc)                 \
    rtype##_nkp xl_##name(Tree *self parms)                             \
    {                                                                   \
        DS(symbol) _code;                                               \
    }                                                                   \
    rtype##_nkp xli_##name(Context *context, Tree *self parms)          \
    {                                                                   \
        (void) context; DS(symbol) _code;                               \
    }                                                                   \
                                                                        \
    static void xl_enter_postfix_##name(Symbols *c,                     \
                                        Context *context,               \
                                        Compiler *compiler,             \
                                        TreeList &parameters)           \
    {                                                                   \
        Tree *parmtree = ParametersTree(parameters);                    \
        Postfix *from = new Postfix(parmtree, new Name(symbol));        \
        Name *to = new Name(symbol);                                    \
        eval_fn fn = (eval_fn) xl_##name;                               \
        setDocumentation(from, doc);                                    \
        Rewrote *rw = c->EnterRewrite(from, to);                        \
        to->code = fn;                                                  \
        to->SetSymbols(c);                                              \
        to->Set<TypeInfo> (rtype##_type);                               \
        compiler->EnterBuiltin(XL_SCOPE #name,                          \
                               to, rw->parameters, to->code);           \
                                                                        \
        Rewrite *rwo = context->Define(from, to);                       \
        rwo->native = (native_fn) xli_##name;                           \
    }


#define BLOCK(name, rtype, open, type, close, _code, doc)               \
    rtype##_nkp xl_##name(Tree *self, type##_r child)                   \
    {                                                                   \
        DS(symbol) _code;                                               \
    }                                                                   \
    rtype##_nkp xli_##name(Context *context,                            \
                          Tree *self, type##_r child)                   \
    {                                                                   \
        (void) context; DS(symbol) _code;                               \
    }                                                                   \
    static void xl_enter_block_##name(Symbols *c,                       \
                                      Context *context,                 \
                                      Compiler *compiler)               \
    {                                                                   \
        Infix *parms = new Infix(":", new Name("V"), new Name(#type));  \
        Block *from = new Block(parms, open, close);                    \
        Name *to = new Name(#name);                                     \
        eval_fn fn = (eval_fn) xl_##name;                               \
        setDocumentation(from, doc);                                    \
        Rewrote *rw = c->EnterRewrite(from, to);                        \
        to->code = fn;                                                  \
        to->SetSymbols(c);                                              \
        to->Set<TypeInfo> (rtype##_type);                               \
        compiler->EnterBuiltin(XL_SCOPE #name, to,                      \
                               rw->parameters, to->code);               \
                                                                        \
        Rewrite *rwo = context->Define(from, to);                       \
        rwo->native = (native_fn) xli_##name;                           \
    }

#define NAME(symbol)    Name_p xl_##symbol;

#define TYPE(symbol)    Name_p symbol##_type;

