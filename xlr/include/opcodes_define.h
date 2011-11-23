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
#undef RETURNS
#undef GROUP
#undef SYNOPSIS
#undef DESCRIPTION
#undef SEE

#ifndef XL_SCOPE
#define XL_SCOPE "xl_"
#endif // XL_SCOPE

#if GENERATE_DOCUMENTATION_FOR_TBL_FILES // Now disabled, see #1639
#define RETURNS(rtype, rdoc)                                            \
                          returns = " return_value \"" #rtype "\", <<"; \
                          returns.append(rdoc).append(">>\n");
#define GROUP(grp)        docgroup = #grp;
#define SYNOPSIS(syno)    synopsis = syno;
#define DESCRIPTION(desc) description = desc;
#define SEE(see)          seealso = see;

#define DOC(name, syntax, docinfo)                                      \
    text returns, docgroup, docparms, synopsis, description, seealso;   \
    text docsyntax = syntax;                                            \
    do { docinfo; } while (0);                                          \
    text doc = "/*| docname \"" #name "\", \"" + docgroup + "\", do\n"; \
    doc += " dsyntax <<" + docsyntax + ">>\n";                          \
    doc += " synposis <<" + synopsis + ">>\n";                          \
    doc += " description << " + description + ">>\n";                   \
    if (docparms != "")                                                 \
        doc.append(" parameters\n").append(docparms).append("\n");      \
    if (returns != "")                                                  \
        doc.append(returns);                                            \
    if (seealso != "")                                                  \
        doc.append(" see \"").append(seealso).append("\"\n");           \
    doc.append("|*/");
#define XDOC(x) x
#else
#define RETURNS(rtype, rdoc)
#define GROUP(grp)
#define SYNOPSIS(syno)
#define DESCRIPTION(desc)
#define SEE(see)
#define DOC(name, syntax, docinfo)      kstring doc = ""; docinfo;
#define XDOC(x)
#endif


#define INFIX(name, rtype, t1, symbol, t2, _code, docinfo)              \
    do                                                                  \
    {                                                                   \
        DOC(name, #t1 " " symbol " " #t2, XDOC(docinfo));               \
        xl_enter_infix(context, XL_SCOPE #name, (native_fn) xl_##name,  \
                       rtype##_type, #t1, symbol, #t2, doc);            \
    } while(0);

#define PARM(symbol, type, pdoc)                                        \
    do                                                                  \
    {                                                                   \
        parameters.push_back(xl_parameter(#symbol, #type));             \
        XDOC(                                                           \
        if (docparms != "")                                             \
            docsyntax.append(", ");                                     \
        else                                                            \
            docsyntax.append(" ");                                      \
        docsyntax.append(#symbol);                                      \
        docparms.append(" parameter \"" #type "\", \"" #symbol "\",");  \
        docparms.append(" <<" pdoc ">>\n");)                            \
    } while (0);

#define PREFIX(name, rtype, symbol, parms, _code, docinfo)              \
    do                                                                  \
    {                                                                   \
        TreeList parameters;                                            \
        DOC(name, #symbol, XDOC(docinfo ;) parms);                      \
        xl_enter_prefix(context, XL_SCOPE #name, (native_fn) xl_##name, \
                        rtype##_type, parameters, symbol, doc);         \
    } while(0);

#define POSTFIX(name, rtype, parms, symbol, _code, docinfo)             \
    do                                                                  \
    {                                                                   \
        TreeList  parameters;                                           \
        DOC(name, #symbol,                                              \
            XDOC(docinfo ;) parms XDOC(; docsyntax += " " #symbol));    \
        xl_enter_postfix(context, XL_SCOPE #name,(native_fn) xl_##name, \
                         rtype##_type, parameters, symbol, doc);        \
    } while(0);

#define BLOCK(name, rtype, open, type, close, _code, docinfo)           \
    do                                                                  \
    {                                                                   \
        DOC(name, #open " " #type " " #close, docinfo);                 \
        xl_enter_block(context, XL_SCOPE #name, (native_fn) xl_##name,  \
                       rtype##_type, open, #type, close, doc);    \
    } while (0);

#define FORM(name, rtype, form, parms, _code, docinfo)                  \
    do                                                                  \
    {                                                                   \
        TreeList parameters;                                            \
        DOC(name, form, XDOC(docinfo;) parms);                          \
        xl_enter_form(context, XL_SCOPE #name, (native_fn) xl_##name,   \
                      rtype##_type, form, parameters, doc);             \
    } while(0);

#define NAME(symbol)                            \
    do                                          \
    {                                           \
        Name *n = new Name(#symbol);            \
        xl_##symbol = n;                        \
        xl_enter_global(MAIN, n, &xl_##symbol); \
        context->Define(n, n);                  \
        xl_enter_name(MAIN->globals, n);        \
    } while (0);


#define TYPE(symbol)                                                    \
    do                                                                  \
    {                                                                   \
        /* Type alone evaluates as self */                              \
        Name *n = new Name(#symbol);                                    \
        symbol##_type = n;                                              \
        xl_enter_global(MAIN, n, &symbol##_type);                       \
        context->Define(n, n);                                          \
        xl_enter_type(MAIN->globals, n,                                 \
                      "xl_" #symbol "_cast", xl_##symbol##_cast);       \
    } while(0);
