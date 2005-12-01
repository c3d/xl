// ****************************************************************************
//  main.cpp                       (C) 1992-2003 Christophe de Dinechin (ddd)
//                                                                XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//      Main entry point of the compiler
// 
// 
// 
// 
// 
// 
// 
// 
// ****************************************************************************
// This program is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "configuration.h"
#if CONFIG_USE_SBRK
#include <unistd.h>
#endif
#include <map>
#include <iostream>
#include "scanner.h"
#include "parser.h"
#include "tree.h"
#include "ctrans.h"
#include "options.h"


struct XLInitializeContext : XLAction
// ----------------------------------------------------------------------------
//   For debugging: emit the things of interest
// ----------------------------------------------------------------------------
{
    enum WhereAmI { inUnknown, inPrefix, inInfix, inPostfix,
                    inComment, inCommentDef,
                    inText, inTextDef,
                    inBlock, inBlockDef };

    XLInitializeContext(XLContext &ctx):
        context(ctx), priority(0), whereami(inUnknown) {}
    
    bool Name(XLName *input)
    {
        return Enter(input->value);
    }

    bool String(XLString *input)
    {
        return Enter(input->value);
    }

    bool Integer(XLInteger *input)
    {
        priority = int(input->value);
        return false;
    }

    bool Enter(text txt)
    {
        if (txt == text("NEWLINE"))
            txt = "\n";
        else if (txt == text("INDENT"))
            txt = INDENT_MARKER;
        else if (txt == text("UNINDENT"))
            txt = UNINDENT_MARKER;

        if (txt == text("PREFIX"))
        {
            whereami = inPrefix;
        }
        else if (txt == text("INFIX"))
        {
            whereami = inInfix;
        }
        if (txt == text("POSTFIX"))
        {
            whereami = inPostfix;
        }
        else if (txt == text("COMMENT"))
        {
            whereami = inComment;
        }
        else if (txt == text("TEXT"))
        {
            whereami = inText;
        }
        else if (txt == text("BLOCK"))
        {
            whereami = inBlock;
        }
        else if (txt == text("STATEMENT"))
        {
            context.statement_priority = priority;
        }
        else if (txt == text("FUNCTION"))
        {
            context.function_priority = priority;
        }
        else if (txt == text("DEFAULT"))
        {
            context.default_priority = priority;
        }
        else switch (whereami)
        {
        case inPrefix:
            context.SetPrefixPriority(txt, priority);
            break;
        case inInfix:
            context.SetInfixPriority(txt, priority);
            break;
        case inPostfix:
            // Ignored
            break;
        case inComment:
            entry = txt;
            whereami = inCommentDef;
            break;
        case inCommentDef:
            context.Comment(entry, txt);
            whereami = inComment;
            break;
        case inText:
            entry = txt;
            whereami = inTextDef;
            break;
        case inTextDef:
            context.TextDelimiter(entry, txt);
            whereami = inComment;
            break;
        case inBlock:
            entry = txt;
            whereami = inBlockDef;
            context.SetInfixPriority(entry, priority);
            break;
        case inBlockDef:
            context.Block(entry, txt);
            context.Block(txt, ""); // Mark to indentify single-char blocks
            whereami = inBlock;
            break;
        case inUnknown:
            std::cerr << "WARNING: Invalid syntax table format: "
                      << txt << '\n';
            break;
        }
        return false;
    }

    XLContext & context;
    int priority;
    WhereAmI whereami;
    text entry;
};


void ReadContext(kstring file, XLContext &context)
// ----------------------------------------------------------------------------
//   Parse the syntax description table
// ----------------------------------------------------------------------------
{
    XLContext syntaxTable;

    // Enter infix priorities
    syntaxTable /
        // Separator have very low priority
        10 / "\n" /
        20 / "\t" /* Priority for blocks */
        ;

    // Enter comment descriptors
    syntaxTable.Comment("//", "\n");
    
    XLParser parser (file, &syntaxTable);
    XLTree *tree = parser.Parse();

    // At first, I thought it was elegant.
    // In reality, it is darn ugly. But hey, it's just throwaway code
    XLInitializeContext init(context);
    XLDo<XLInitializeContext> doInit(init);
    doInit(tree);

    // Cheat grossly, now that the C++ parser behaves differently from the
    // XL versions with respect to block priority...
    context.SetInfixPriority(INDENT_MARKER, 400);
}

XLContext gContext;

int main(int argc, char **argv)
// ----------------------------------------------------------------------------
//   Parse the command line and run the compiler phases
// ----------------------------------------------------------------------------
{
#if CONFIG_USE_SBRK
    char *low_water = (char *) sbrk(0);
#endif

    text cmd, end = "";

    // Make sure debug function is linked in...
    debug(NULL);

    // Initialize basic XL syntax from syntax description file
    ReadContext("xl.syntax", gContext);

    // Initialize the C translator
    XLInitCTrans();

    for (cmd = gOptions.Parse(argc, argv);
         cmd != end;
         cmd = gOptions.ParseNext())
    {
        XLParser parser (cmd.c_str(), &gContext);
        XLTree *tree = parser.Parse();

        IFTRACE(source)
        {
            std::cout << *tree << "\n";
        }
        IFTRACE(parse)
        {
            XLTree::outputDebug = true;
            std::cout << *tree << "\n";
        }

        XL2C(tree);
    }

#if CONFIG_USE_SBRK
        IFTRACE(timing)
            fprintf(stderr, "Total memory usage: %ldK\n",
                    long ((char *) malloc(1) - low_water) / 1024);
#endif

    return 0;
}
