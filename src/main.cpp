// *****************************************************************************
// main.cpp                                                           XL project
// *****************************************************************************
//
// File description:
//
//    Main entry point of the XL runtime and compiler
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2011, Catherine Burvelle <catherine@taodyne.com>
// (C) 2003-2005,2008-2019, Christophe de Dinechin <cdedinechin@dxo.com>
// (C) 2010-2013, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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

#include "tree-clone.h"
#include "main.h"
#include "scanner.h"
#include "parser.h"
#include "renderer.h"
#include "errors.h"
#include "tree.h"
#include "context.h"
#include "types.h"
#include "options.h"
#include "basics.h"
#include "serializer.h"
#include "runtime.h"
#include "utf8_fileutils.h"
#include "opcodes.h"
#include "remote.h"
#include "interpreter.h"
#ifndef INTERPRETER_ONLY
#include "compiler.h"
#include "compiler-fast.h"
#endif // INTERPRETER_ONLY

#include <recorder/recorder.h>
#include <unistd.h>
#include <stdlib.h>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>


RECORDER(fileload,                      16, "Files being loaded");
RECORDER(main,                          16, "Compiler main entry point");
RECORDER(run_results,                   16, "Show run results");
RECORDER(fault_injection,                4, "Fault injection");
RECORDER_TWEAK_DEFINE(gc_statistics,     0, "Display garbage collector stats");
RECORDER_TWEAK_DEFINE(dump_on_exit,  false, "Dump the recorder on exit");
RECORDER_TWEAK_DEFINE(inject_fault,  false, "Test fault handler "
                      "(1 pre-LLVM, 2 post-LLVM 3 stack overflow)");

XL_BEGIN

Main *MAIN = NULL;


// ============================================================================
//
//    Options defined in this file
//
// ============================================================================

namespace Opt
{
TextOption      builtinsPath("builtins_path",
                             "Set the path for the XL builtins file",
                             "builtins.xl");

BooleanOption   builtins("builtins", "Enable builtins file", true);

BooleanOption   compile("compile",
                        "Only compile the file without evaluating it");

IntegerOption   optimize("optimize",
                         "Select optimization level",
                         0, 0, 3);
AliasOption     optimizeAlias("O", optimize);
CodeOption      interpreted("interpreted",
                            "Interpreted mode (same as -O0)",
                            [](Option &opt, Options &opts)
                            {
                                optimize.value = 0;
                            });


BooleanOption   parse("parse",
                      "Only parse the file without evaluating it");

BooleanOption   remote("remote",
                       "Listen for remote programs");

IntegerOption   remotePort("remote_port",
                           "Select the port to listen to for remote access",
                           1205, 1000, 32767);

IntegerOption   remoteForks("remote_forks",
                            "Select the number of forks for remote access",
                            20, 0, 1000);

BooleanOption   showSource("show",
                           "Show the source code");

TextOption      stylesheet("stylesheet",
                           "Select the style sheet for rendering XL code",
                           "xl.stylesheet");

CodeOption      trace("trace",
                      "Activate recorder traces",
                      [](Option &opt, Options &opts)
                      {
                          kstring arg = opts.Argument();
                          recorder_trace_set(arg);
                      });
AliasOption     traceAlias("t", trace);

BooleanOption   writeEncrypted("encrypted_writes",
                             "Encrypt files as they are written");

BooleanOption   writePacked("packed_writes",
                     "Pack files as they are written");

BooleanOption   emitIR("emit_ir", "Generate LLVM IR suitable for llvmc");
AliasOption     emitIRAlias("B", emitIR);
}



// ============================================================================
//
//    Source file data
//
// ============================================================================

