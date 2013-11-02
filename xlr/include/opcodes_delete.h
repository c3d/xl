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
