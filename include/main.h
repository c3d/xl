#ifndef MAIN_H
#define MAIN_H
// *****************************************************************************
// main.h                                                             XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2009-2012,2014-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010-2013, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

#include "tree.h"
#include "scanner.h"
#include "parser.h"
#include "renderer.h"
#include "errors.h"
#include "module.h"
#include "syntax.h"
#include "context.h"
#include "options.h"
#include "info.h"
#include "evaluator.h"
#include "ownership.h"

#include <recorder/recorder.h>
#include <map>
#include <set>
#include <time.h>
#include <fstream>


XL_BEGIN

struct Serializer;
struct Deserializer;
struct Compiler;

typedef std::vector<text>       path_list;


struct Main
// ----------------------------------------------------------------------------
//    The main entry point and associated data
// ----------------------------------------------------------------------------
{
    Main(int               argc,
         char            **argv,
         const path_list  &paths,
         const path_list  &bin_paths,
         const path_list  &lib_paths,
         text              compiler_name);
    virtual ~Main();

    // Evaluate a tree in the given context
    Tree *              Evaluate(Scope *scope, Tree *value);
    Tree *              TypeCheck(Scope *scope, Tree *type, Tree *value);

    // Internal procesing
    Errors *            InitMAIN();
    void                ParseOptions();
    Tree_p              LoadFiles();
    eval_fn             Importer(text name);

    // Hooks for use as a library in an application
    virtual Tree_p      LoadFile(text file, bool evaluate);
    virtual void        Log(Error &e)   { errors->Log(e); }
    virtual uint        HadErrors()     { return errors->Count(); }
    virtual text        SearchFile(text input, text ext = "");
    virtual text        SearchLibFile(text input, text ext = "");
    virtual text        SearchFile(text input, path_list &p, text ext="");
    virtual bool        Refresh(double delay);
    virtual text        Decrypt(text input);
    virtual text        Encrypt(text input);
    virtual Tree*       Normalize(Tree *input);
    virtual Tree *      Show(std::ostream &, Tree *);
    virtual void        AddImporters();

public:
    int                 argc;
    char **             argv;

    Positions           positions;
    Errors *            errors;
    Errors              topLevelErrors;
    Syntax              syntax;
    Options             options;
    Context             context;
    Renderer            renderer;
    path_list           file_names;
    Deserializer *      reader;
    Serializer   *      writer;
    Evaluator *         evaluator;
};

extern Main *MAIN;

namespace Opt
{
extern NaturalOption    optimize;
extern NaturalOption    remoteForks;
extern TextOption       stylesheet;
extern BooleanOption    emitIR;
extern BooleanOption    caseSensitive;
extern BooleanOption    showSource;

}

XL_END

RECORDER_DECLARE(fileload);
RECORDER_DECLARE(main);
RECORDER_TWEAK_DECLARE(gc_statistics);
RECORDER_TWEAK_DECLARE(dump_on_exit);

#endif // MAIN_H