SourceFile::SourceFile(text n, Tree *t, Scope *scope, bool ro)
// ----------------------------------------------------------------------------
//   Construct a source file given a name
// ----------------------------------------------------------------------------
    : name(n), tree(t), scope(scope),
      modified(0), changed(false), readOnly(ro)
{
    utf8_filestat_t st;
    if (utf8_stat (n.c_str(), &st) < 0)
        return;
    modified = st.st_mtime;
    if (utf8_access (n.c_str(), W_OK) != 0)
        readOnly = true;
}


SourceFile::SourceFile()
// ----------------------------------------------------------------------------
//   Default constructor
// ----------------------------------------------------------------------------
    : name(""), tree(NULL), scope(NULL),
      modified(0), changed(false), readOnly(false)
{}


SourceFile::~SourceFile()
// ----------------------------------------------------------------------------
//   Delete info
// ----------------------------------------------------------------------------
{}



// ============================================================================
//
//     Main
//
// ============================================================================

Main::Main(int inArgc,
           char **inArgv,
           const path_list &bin_paths,
           const path_list &lib_paths,
           text compilerName,
           text syntaxName,
           text styleSheetName,
           text builtinsName)
// ----------------------------------------------------------------------------
//   Initialization of the globals
// ----------------------------------------------------------------------------
    : argc(inArgc),
      argv(inArgv),
      bin_paths(bin_paths),
      lib_paths(lib_paths),
      positions(),
      errors(InitMAIN()),
      topLevelErrors(),
      syntax(SearchLibFile(syntaxName).c_str()),
      options(inArgc, inArgv),
      context(),
      renderer(std::cout, SearchLibFile(styleSheetName), syntax),
      reader(NULL),
      writer(NULL),
      evaluator(NULL)
{
    recorder_dump_on_common_signals(0, 0);
    recorder_trace_set(".*_(error|warning)");
    recorder_trace_set(getenv("XL_TRACES"));
    recorder_configure_type('O', recorder_render<std::ostringstream, Op *>);
    recorder_configure_type('t', recorder_render<std::ostringstream, Tree *>);
    Renderer::renderer = &renderer;
    Syntax::syntax = &syntax;
    MAIN = this;
    Opt::builtinsPath.value = SearchLibFile(builtinsName);
    ParseOptions();
    Opcode::Enter(&context);

    // Once all options have been read, enter symbols and setup compiler
#ifndef INTERPRETER_ONLY
    compilerName = SearchFile(compilerName, bin_paths);
    kstring cname = compilerName.c_str();
    uint opt = Opt::optimize.value;
    if (opt == 1)
        evaluator = new FastCompiler(cname, opt, inArgc, inArgv);
    else if (opt >= 2)
        evaluator = new Compiler(cname, opt, inArgc, inArgv);
    else
#endif // INTERPRETER_ONLY
        evaluator = new Interpreter;

    // Force a crash if this is requested
    XL_ASSERT(RECORDER_TWEAK(inject_fault) != 2 && "Running late crash test");
}


Main::~Main()
// ----------------------------------------------------------------------------
//   Destructor
// ----------------------------------------------------------------------------
{
    if (topLevelErrors.HadErrors())
    {
        topLevelErrors.Display();
        topLevelErrors.Clear();
    }

    delete reader;
    delete writer;
}


Tree *Main::Evaluate(Scope *scope, Tree *source)
// ----------------------------------------------------------------------------
//   Dispatch evaluation to the appropriate engine for the given opt level
// ----------------------------------------------------------------------------
{
    return evaluator->Evaluate(scope, source);
}


bool Main::TypeAnalysis(Scope *scope, Tree *source)
// ----------------------------------------------------------------------------
//   Dispatch type analysis to the appropriate engine for the given opt level
// ----------------------------------------------------------------------------
{
    return evaluator->TypeAnalysis(scope, source);
}


Tree *Main::TypeCheck(Scope *scope, Tree *type, Tree *value)
// ----------------------------------------------------------------------------
//   Dispatch evaluation to the appropriate engine for the given opt level
// ----------------------------------------------------------------------------
{
    return evaluator->TypeCheck(scope, type, value);
}


