// *****************************************************************************
// include/opcodes_delete.h                                           XL project
// *****************************************************************************
//
// File description:
//
//     Macros used to delete built-ins.
//
//     Usage:
//     #include "opcodes_delete.h"
//     #include "builtins.tbl"
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010, Catherine Burvelle <catherine@taodyne.com>
// (C) 2010-2011,2019, Christophe de Dinechin <christophe@dinechin.org>
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
#undef RETURNS
#undef GROUP
#undef SYNOPSIS
#undef DESCRIPTION
#undef SEE

#define SEE(see)
#define RETURNS(type, rdoc)
#define GROUP(grp)
#define SYNOPSIS(syno)
#define DESCRIPTION(desc)

#define INFIX(name, rtype, t1, symbol, t2, _code, docinfo)
#define PARM(symbol, type, pdoc)
#define PREFIX(name, rtype, symbol, parms, _code, docinfo)
#define POSTFIX(name, rtype, parms, symbol, _code, docinfo)
#define BLOCK(name, rtype, open, type, close, _code, docinfo)
#define FORM(name, rtype, form, parms, _code, docinfo)
#define NAME(symbol)    xl_##symbol = 0;
#define TYPE(symbol)    symbol##_type = 0;
