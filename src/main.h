#ifndef MAIN_H
#define MAIN_H
// ****************************************************************************
//  main.h                          (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                               XL project
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
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
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
#include "options.h"
#include "info.h"

#include <recorder/recorder.h>
#include <map>
#include <set>
#include <time.h>
#include <fstream>


XL_BEGIN

struct Serializer;
struct Deserializer;
struct Compiler;


struct SourceFile
// ----------------------------------------------------------------------------
//    A source file and associated data
// ----------------------------------------------------------------------------
{
    SourceFile(text n, Tree *t, Context *ctx, bool readOnly = false);
    SourceFile();
    ~SourceFile();

    text        name;
    Tree_p      tree;
    Context_p   context;
    time_t      modified;
    text        hash;
    bool        changed;
    bool        readOnly;
};
typedef std::map<text, SourceFile> source_files;
typedef std::vector<text> source_names, path_list;


struct Main
// ----------------------------------------------------------------------------
//    The main entry point and associated data
// ----------------------------------------------------------------------------
{
    Main(int               argc,
         char            **argv,
         const path_list  &bin_paths,
         const path_list  &lib_paths,
         text              compiler_name,
         text              syntax,
         text              style,
         text              builtins);
    virtual ~Main();

    // Entry point that does everythin
    int          LoadAndRun();

    // Individual phases of the above
    Errors *     InitMAIN();
    int          ParseOptions();
    int          LoadFiles();
    int          LoadFile(const text &file, text modname="");
    int          Run();

    // Error checking
    void         Log(Error &e)   { errors->Log(e); }
    uint         HadErrors() { return errors->Count(); }

    // Hooks for use as a library in an application
    virtual text SearchFile(text input);
    virtual text SearchLibFile(text input);
    virtual text SearchFile(text input, const path_list &p);
    virtual text ModuleDirectory(text path);
    virtual text ModuleBaseName(text path);
    virtual text ModuleName(text path);
    virtual bool Refresh(double delay);
    virtual text Decrypt(text input);
    virtual text Encrypt(text input);
    virtual Tree*Normalize(Tree *input);


public:
    int          argc;
    char **      argv;
    path_list    bin_paths, lib_paths, paths;

    Positions    positions;
    Errors *     errors;
    Errors       topLevelErrors;
    Syntax       syntax;
    Options      options;
#ifndef INTERPRETER_ONLY
    Compiler    *compiler;
#endif // INTERPRETER_ONLY
    Context_p    context;
    Renderer     renderer;
    source_files files;
    source_names file_names;
    Deserializer *reader;
    Serializer   *writer;
};

extern Main *MAIN;

RECORDER_DECLARE(file_load);

XL_END

#endif // MAIN_H