int Main::LoadAndRun()
// ----------------------------------------------------------------------------
//   An single entry point for the normal phases
// ----------------------------------------------------------------------------
{
    int rc = LoadFiles();
    if (!rc && !Opt::parse)
        rc = Run();
    if (!rc && HadErrors())
        rc = 1;

    if (!rc && Opt::remote)
    {
        Scope *scope = context.CurrentScope();
        return xl_listen(scope, Opt::remoteForks, Opt::remotePort);
    }

    record(main, "LoadAndRun returns %d", rc);
    return rc;
}


Errors *Main::InitMAIN()
// ----------------------------------------------------------------------------
//   Make sure MAIN is set so that its globals can be accessed
// ----------------------------------------------------------------------------
{
    MAIN = this;
    return NULL;
}


int Main::ParseOptions()
// ----------------------------------------------------------------------------
//   Load all files given on the command line and compile them
// ----------------------------------------------------------------------------
{
    text cmd, end = "";

    // Initialize the locale
    if (!setlocale(LC_CTYPE, ""))
        std::cerr << "WARNING: Cannot set locale.\n"
                  << "         Check LANG, LC_CTYPE, LC_ALL.\n";

    // Test that the crash handler works for stack overflows
    if (RECORDER_TWEAK(inject_fault) == 3)
    {
        record(fault_injection, "Provoking stack overflow %p", (void *) &cmd);
        ParseOptions();
    }

    // Scan options and build list of files we need to process
    for (cmd = options.ParseFirst(); cmd != end; cmd = options.ParseNext())
        file_names.push_back(cmd);

    // Test that the crash handler works for stack overflows
    if (RECORDER_TWEAK(inject_fault) == 3)
    {
        record(fault_injection, "Provoking stack overflow %p", (void *) &cmd);
        ParseOptions();
    }

    // Load builtins before the rest (only after parsing options for builtins)
    if (Opt::builtins)
        file_names.insert(file_names.begin(), Opt::builtinsPath);

    // Check if there were errors parsing it
    if (topLevelErrors.HadErrors())
    {
        fprintf(stderr, "\n");
        topLevelErrors.Display();
        exit(1);
    }

    // Update style sheet
    renderer.SelectStyleSheet(SearchLibFile(Opt::stylesheet, ".stylesheet"));

    // Force a crash if this is requested
    XL_ASSERT(RECORDER_TWEAK(inject_fault) != 1 && "Running early crash test");

    return false;
}


int Main::LoadFiles()
// ----------------------------------------------------------------------------
//   Load all files given on the command line and compile them
// ----------------------------------------------------------------------------
{
    source_names::iterator  file;
    bool hadError = false;

    // Loop over files we will process
    for (auto &file : file_names)
    {
        int rc = LoadFile(file);
        hadError |= rc;
        record(fileload,
               "Load file %s code %d, errors %d", file.c_str(), rc, hadError);
    }

    return hadError;
}


