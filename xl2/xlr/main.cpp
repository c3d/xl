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
#include <stdlib.h>
#endif
#include <map>
#include <iostream>
#include "scanner.h"
#include "parser.h"
#include "tree.h"
#include "options.h"


struct XLInitializeSyntax 
// ----------------------------------------------------------------------------
//   For debugging: emit the things of interest
// ----------------------------------------------------------------------------
{
    enum WhereAmI { inUnknown, inPrefix, inInfix, inPostfix,
                    inComment, inCommentDef,
                    inText, inTextDef,
                    inBlock, inBlockDef };

    XLInitializeSyntax(XLSyntax &ctx):
        syntax(ctx), priority(0), whereami(inUnknown) {}
    
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
            syntax.statement_priority = priority;
        }
        else if (txt == text("FUNCTION"))
        {
            syntax.function_priority = priority;
        }
        else if (txt == text("DEFAULT"))
        {
            syntax.default_priority = priority;
        }
        else switch (whereami)
        {
        case inPrefix:
            syntax.SetPrefixPriority(txt, priority);
            break;
        case inInfix:
            syntax.SetInfixPriority(txt, priority);
            break;
        case inPostfix:
            // Ignored
            break;
        case inComment:
            entry = txt;
            whereami = inCommentDef;
            break;
        case inCommentDef:
            syntax.Comment(entry, txt);
            whereami = inComment;
            break;
        case inText:
            entry = txt;
            whereami = inTextDef;
            break;
        case inTextDef:
            syntax.TextDelimiter(entry, txt);
            whereami = inComment;
            break;
        case inBlock:
            entry = txt;
            whereami = inBlockDef;
            syntax.SetInfixPriority(entry, priority);
            break;
        case inBlockDef:
            syntax.Block(entry, txt);
            syntax.Block(txt, ""); // Mark to indentify single-char blocks
            whereami = inBlock;
            break;
        case inUnknown:
            std::cerr << "WARNING: Invalid syntax table format: "
                      << txt << '\n';
            break;
        }
        return false;
    }

    XLSyntax & syntax;
    int priority;
    WhereAmI whereami;
    text entry;
};


void ReadSyntax(kstring file, XLSyntax &syntax)
// ----------------------------------------------------------------------------
//   Parse the syntax description table
// ----------------------------------------------------------------------------
{
    XLSyntax syntaxTable;

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
    XLInitializeSyntax init(syntax);
    // XLDo<XLInitializeSyntax> doInit(init);
    // doInit(tree);

    // Cheat grossly, now that the C++ parser behaves differently from the
    // XL versions with respect to block priority...
    syntax.SetInfixPriority(INDENT_MARKER, 400);
}

int main(int argc, char **argv)
// ----------------------------------------------------------------------------
//   Parse the command line and run the compiler phases
// ----------------------------------------------------------------------------
{
#if CONFIG_USE_SBRK
    char *low_water = (char *) sbrk(0);
#endif
    XLSyntax syntax;
    text cmd, end = "";

    // Make sure debug function is linked in...
    debug(NULL);

    // Initialize basic XL syntax from syntax description file
    ReadSyntax("xl.syntax", syntax);

    for (cmd = gOptions.Parse(argc, argv);
         cmd != end;
         cmd = gOptions.ParseNext())
    {
        XLParser parser (cmd.c_str(), &syntax);
        XLTree *tree = parser.Parse();

        IFTRACE(source)
        {
            // std::cout << *tree << "\n";
        }
        IFTRACE(parse)
        {
            // XLTree::outputDebug = true;
            // std::cout << *tree << "\n";
        }
    }

#if CONFIG_USE_SBRK
        IFTRACE(timing)
            fprintf(stderr, "Total memory usage: %ldK\n",
                    long ((char *) malloc(1) - low_water) / 1024);
#endif

    return 0;
}
