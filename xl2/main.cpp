// ****************************************************************************
//  main.cpp                        (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                            XL2 project 
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

#include <map>
#include <iostream>
#if CONFIG_USE_SBRK
#   include <unistd.h>
#endif
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
    XLInitializeContext(XLContext &ctx):
        context(ctx), priority(0), isPrefix(0) {}
    
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
        if (txt == text("PREFIX"))
        {
            isPrefix = 1;
        }
        else if (txt == text("INFIX"))
        {
            isPrefix = false;
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
        else
        {
            if (txt == text("NEWLINE"))
                txt = "\n";
            else if (txt == text("BLOCK"))
                txt = "\t";
            if (isPrefix)
            {
                context.SetPrefixPriority(txt, priority);
            }
            else
            {
                context.SetInfixPriority(txt, priority);
            }
        }
        return false;
    }

    XLContext & context;
    int priority;
    int isPrefix;
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

    XLInitializeContext init(context);
    XLDo<XLInitializeContext> doInit(init);
    doInit(tree);
}

XLContext gContext;

int main(int argc, char **argv)
// ----------------------------------------------------------------------------
//   Parse the command line and run the compiler phases
// ----------------------------------------------------------------------------
{
#   if CONFIG_USE_SBRK
        char *low_water = (char *) sbrk(0);
#   endif

    text cmd, end = "";

    // Make sure debug function is linked in...
    debug(NULL);

    // Initialize basic XL syntax from syntax description file
    gContext.Comment("//", "\n");
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

#   if CONFIG_USE_SBRK
        IFTRACE(timing)
            fprintf(stderr, "Total memory usage: %ldK\n",
                    long ((char *) malloc(1) - low_water) / 1024);
#   endif

    return 0;
}