int Main::LoadFile(text file, text modname)
// ----------------------------------------------------------------------------
//   Load an individual file
// ----------------------------------------------------------------------------
{
    // Find which source file we are referencing
    SourceFile         &sf       = files[file];
    std::istream       *input    = NULL;
    Tree_p              tree     = NULL;
    utf8_ifstream       inputFile(file.c_str(), std::ios::in|std::ios::binary);
    std::stringstream   inputStream;


    // See if we read from standard input
    if (file == "-")
    {
        record(fileload, "Loading from standard input");
        input = &std::cin;
    }
    else
    {
        record(fileload, "Loading from %s", file.c_str());
        input = &inputFile;
    }

    // Check if we need to decrypt an input file first
    if (Opt::writeEncrypted)
    {
        inputStream << input->rdbuf();
        text decrypted = Decrypt(inputStream.str());
        if (decrypted != "")
        {
            record(fileload, "Input was encrypted");
            inputStream.str(decrypted);
        }
        input = &inputStream;
    }

    // Check if we need to deserialize the input file first
    if (Opt::writePacked)
    {
        Deserializer deserializer(*input);
        tree = deserializer.ReadTree();
        if (deserializer.IsValid())
        {
            record(fileload, "Input was in serialized format");
        }
    }

    // Read in standard format if we could not read it from packed format
    if (!tree)
    {
        // Set the name used for error messages
        kstring errName = file.c_str();
        if (file == "-")
            errName = "<stdin>";
        Parser parser (*input, syntax, positions, topLevelErrors, errName);
        tree = parser.Parse();
    }

    // If at this stage we don't have a tree, this is an error
    if (!tree)
    {
        record(fileload, "File load error for %s", file.c_str());
        return false;
    }

    // Output packed if this was requested
    if (Opt::writePacked)
    {
        std::stringstream output;
        Serializer serialize(output);
        tree->Do(serialize);
        text packed = output.str();
        if (Opt::writeEncrypted)
        {
            text crypted = Encrypt(packed);
            if (crypted == "")
            {
                record(fileload, "No encryption, output is packed");
                std::cout << packed;
            }
            else
            {
                record(fileload, "Encrypted output");
                std::cout << crypted;
            }
        }
        else
        {
            record(fileload, "Packed output");
            std::cout << packed;
        }
    }

    // Normalize if necessary
    tree = Normalize(tree);

    // Show source if requested
    if (Opt::showSource)
        std::cout << tree << "\n";

    // Create new symbol table for the file
    context.CreateScope(tree->Position());

    // Set the module path, directory and file
    context.SetModulePath(file);
    context.SetModuleDirectory(ModuleDirectory(file));
    context.SetModuleFile(ModuleBaseName(file));

    // Check if the module name is given
    if (modname != "")
    {
        // If we have an explicit module name (e.g. import),
        // we will refer to the content using that name
        context.SetModuleName(modname);
        context.Define(modname, tree);
    }
    else
    {
        // No explicit module name: update current context
        modname = ModuleName(file);
        context.SetModuleName(modname);
    }

    // Register the source file we had
    sf = SourceFile (file, tree, context.CurrentScope());

    // Process declarations from the program
    record(fileload, "File loaded as %t", tree);
    record(fileload, "File loaded in %t", context.CurrentScope());

    // We were OK, done
    return false;
}


int Main::Run()
// ----------------------------------------------------------------------------
//   Run all files given on the command line
// ----------------------------------------------------------------------------
{
    text cmd, end = "";
    source_names::iterator file;
    bool hadError = false;

    // If we only parse or compile, return
    if (Opt::parse || Opt::compile)
        return -1;

    // Loop over files we will process
    Tree_p result = xl_nil;
    for (file = file_names.begin(); file != file_names.end(); file++)
    {
        SourceFile &sf = files[*file];

        // Evaluate the given tree
        Errors errors;
        if (Tree *tree = sf.tree)
        {
            result = Evaluate(sf.scope, tree);
            if (errors.HadErrors())
            {
                errors.Display();
                errors.Clear();
            }
        }

        if (!result)
        {
            hadError = true;
        }
        record(run_results, "Result of %+s is %t", sf.name, result);
    }

    // Output the result
    if (result && !Opt::remote && !Opt::emitIR)
        std::cout << result << "\n";

    return hadError;
}



// ============================================================================
//
//   Configurable hooks for use as an application library
//
// ============================================================================

text Main::SearchFile(text file, text extension)
// ----------------------------------------------------------------------------
//   Implement the default search strategy for file in paths
// ----------------------------------------------------------------------------
{
    return SearchFile(file, paths, extension);
}


text Main::SearchLibFile(text file, text extension)
// ----------------------------------------------------------------------------
//   Implement the default search strategy for file in the library paths
// ----------------------------------------------------------------------------
{
    return SearchFile(file, lib_paths, extension);
}


