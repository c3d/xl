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

#undef INFIX
#undef PREFIX
#undef POSTFIX
#undef BLOCK
#undef FORM
#undef NAME
#undef TYPE
#undef PARM
#undef DS
#undef RS
#undef RETURNS
#undef GROUP
#undef SYNOPSIS
#undef DESCRIPTION
#undef SEE

#ifndef XL_SCOPE
#define XL_SCOPE "xl_"
#endif // XL_SCOPE

#define DS(n) IFTRACE(builtins) std::cerr << "Builtin " #n ": " << self << '\n';

#define GROUP(grp)
#define SYNOPSIS(syno)
#define DESCRIPTION(desc)
#define RETURNS(rytpe, rdoc)
#define SEE(see)

#define INFIX(name, rtype, t1, symbol, t2, _code, docinfo)              \
    rtype##_nkp xl_##name(Context *context, Tree *self,                 \
                           t1##_r l,t2##_r r)                           \
    {                                                                   \
        (void) context; DS(symbol) _code;                               \
    }

#define PARM(symbol, type, pdoc)      , type##_r symbol

#define PREFIX(name, rtype, symbol, parms, _code, docinfo)              \
    rtype##_nkp xl_##name(Context *context, Tree *self parms)           \
    {                                                                   \
        (void) context; DS(symbol) _code;                               \
    }                                                                   \

#define POSTFIX(name, rtype, parms, symbol, _code, docinfo)             \
    rtype##_nkp xl_##name(Context *context, Tree *self parms)           \
    {                                                                   \
        (void) context; DS(symbol) _code;                               \
    }

#define BLOCK(name, rtype, open, type, close, _code, docinfo)           \
    rtype##_nkp xl_##name(Context *context,                             \
                          Tree *self, type##_r child)                   \
    {                                                                   \
        (void) context; DS(symbol) _code;                               \
    }

#define FORM(name, rtype, form, parms, _code, docinfo)          \
    rtype##_nkp xl_##name(Context *context, Tree *self parms)   \
    {                                                           \
        (void) context; DS(symbol) _code;                       \
    }

#define NAME(symbol)    Name_p xl_##symbol;

#define TYPE(symbol)    Name_p symbol##_type;

