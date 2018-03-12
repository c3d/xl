// ****************************************************************************
//  main.cpp                                                      XL project
// ****************************************************************************
//
//   File Description:
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
//  (C) 2010 Jerome Forissier <jerome@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "configuration.h"
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


XL_BEGIN

Main *MAIN = NULL;



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
//    Recorder helpers
//
// ============================================================================




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
    Options::options = &options;
    Renderer::renderer = &renderer;
    Syntax::syntax = &syntax;
    MAIN = this;
    options.builtins = SearchLibFile(builtinsName);
    ParseOptions();
    Opcode::Enter(&context);

    // Once all options have been read, enter symbols and setup compiler
    recorder_configure_type('O', recorder_render<std::ostringstream, Op *>);
    recorder_configure_type('t', recorder_render<std::ostringstream, Tree *>);
#ifndef INTERPRETER_ONLY
    if (options.optimize_level > 1)
    {
        compilerName = SearchFile(compilerName, bin_paths);
        evaluator = new Compiler(compilerName.c_str(), inArgc, inArgv);
    }
    else
#endif // INTERPRETER_ONLY
        evaluator = new Interpreter;
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
    if (!rc && !options.parseOnly)
        rc = Run();
    if (!rc && HadErrors())
        rc = 1;

    if (!rc && options.listen)
    {
        Scope *scope = context.CurrentScope();
        return xl_listen(scope, options.listen_forks, options.listen);
    }

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

    // Make sure debug function is linked in...
    if (getenv("SHOW_INITIAL_DEBUG"))
        debug((Tree *) NULL);

    // Initialize the locale
    if (!setlocale(LC_CTYPE, ""))
        std::cerr << "WARNING: Cannot set locale.\n"
                  << "         Check LANG, LC_CTYPE, LC_ALL.\n";

    // Scan options and build list of files we need to process
    for (cmd = options.ParseFirst(); cmd != end; cmd = options.ParseNext())
        file_names.push_back(cmd);

    // Load builtins before the rest (only after parsing options for builtins)
    if (!options.builtins.empty())
        file_names.insert(file_names.begin(), options.builtins);

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
    for (file = file_names.begin(); file != file_names.end(); file++)
        hadError |= LoadFile(*file);

    return hadError;
}


RECORDER(file_load, 64, "Files being loaded");
int Main::LoadFile(const text &file, text modname)
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
        record(file_load, "Loading from standard input");
        input = &std::cin;
    }
    else
    {
        record(file_load, "Loading from %s", file.c_str());
        input = &inputFile;
    }

    // Check if we need to decrypt an input file first
    if (options.crypted)
    {
        inputStream << input->rdbuf();
        text decrypted = Decrypt(inputStream.str());
        if (decrypted != "")
        {
            record(file_load, "Input was encrypted");
            inputStream.str(decrypted);
        }
        input = &inputStream;
    }

    // Check if we need to deserialize the input file first
    if (options.packed)
    {
        Deserializer deserializer(*input);
        tree = deserializer.ReadTree();
        if (deserializer.IsValid())
        {
            record(file_load, "Input was in serialized format");
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
        record(file_load, "File load error for %s", file.c_str());
        return false;
    }

    // Output packed if this was requested
    if (options.pack)
    {
        std::stringstream output;
        Serializer serialize(output);
        tree->Do(serialize);
        text packed = output.str();
        if (options.crypt)
        {
            text crypted = Encrypt(packed);
            if (crypted == "")
            {
                record(file_load, "No encryption, output is packed");
                std::cout << packed;
            }
            else
            {
                record(file_load, "Encrypted output");
                std::cout << crypted;
            }
        }
        else
        {
            record(file_load, "Packed output");
            std::cout << packed;
        }
    }

    // Normalize if necessary
    tree = Normalize(tree);

    // Show source if requested
    if (options.showSource || options.verbose)
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
    record(file_load, "File loaded as %t", tree);
    record(file_load, "File loaded in %t", context.CurrentScope());

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
    if (options.parseOnly || options.compileOnly)
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
        else if (options.verbose)
        {
            std::cout << "RESULT of " << sf.name << "\n" << result << "\n";
        }
    }

    // Output the result
    if (result && !options.listen)
        std::cerr << result << "\n";

    return hadError;
}



// ============================================================================
//
//   Configurable hooks for use as an application library
//
// ============================================================================

text Main::SearchFile(text file)
// ----------------------------------------------------------------------------
//   Implement the default search strategy for file in paths
// ----------------------------------------------------------------------------
{
    return SearchFile(file, paths);
}


text Main::SearchLibFile(text file)
// ----------------------------------------------------------------------------
//   Implement the default search strategy for file in the library paths
// ----------------------------------------------------------------------------
{
    return SearchFile(file, lib_paths);
}


text Main::SearchFile(text file, const path_list &paths)
// ----------------------------------------------------------------------------
//   Hook to search a file in paths if application sets them up
// ----------------------------------------------------------------------------
{
    utf8_filestat_t st;

    // If the given file is already good, use that
    if (utf8_stat(file.c_str(), &st) == 0)
        return file;

    // Search for a possible file in path
    for (auto &p : paths)
    {
        text candidate = p + file;
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


XL_END


#ifndef LIBXL

RECORDER(main, 32, "Compiler main entry point");
RECORDER_TWEAK_DEFINE(gc_statistics, 0, "Display garbage collector stats");


 int main(int argc, char **argv)
// ----------------------------------------------------------------------------
//   Parse the command line and run the compiler phases
// ----------------------------------------------------------------------------
{
    record(main, "XL Compiler version %s starting", XL_VERSION);

#if HAVE_SBRK
    char *low_water = (char *) sbrk(0);
#endif

    XL::path_list bin { XL_BIN,
                        "/usr/local/bin/", "/bin/", "/usr/bin/" };
    XL::path_list lib { "../lib/xl/", "../lib/",
                        XL_LIB,
                        "/usr/local/lib/xl/", "/lib/xl/", "/usr/lib/xl/"  };
    XL::Main main(argc, argv, bin, lib,
                  "xl", "xl.syntax", "xl.stylesheet", "builtins.xl");
    int rc = main.LoadAndRun();

    if (RECORDER_TRACE(memory) || RECORDER_TRACE(gc_statistics))
        XL::GarbageCollector::GC()->PrintStatistics();

#if HAVE_SBRK
    if (RECORDER_TRACE(memory))
        fprintf(stderr, "Total memory usage: %ldK\n",
                long ((char *) malloc(1) - low_water) / 1024);
#endif

    RECORD(main, "Compiler exit code %d", rc);

    if (main.options.dumpRecorder)
        recorder_dump();

    return rc;
}

#endif // LIBXL
