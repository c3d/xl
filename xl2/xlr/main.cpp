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
#include "main.h"
#include "scanner.h"
#include "parser.h"
#include "renderer.h"
#include "errors.h"
#include "tree.h"
#include "context.h"
#include "compiler.h"
#include "options.h"
#include "basics.h"


XL_BEGIN

Main *MAIN = NULL;


Main::Main(int inArgc, char **inArgv)
// ----------------------------------------------------------------------------
//   Initialization of the globals
// ----------------------------------------------------------------------------
    : argc(inArgc), argv(inArgv),
      positions(),
      errors(&positions),
      syntax("xl.syntax"),
      options(errors),
      compiler("xl_tao"),
      context(errors, &compiler),
      renderer(std::cout, "xl.stylesheet", syntax)
{}


int Main::Run()
// ----------------------------------------------------------------------------
//   The main exection loop
// ----------------------------------------------------------------------------
{
    text cmd, end = "";
    std::vector<text> files;
    std::vector<text>::iterator file;
    bool hadError = false;

    // Make sure debug function is linked in...
    if (getenv("SHOW_INITIAL_DEBUG"))
        debug(NULL);

    // Initialize the locale
    if (!setlocale(LC_CTYPE, ""))
        std::cerr << "WARNING: Cannot set locale.\n"
                  << "         Check LANG, LC_CTYPE, LC_ALL.\n";

    // Initialize basics
    EnterBasics(&context);

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
        Parser parser (cmd.c_str(), syntax, positions, errors);
        Tree *tree = parser.Parse();
        if (!tree)
        {
            hadError = true;
            break;           // File read error, message already emitted
        }

        new TreeRoot(tree);
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
            Tree *runnable = context.CompileAll(tree);
            if (!runnable)
            {
                hadError = true;
                break;
            }
            new XL::TreeRoot(runnable);
            tree = context.Run(runnable);
        }

        if (isBuiltin)
            isBuiltin = false;
        if (file != files.begin() || options.verbose)
        {
            if (options.verbose)
                debugp(tree);
            else
                std::cout << tree << "\n";
        }
    }

    return 0;
}

XL_END


int main(int argc, char **argv)
// ----------------------------------------------------------------------------
//   Parse the command line and run the compiler phases
// ----------------------------------------------------------------------------
{
#if CONFIG_USE_SBRK
    char *low_water = (char *) sbrk(0);
#endif

    using namespace XL;
    MAIN = new Main(argc, argv);
    int rc = MAIN->Run();
    delete MAIN;

#if CONFIG_USE_SBRK
    IFTRACE(memory)
        fprintf(stderr, "Total memory usage: %ldK\n",
                long ((char *) malloc(1) - low_water) / 1024);
#endif

    return rc;
}
