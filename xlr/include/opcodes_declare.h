// *****************************************************************************
// opcodes_declare.h                                                  XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010, Catherine Burvelle <catherine@taodyne.com>
// (C) 2009-2011,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
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

