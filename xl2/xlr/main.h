#ifndef MAIN_H
#define MAIN_H
// ****************************************************************************
//  main.h                          (C) 1992-2009 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     The global variables defined in main.cpp
// 
// 
// 
// 
// 
// 
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

#include "tree.h"
#include "scanner.h"
#include "parser.h"
#include "renderer.h"
#include "errors.h"
#include "syntax.h"
#include "context.h"
#include "compiler.h"
#include "options.h"
#include <map>

XL_BEGIN

struct SourceFile
// ----------------------------------------------------------------------------
//    A source file and associated data
// ----------------------------------------------------------------------------
{
    SourceFile(text n, Tree *t, Symbols *s):
        name(n), tree(t), symbols(s), changed(false) {}
    SourceFile(): name(""), tree(NULL), symbols(NULL), changed(false) {}
    text        name;
    TreeRoot    tree;
    Symbols *   symbols;
    bool        changed;
};
typedef std::map<text, SourceFile> source_files;


struct Main
// ----------------------------------------------------------------------------
//    The main entry point and associated data
// ----------------------------------------------------------------------------
{
    Main(int argc, char **argv);

    int          argc;
    char **      argv;

    Positions    positions;
    Errors       errors;
    Syntax       syntax;
    Options      options;
    Compiler     compiler;
    Context      context;
    Renderer     renderer;
    source_files files;

    int Run();
};

extern Main *MAIN;

XL_END

#endif // MAIN_H
