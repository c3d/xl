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
#include "renderer.h"
#include "errors.h"
#include "tree.h"
#include "options.h"

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
    text cmd, end = "";

    // Make sure debug function is linked in...
    if (!low_water)
        debug(NULL);

    // Initialize basic XL syntax from syntax description file
    syntax.ReadSyntaxFile("xl.syntax");

    XL::command_line_options = &options;
    for (cmd = options.Parse(argc, argv);
         cmd != end;
         cmd = options.ParseNext())
    {
        XL::Parser parser (cmd.c_str(), syntax, positions, errors);
        XL::Tree *tree = parser.Parse();

        IFTRACE(source)
        {
            std::cout << tree << "\n";
        }
        IFTRACE(parse)
        {
            std::cout << tree << "\n";
        }
    }

#if CONFIG_USE_SBRK
        IFTRACE(timing)
            fprintf(stderr, "Total memory usage: %ldK\n",
                    long ((char *) malloc(1) - low_water) / 1024);
#endif

    return 0;
}
