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
#include <unistd.h>
#include <stdlib.h>
#include <map>
#include <iostream>
#include "scanner.h"
#include "parser.h"
#include "renderer.h"
#include "errors.h"
#include "tree.h"
#include "context.h"
#include "compiler.h"
#include "options.h"
#include "basics.h"


int main(int argc, char **argv)
// ----------------------------------------------------------------------------
//   Parse the command line and run the compiler phases
// ----------------------------------------------------------------------------
{
#if CONFIG_USE_SBRK
    char *low_water = (char *) sbrk(0);
#endif
    XL::Syntax syntax;
    XL::Positions positions;
    XL::Errors errors(&positions);
    XL::Options options(errors);
    XL::command_line_options = &options;
    XL::Compiler compiler("xl_tao");
    XL::Context context(errors, &compiler);
    text cmd, end = "";

    // Make sure debug function is linked in...
    if (!low_water)
        debug(NULL);

    // Initialize basic XL syntax from syntax description file
    syntax.ReadSyntaxFile("xl.syntax");

    // Initialize basic rendering engine
    XL::Renderer renderer(std::cout, "xl.stylesheet", syntax);
    XL::Renderer::renderer = &renderer;
    XL::Context::context = &context;

    // Initialize basics
    XL::EnterBasics(&context);

    for (cmd = options.Parse(argc, argv);
         cmd != end;
         cmd = options.ParseNext())
    {
        XL::Parser parser (cmd.c_str(), syntax, positions, errors);
        XL::Tree *tree = parser.Parse();
        if (!tree)
            break;           // File read error, message already emitted

        context.Root(tree);
        context.CollectGarbage();
        IFTRACE(source)
            std::cout << "SOURCE:\n" << tree << "\n";
        if (options.parseOnly || options.compileOnly)
        {
            if (tree && !options.parseOnly)
            {
                tree = context.CompileAll(tree);
                if (tree && !options.compileOnly)
                    tree = context.Run(tree);
            }
        }
        else if (tree)
        {
            tree = context.CompileAll(tree);
            tree = context.Run(tree);
        }

        if (options.verbose)
            debugp(tree);
        else
            std::cout << tree << "\n";
    }

#if CONFIG_USE_SBRK
        IFTRACE(memory)
            fprintf(stderr, "Total memory usage: %ldK\n",
                    long ((char *) malloc(1) - low_water) / 1024);
#endif

    return 0;
}
