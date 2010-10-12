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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
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
#include <set>
#include <time.h>


XL_BEGIN

struct Serializer;
struct Deserializer;


struct SourceFile
// ----------------------------------------------------------------------------
//    A source file and associated data
// ----------------------------------------------------------------------------
{
    SourceFile(text n, Tree *t, Context *c, bool readOnly = false);
    SourceFile();
    text        name;
    Tree_p      tree;
    Context_p   context;
    time_t      modified;
    text        hash;
    bool        changed;
    bool        readOnly;
};
typedef std::map<text, SourceFile> source_files;
typedef std::vector<text> source_names;


struct Main
// ----------------------------------------------------------------------------
//    The main entry point and associated data
// ----------------------------------------------------------------------------
{
    Main(int argc, char **argv, Compiler *comp,
         text syntax = "xl.syntax",
         text style = "xl.stylesheet",
         text builtins = "builtins.xl");
    ~Main();

    int         ParseOptions();
    int         LoadContextFiles(source_names &context_file_names);
    void        EvaluateContextFiles(source_names &context_file_names);
    int         LoadFiles();
    int         LoadFile(text file, bool updateContext = false);
    SourceFile *NewFile(text path);
    text        SearchFile(text input);
    int         Run();
    int         Diff();
    void        Log(Error &e)   { errors->Log(e); }
    Errors *    InitErrorsAndMAIN();
    uint        HadErrors() { return errors->Count(); }

public:
    int          argc;
    char **      argv;

    Positions    positions;
    Errors *     errors;
    Errors       topLevelErrors;
    Syntax       syntax;
    Options      options;
    Compiler    *compiler;
    Context_p    context;
    Renderer     renderer;
    source_files files;
    source_names file_names;
    Deserializer *reader;
    Serializer   *writer;
};

extern Main *MAIN;

XL_END

#endif // MAIN_H
