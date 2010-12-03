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
#undef NAME
#undef TYPE
#undef PARM
#undef DOC_RET
#undef DOC_GROUP
#undef DOC_SYNOPSIS
#undef DOC_DESCRIPTION
#undef DOC_MISC

#define DOC_MISC(misc)
#define DOC_RET(type, rdoc)
#define DOC_GROUP(grp)
#define DOC_SYNOPSIS(syno)
#define DOC_DESCRIPTION(desc)

#define INFIX(name, rtype, t1, symbol, t2, _code, group, synopsis, desc, retdoc, misc)

#define PARM(symbol, type, pdoc)

#define PREFIX(name, rtype, symbol, parms, _code, group, synopsis, desc, retdoc, misc)

#define POSTFIX(name, rtype, parms, symbol, _code, group, synopsis, desc, retdoc, misc)

#define BLOCK(name, rtype, open, type, close, _code, group, synopsis, desc, retdoc, misc)

#define NAME(symbol)    xl_##symbol = 0;

#define TYPE(symbol)    symbol##_type = 0;
