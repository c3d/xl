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
// This software is licensed under the GNU General Public License v3+
// (C) 2011, Catherine Burvelle <catherine@taodyne.com>
// (C) 2003-2005,2008-2020, Christophe de Dinechin <cdedinechin@dxo.com>
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

#include "tree-clone.h"
#include "main.h"
#include "scanner.h"
#include "parser.h"
#include "renderer.h"
#include "errors.h"
#include "tree.h"
#include "context.h"
#include "options.h"
#include "serializer.h"
#include "runtime.h"
#include "utf8_fileutils.h"
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

Main *MAIN = nullptr;


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
TextListOption  preloads("preload",
                         "Files to preload as context for the program");

BooleanOption   builtins("builtins", "Enable builtins file", true);

BooleanOption   compile("compile",
                        "Only compile the file without evaluating it");

NaturalOption   optimize("optimize",
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

TextListOption  paths("paths",
                      "Paths to search for source code files");
AliasOption     pathsAlias("I", paths);

TextListOption  binPaths("binaries",
                        "Paths to search for binaries");
TextListOption  libPaths("libraries",
                        "Paths to search for libraries");
AliasOption     libPathsAlias("L", libPaths);

BooleanOption   remote("remote",
                       "Listen for remote programs");

NaturalOption   remotePort("remote_port",
                           "Select the port to listen to for remote access",
                           1205, 1000, 32767);

NaturalOption   remoteForks("remote_forks",
                            "Select the number of forks for remote access",
                            20, 0, 1000);

BooleanOption   showSource("show",
                           "Show the source code");

TextOption      stylesheet("stylesheet",
                           "Select the style sheet for rendering XL code",
                           "xl.stylesheet");

TextOption      syntax("syntax",
                       "Select the syntax file for parsing XL code",
                       "xl.syntax");

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
//     Main
//
// ============================================================================

Main::Main(int inArgc,
           char **inArgv,
           const path_list &paths,
           const path_list &bin_paths,
           const path_list &lib_paths,
           text compilerName)
// ----------------------------------------------------------------------------
//   Initialization of the globals
// ----------------------------------------------------------------------------
    : argc(inArgc),
      argv(inArgv),
      positions(),
      errors(InitMAIN()),
      topLevelErrors(),
      syntax(),
      options(inArgc, inArgv),
      context(),
      renderer(std::cout, syntax),
      reader(nullptr),
      writer(nullptr),
      evaluator(nullptr)
{
    recorder_dump_on_common_signals(0, 0);
    recorder_trace_set(".*_(error|warning)");
    recorder_trace_set(getenv("XL_TRACES"));
    recorder_configure_type('t', recorder_render<std::ostringstream, Tree *>);
    Renderer::renderer = &renderer;
    Syntax::syntax = &syntax;
    MAIN = this;
    ParseOptions();
#ifndef INTERPRETER_ONLY
    if (Opt::emitIR && Opt::optimize.value < 2)
    {
        JIT::Comment("WARNING: Using -emit_ir or -B option without LLVM.");
        JIT::Comment("         Enabled -O3 to get an LLVM output.");
        Opt::optimize.value = 3;
    }
#endif // INTERPRETER_ONLY

    // When given paths, add the standard ones at end
    path_list &plist = Opt::paths;
    path_list &blist = Opt::binPaths;
    path_list &llist = Opt::libPaths;
    plist.insert(plist.end(), paths.begin(), paths.end());
    blist.insert(blist.end(),bin_paths.begin(),bin_paths.end());
    llist.insert(llist.end(),lib_paths.begin(),lib_paths.end());

    // Adjust syntax file and renderer based on options
    text syntaxPath = SearchLibFile(Opt::syntax, "syntax");
    if (syntaxPath.length())
        syntax.ReadSyntaxFile(syntaxPath);
    else
        Ooops("Syntax file $1 not found").Arg(Opt::syntax);
    text stylePath = SearchLibFile(Opt::stylesheet, "stylesheet");
    if (stylePath.length())
        renderer.SelectStyleSheet(stylePath);
    else
        Ooops("Stylesheet file $1 not found").Arg(Opt::stylesheet);

    // Once all options have been read, enter symbols and setup compiler
#ifndef INTERPRETER_ONLY
    compilerName = SearchFile(compilerName, Opt::binPaths);
    kstring cname = compilerName.c_str();
    uint opt = Opt::optimize.value;
    if (opt == 1)
        evaluator = new FastCompiler(cname, opt, inArgc, inArgv);
    else if (opt >= 2)
        evaluator = new Compiler(cname, opt, inArgc, inArgv);
    else
#endif // INTERPRETER_ONLY
        evaluator = new Interpreter(context);

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
    delete evaluator;
}


Tree *Main::Evaluate(Scope *scope, Tree *source)
// ----------------------------------------------------------------------------
//   Dispatch evaluation to the appropriate engine for the given opt level
// ----------------------------------------------------------------------------
{
    return evaluator->Evaluate(scope, source);
}


Tree *Main::TypeCheck(Scope *scope, Tree *type, Tree *value)
// ----------------------------------------------------------------------------
//   Dispatch evaluation to the appropriate engine for the given opt level
// ----------------------------------------------------------------------------
{
    return evaluator->TypeCheck(scope, type, value);
}


Errors *Main::InitMAIN()
// ----------------------------------------------------------------------------
//   Make sure MAIN is set so that its globals can be accessed
// ----------------------------------------------------------------------------
{
    MAIN = this;
    Interpreter::InitializeBuiltins();
    return nullptr;
}


void Main::ParseOptions()
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

    // Check if there were errors parsing it
    if (topLevelErrors.HadErrors())
    {
        fprintf(stderr, "\n");
        topLevelErrors.Display();
        exit(1);
    }

    // Force a crash if this is requested
    XL_ASSERT(RECORDER_TWEAK(inject_fault) != 1 && "Running early crash test");
}