text Main::SearchFile(text file, const path_list &paths, text extension)
// ----------------------------------------------------------------------------
//   Hook to search a file in paths if application sets them up
// ----------------------------------------------------------------------------
{
    utf8_filestat_t st;

    record(fileload, "Search file '%s' extension '%s', %u paths",
           file, extension, paths.size());

    // Add extension if needed
    size_t len = extension.length();
    if (len)
    {
        size_t flen = file.length();
        if (file.rfind(extension) != flen - len)
            file += extension;
    }

    // If the given file is already good, use that
    if (file.find("/") == 0 || utf8_stat(file.c_str(), &st) == 0)
    {
        record(fileload, "Quick exit for '%s'", file);
        return file;
    }

    // Search for a possible file in path
    for (auto &p : paths)
    {
        text candidate = p + file;
        record(fileload, "Checking candidate '%s' in path '%s'", candidate, p);
        if (utf8_stat (candidate.c_str(), &st) == 0)
            return candidate;
    }

    // If we get there, we'll probably have an error downstream
    return file;
}


static inline kstring endOfPath(kstring path)
// ----------------------------------------------------------------------------
//   Find the end of the path
// ----------------------------------------------------------------------------
{
    const char *p = path;
    while (*p) p++;
    p--;
    while (p != path && isDirectorySeparator(*p))
        p--;
    while (p != path && !isDirectorySeparator(*p))
        p--;
    if (isDirectorySeparator(*p))
        p++;
    return p;
}


text Main::ModuleDirectory(text path)
// ----------------------------------------------------------------------------
//   Return parent directory for a given file name
// ----------------------------------------------------------------------------
{
    kstring str = path.c_str();
    kstring dirSep = endOfPath(str);
    text    result = path.substr(0, dirSep - str);
    if (result == "")
        result = "./";
    return result;
}


text Main::ModuleBaseName(text path)
// ----------------------------------------------------------------------------
//   Return the base name for the path
// ----------------------------------------------------------------------------
{
    kstring str = path.c_str();
    kstring dirSep = endOfPath(str);
    return text(dirSep);
}


text Main::ModuleName(text path)
// ----------------------------------------------------------------------------
//   Return the module name, e.g. turn foo/bar-bi-tu.xl into bar_bi_tu
// ----------------------------------------------------------------------------
{
    text    result = "";
    bool    hadUnderscore = false;
    kstring str = path.c_str();
    kstring p = endOfPath(str);
    while (char c = *p++)
    {
        if (c == '.')
            break;

        bool punct = ispunct(c);
        if (!punct)
            result += c;
        else if (!hadUnderscore)
            result += '_';

        hadUnderscore = punct;
    }

    return result;
}


bool Main::Refresh(double delay)
// ----------------------------------------------------------------------------
//   Tell that the program won't execute again after the given delay
// ----------------------------------------------------------------------------
{
    (void) delay;
    return false;
}


text Main::Decrypt(text file)
// ----------------------------------------------------------------------------
//   Decryption hook - Return decrypted file if possible
// ----------------------------------------------------------------------------
{
    return "";
}


text Main::Encrypt(text file)
// ----------------------------------------------------------------------------
//   Encryption hook - Encrypt file if possible
// ----------------------------------------------------------------------------
{
    return "";
}


Tree *Main::Normalize(Tree *input)
// ----------------------------------------------------------------------------
//   Tree normalization hook
// ----------------------------------------------------------------------------
//   Normalization allows a user application to change the shape of the tree
//   to bring it in some "normal form" before using it.
{
    return input;
}


eval_fn Main::Declarator(text name)
// ----------------------------------------------------------------------------
//   Return a declarator for 'import' and 'load'
// ----------------------------------------------------------------------------
{
    if (name == "import")
        return xl_import;
    if (name == "load")
        return xl_load;
    return nullptr;
}

XL_END
