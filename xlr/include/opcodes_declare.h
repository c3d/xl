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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
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
#undef RETURNS
#undef GROUP
#undef SYNOPSIS
#undef DESCRIPTION
#undef MISC
#undef SEE

#ifndef XL_SCOPE
#define XL_SCOPE "xl_"
#endif // XL_SCOPE

#define DS(n) IFTRACE(builtins) std::cerr << "Builtin " #n ": " << self << '\n';

#define GROUP(grp)
#define SYNOPSIS(syno)
#define DESCRIPTION(desc)
#define RETURNS(rytpe, rdoc)
#define MISC(misc)
#define SEE(see)

#define INFIX(name, rtype, t1, symbol, t2, _code, docinfo)              \
    rtype##_nkp xl_##name(Context *context, Tree *self,                 \
                           t1##_r l,t2##_r r)                           \
    {                                                                   \
        (void) context; DS(symbol) _code;                               \
    }                                                                   \
    static void xl_enter_infix_##name(Context *context, text doc)       \
    {                                                                   \
        Tree *ldecl = xl_parameter("l", #t1);                           \
        Tree *rdecl = xl_parameter("r", #t2);                           \
        Infix *from = new Infix(symbol, ldecl, rdecl);                  \
        Name *to = new Name(symbol);                                    \
        setDocumentation(from, doc);                                    \
        Rewrite *rw = context->Define(from, to);                        \
        rw->native = (native_fn) xl_##name;                             \
    }

#define PARM(symbol, type, pdoc)      , type##_r symbol

#define PREFIX(name, rtype, symbol, parms, _code, docinfo)              \
    rtype##_nkp xl_##name(Context *context, Tree *self parms)           \
    {                                                                   \
        (void) context; DS(symbol) _code;                               \
    }                                                                   \
    static void xl_enter_prefix_##name(Context *context,                \
                                       TreeList &parameters,            \
                                       text doc)                        \
    {                                                                   \
        if (parameters.size())                                          \
        {                                                               \
            Tree *parmtree = ParametersTree(parameters);                \
            Prefix *from = new Prefix(new Name(symbol), parmtree);      \
            Name *to = new Name(symbol);                                \
            setDocumentation(from, doc);                                \
            to->Set<TypeInfo> (rtype##_type);                           \
            Rewrite *rw = context->Define(from, to);                    \
            rw->native = (native_fn) xl_##name;                         \
        }                                                               \
        else                                                            \
        {                                                               \
            Name *n  = new Name(symbol);                                \
            n ->Set<TypeInfo> (rtype##_type);                           \
            setDocumentation(n, doc);                                   \
            Rewrite *rw = context->Define(n, n);                        \
            rw->native = (native_fn) xl_##name;                         \
        }                                                               \
    }

#define POSTFIX(name, rtype, parms, symbol, _code, docinfo)             \
    rtype##_nkp xl_##name(Context *context, Tree *self parms)           \
    {                                                                   \
        (void) context; DS(symbol) _code;                               \
    }                                                                   \
                                                                        \
    static void xl_enter_postfix_##name(Context *context,               \
                                        TreeList &parameters,           \
                                        text doc)                       \
    {                                                                   \
        Tree *parmtree = ParametersTree(parameters);                    \
        Postfix *from = new Postfix(parmtree, new Name(symbol));        \
        Name *to = new Name(symbol);                                    \
        setDocumentation(from, doc);                                    \
        to->Set<TypeInfo> (rtype##_type);                               \
        Rewrite *rw = context->Define(from, to);                        \
        rw->native = (native_fn) xl_##name;                             \
    }


#define BLOCK(name, rtype, open, type, close, _code, docinfo)           \
    rtype##_nkp xl_##name(Context *context,                             \
                          Tree *self, type##_r child)                   \
    {                                                                   \
        (void) context; DS(symbol) _code;                               \
    }                                                                   \
    static void xl_enter_block_##name(Context *context, text doc)       \
    {                                                                   \
        Tree *parms = xl_parameter("c", #type);                         \
        Block *from = new Block(parms, open, close);                    \
        Name *to = new Name(#name);                                     \
        setDocumentation(from, doc);                                    \
        to->Set<TypeInfo> (rtype##_type);                               \
        /* Need an extra block that gets removed by Define */           \
        from = new Block(from, open, close);                            \
        Rewrite *rw = context->Define(from, to);                        \
        rw->native = (native_fn) xl_##name;                             \
    }

#define NAME(symbol)    Name_p xl_##symbol;

#define TYPE(symbol)    Name_p symbol##_type;

