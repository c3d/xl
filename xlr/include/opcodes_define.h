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
#undef DOC_RET
#undef DOC_GROUP
#undef DOC_SYNOPSIS
#undef DOC_DESCRIPTION
#undef DOC_MISC

#ifndef XL_SCOPE
#define XL_SCOPE "xl_"
#endif // XL_SCOPE

#define DOC_RET(rtype, rdoc)  doc+=text("  return_value \"" #rtype "\", <<")+rdoc+">>\n"
#define DOC_GROUP(grp)        #grp
#define DOC_SYNOPSIS(syno)    syno
#define DOC_DESCRIPTION(desc) desc
#define DOC_MISC(misc)        misc


#define INFIX(name, rtype, t1, symbol, t2, _code, group, synopsis, desc, retdoc, misc)  \
    do {                                                                          \
         text doc = text("/*| docname \"" #name "\", \"") + group + "\", do \n";  \
         doc += text("  dsyntax \" "#t1" ") + symbol + " "#t2" \"\n";             \
         doc += text("  synopsis <<") + synopsis + ">>\n";                        \
         doc += text("  description <<") + desc + ">>\n";                         \
         retdoc;                                                                  \
         doc += text(misc) + "|*/";                                               \
        xl_enter_infix_##name(c, main, doc);                                      \
    } while (0);



#define PARM(symbol, type, pdoc)                                \
    do {                                                        \
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
        }                                                       \
        syntaxe +=  text( #symbol ) + ", ";                     \
        param_desc += text("  parameter \"" #type "\", \"" #symbol "\", <<")+pdoc+">>\n";\
    } while (0);


#define PREFIX(name, rtype, symbol, parms, _code, group, synopsis, desc, retdoc, misc)   \
    do                                                                             \
    {                                                                              \
        text doc = text("/*| docname \"" #name "\", \"") + group + "\", do \n";    \
        text syntaxe = text("  dsyntax \"") + symbol + " ";                        \
        text param_desc = "";                                                      \
        TreeList parameters;                                                       \
        parms;                                                                     \
        bool paramPresent = param_desc.length();                                   \
        if (paramPresent)                                                          \
            syntaxe.resize(syntaxe.length() - 2);                                  \
        doc += syntaxe + "\"\n";                                                   \
        doc +=  text("  synopsis <<") + synopsis + ">> \n";                        \
        doc +=  text("  description <<") + desc + ">> \n";                         \
        if (paramPresent)                                                          \
        {                                                                          \
            doc +=  "  parameters \n";                                             \
            doc += param_desc;                                                     \
        }                                                                          \
        retdoc;                                                                    \
        doc += text(misc) + " |*/";                                                \
        xl_enter_prefix_##name(c, main, parameters, doc);                          \
    } while(0);


#define POSTFIX(name, rtype, parms, symbol, _code, group, synopsis, desc, retdoc, misc)  \
    do                                                                             \
    {                                                                              \
        text doc = text("/*| docname \"" #name "\", \"") + group + "\", do \n";    \
        text syntaxe = "  dsyntax \"";                                             \
        text param_desc = "";                                                      \
        TreeList  parameters;                                                      \
        parms;                                                                     \
        bool paramPresent = param_desc.length();                                   \
        if (paramPresent)                                                          \
            syntaxe.resize(syntaxe.length() - 2);                                  \
        doc += syntaxe + " " + symbol + "\"\n";                                    \
        doc += text("  synopsis <<") + synopsis + ">>\n";                          \
        doc += text("  description <<") + desc + ">>\n";                           \
        doc += "  parameters \n";                                                  \
        doc += param_desc;                                                         \
        retdoc;                                                                    \
        doc += text(misc) + " |*/";                                                \
        xl_enter_postfix_##name(c, main, parameters, doc);                         \
    } while(0);


#define BLOCK(name, rtype, open, type, close, _code, group, synopsis, desc, retdoc, misc) \
    do {                                                                            \
         text doc = text("/*| docname \"" #name "\", \"") + group + "\", do \n";    \
         doc += "  dsyntax \"" #name " " #open " " #type " " #close "\" \n";        \
         doc +=  text("  synopsis <<") + synopsis + ">> \n";                        \
         doc +=  text("  description <<") + desc + ">> \n";                         \
         retdoc;                                                                    \
         doc += text(misc) + "|*/";                                                 \
         xl_enter_block_##name(c, main, doc);                                       \
     } while (0);


#define NAME(symbol)                            \
    do                                          \
    {                                           \
        Name *n = new Name(#symbol);            \
        n->code = xl_identity;                  \
        n->SetSymbols(c);                       \
        c->EnterName(#symbol, n);               \
        xl_##symbol = n;                        \
        xl_enter_global(main, n, &xl_##symbol); \
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
        xl_enter_global(main, n, &symbol##_type);                       \
                                                                        \
        /* Type as infix : evaluates to type check, e.g. 0 : integer */ \
        Infix *from = new Infix(":", new Name("V"), new Name(#symbol)); \
        Name *to = new Name(#symbol);                                   \
        Rewrite *rw = c->EnterRewrite(from, to);                        \
        eval_fn typeTestFn = (eval_fn) xl_##symbol##_cast;              \
        to->code = typeTestFn;                                          \
        to->SetSymbols(c);                                              \
        xl_enter_builtin(main, XL_SCOPE #symbol,                        \
                               to, rw->parameters, typeTestFn);         \
                                                                        \
    } while(0);
