// ****************************************************************************
//  opcodes_delete.h                                               XLR project
// ****************************************************************************
//
//   File Description:
//
//     Macros used to delete built-ins.
//
//     Usage:
//     #include "opcodes_delete.h"
//     #include "builtins.tbl"
//
//
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Jerome Forissier <jerome@taodyne.com>
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