Tree_p Main::LoadFiles()
// ----------------------------------------------------------------------------
//   Process all the files given on the command line, return true on success
// ----------------------------------------------------------------------------
{
    // Loop over files we will process
    bool evaluate = !Opt::parse && !Opt::compile;
    Tree_p result = xl_nil;

    // Load builtins and preloads first
    path_list preloads = Opt::preloads;
    if (Opt::builtins)
        preloads.insert(preloads.begin(), Opt::builtinsPath);
    for (auto p : preloads)
    {
        text path = SearchLibFile(p);
        if (path == "")
        {
            Ooops("File $1 not found").Arg(p);
            return result;
        }
        Tree_p result = LoadFile(path, evaluate);
        if (HadErrors())
            return result;
        if (Scope *scope = result->As<Scope>())
            context.SetSymbols(scope);
    }

    // Then process files
    for (auto &file : file_names)
    {
        result = LoadFile(file, evaluate);
        if (HadErrors())
            return result;
    }
    return result;
}


Tree_p Main::LoadFile(text file, bool evaluate)
// ----------------------------------------------------------------------------
//   Load an individual file
// ----------------------------------------------------------------------------
{
    record(fileload, "Loading file %s", file);
    Module *module = Module::Get(context.Symbols(), file, evaluate);
    Tree_p result = module->Value();
    record(fileload, "Loaded file %s into module %p value %t",
           file, module, result);
    return result;
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
    return SearchFile(file, Opt::paths, extension);
}


text Main::SearchLibFile(text file, text extension)
// ----------------------------------------------------------------------------
//   Implement the default search strategy for file in the library paths
// ----------------------------------------------------------------------------
{
    return SearchFile(file, Opt::libPaths, extension);
}


text Main::SearchFile(text file, path_list &paths, text extension)
// ----------------------------------------------------------------------------
//   Hook to search a file in paths if application sets them up
// ----------------------------------------------------------------------------
{
    record(fileload, "Search file '%s' extension '%s', %u paths",
           file, extension, paths.size());

    // Quick exit for empty file
    size_t flen = file.length();
    if (!flen)
        return "";

    // Add extension if needed
    size_t len = extension.length();
    if (len && extension[0] != '.')
    {
        extension = "." + extension;
        len++;
    }
    if (flen < len || file.compare(flen - len, len, extension))
        file += extension;

    // If the given file is already good, use that
    utf8_filestat_t st;
    if (utf8_stat(file.c_str(), &st) == 0)
    {
        record(fileload, "Quick exit for '%s'", file);
        return file;
    }
    if (isDirectorySeparator(file[0]))
    {
        record(fileload, "Absolute path '%s' not found", file);
        return "";
    }

    // Search for a possible file in path
    for (auto &&p : paths)
    {
        if (p != "" && !isDirectorySeparator(p[p.length() - 1]))
            p += "/";
        text candidate = p + file;
        record(fileload, "Checking candidate '%s' in path '%s'", candidate, p);
        if (utf8_stat (candidate.c_str(), &st) == 0)
        {
            record(fileload, "Found candidate '%s'", candidate);
            return candidate;
        }
    }

    // We failed to find a candidate
    return "";
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
//   Return a declarator for 'use' and 'load'
// ----------------------------------------------------------------------------
{
    if (name == "use" || name == "import")
        return xl_import;
    if (name == XL::Normalize("parse_file"))
        return xl_parse_file;
    return nullptr;
}

XL_END
