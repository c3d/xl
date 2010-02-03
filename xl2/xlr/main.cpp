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
#include <stdio.h>
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


Main::Main(int inArgc, char **inArgv, Compiler &comp)
// ----------------------------------------------------------------------------
//   Initialization of the globals
// ----------------------------------------------------------------------------
    : argc(inArgc), argv(inArgv),
      positions(),
      errors(&positions),
      syntax("xl.syntax"),
      options(errors),
      compiler(comp),
      context(errors, &compiler),
      renderer(std::cout, "xl.stylesheet", syntax)
{
    Options::options = &options;
    Context::context = &context;
    Symbols::symbols = &context;
    Renderer::renderer = &renderer;
    Syntax::syntax = &syntax;
}


Main::~Main()
// ----------------------------------------------------------------------------
//   Destructor
// ----------------------------------------------------------------------------
{
}


int Main::Run()
// ----------------------------------------------------------------------------
//   The main exection loop
// ----------------------------------------------------------------------------
{
    text cmd, end = "";
    std::vector<text> filelist;
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
    filelist.push_back("builtins.xl");
    for (cmd = options.Parse(argc, argv); cmd != end; cmd = options.ParseNext())
        filelist.push_back(cmd);

    // Loop over files we will process
    for (file = filelist.begin(); file != filelist.end(); file++)
    {
        Tree *tree = NULL;

        // Get the individual command line file
        cmd = *file;
        if (files.count(cmd) > 0)
        {
            continue;
        }
        else
        {
            // Parse program - Local parser to delete scanner and close file
            // This ensures positions are updated even if there is a 'load'
            // being called during execution.
            Parser parser (cmd.c_str(), syntax, positions, errors);
            tree = parser.Parse();
        }
        if (!tree)
        {
            hadError = true;
            break;           // File read error, message already emitted
        }
        Symbols *syms = new Symbols(context);
        files[cmd] = SourceFile (cmd, tree, syms);
        Symbols::symbols = syms;
        tree->SetSymbols(syms);
        new XL::TreeRoot(tree);

        context.CollectGarbage();
        IFTRACE(source)
            std::cout << "SOURCE:\n" << tree << "\n";
        if (options.parseOnly || options.compileOnly)
        {
            if (!options.parseOnly)
            {
                tree = syms->CompileAll(tree);
                if (tree && !options.compileOnly)
                    tree = syms->Run(tree);
                if (!tree)
                {
                    hadError = true;
                    break;
                }
            }
        }
        else if (tree)
        {
            Tree *runnable = syms->CompileAll(tree);
            if (!runnable)
            {
                hadError = true;
                break;
            }
            tree = context.Run(runnable);
        }

        if (file != filelist.begin() || options.verbose)
        {
            if (options.verbose)
                debugp(tree);
            else
                std::cout << tree << "\n";
        }

        Symbols::symbols = Context::context;
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
    Compiler compiler("xl_tao");
    MAIN = new Main(argc, argv, compiler);
    int rc = MAIN->Run();
    delete MAIN;

#if CONFIG_USE_SBRK
    IFTRACE(memory)
        fprintf(stderr, "Total memory usage: %ldK\n",
                long ((char *) malloc(1) - low_water) / 1024);
#endif

    return rc;
}
