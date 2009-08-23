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
    std::vector<text> files;
    std::vector<text>::iterator file;
    bool hadError = false;

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

    // Scan options and build list of files we need to process
    files.push_back("builtins.xl");
    for (cmd = options.Parse(argc, argv); cmd != end; cmd = options.ParseNext())
        files.push_back(cmd);

    // Loop over files we will process
    for (file = files.begin(); file != files.end(); file++)
    {
        // Get the individual command line file
        cmd = *file;

        // Parse and execute program
        XL::Parser parser (cmd.c_str(), syntax, positions, errors);
        XL::Tree *tree = parser.Parse();
        if (!tree)
        {
            hadError = true;
            break;           // File read error, message already emitted
        }

        new XL::TreeRoot(tree);
        context.CollectGarbage();
        IFTRACE(source)
            std::cout << "SOURCE:\n" << tree << "\n";
        if (options.parseOnly || options.compileOnly)
        {
            if (!options.parseOnly)
            {
                tree = context.CompileAll(tree);
                if (tree && !options.compileOnly)
                    tree = context.Run(tree);
                if (!tree)
                {
                    hadError = true;
                    break;
                }
            }
        }
        else if (tree)
        {
            XL::Tree *runnable = context.CompileAll(tree);
            if (!runnable)
            {
                hadError = true;
                break;
            }
            new XL::TreeRoot(runnable);
            tree = context.Run(runnable);
        }

        if (file != files.begin() || options.verbose)
        {
            if (options.verbose)
                debugp(tree);
            else
                std::cout << tree << "\n";
        }
    }

#if CONFIG_USE_SBRK
        IFTRACE(memory)
            fprintf(stderr, "Total memory usage: %ldK\n",
                    long ((char *) malloc(1) - low_water) / 1024);
#endif

    return 0;
}
