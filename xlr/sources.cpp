// ****************************************************************************
//  sources.cpp                                                    XLR project
// ****************************************************************************
//
//   File Description:
//
//    Dealing with source files
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
#include <unistd.h>
#include <stdlib.h>
#include <map>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sys/stat.h>
#include "sources.h"
#include "scanner.h"
#include "parser.h"
#include "renderer.h"
#include "errors.h"
#include "tree.h"
#include "context.h"
#include "symbols.h"
#include "compiler.h"
#include "options.h"
#include "basics.h"
#include "serializer.h"
#include "diff.h"
#include "bfs.h"
#include "gv.h"
#include "runtime.h"
#include "traces.h"
#include "flight_recorder.h"
#include "utf8_fileutils.h"


XL_DEFINE_TRACES

XL_BEGIN

SourceFile::SourceFile(text n, Tree *t, Context *c, Symbols *s, bool ro)
// ----------------------------------------------------------------------------
//   Construct a source file given a name
// ----------------------------------------------------------------------------
    : name(n), tree(t), context(c), symbols(s),
      modified(0), changed(false), readOnly(ro),
      info(NULL)
{
#if defined(CONFIG_MINGW)
    struct _stat st;
#else
    struct stat st;
#endif
    if (utf8_stat (n.c_str(), &st) < 0)
        return;
    modified = st.st_mtime;
    if (utf8_access (n.c_str(), W_OK) != 0)
        readOnly = true;
    if (s)
        s->is_global = true;
}


SourceFile::SourceFile()
// ----------------------------------------------------------------------------
//   Default constructor
// ----------------------------------------------------------------------------
    : name(""), tree(NULL), context(NULL), symbols(NULL),
      modified(0), changed(false), readOnly(false), info(NULL)
{}


SourceFile::~SourceFile()
// ----------------------------------------------------------------------------
//   Delete info
// ----------------------------------------------------------------------------
{
    Info *next = NULL;
    for (Info *i = info; i; i = next)
    {
        next = i->next;
        i->Delete();
    }
}


void SourceFile::ListNames(text begin,
                           name_set &names,
                           name_set &infix,
                           name_set &prefix,
                           name_set &postfix)
// ----------------------------------------------------------------------------
//   List names from the symbol table
// ----------------------------------------------------------------------------
{
    if (symbols)
        symbols->ListNames(begin, names, infix, prefix, postfix);
}


Sources::Sources(int inArgc, char **inArgv, text compilerName,
           text syntaxName, text styleSheetName, text builtinsName)
// ----------------------------------------------------------------------------
//   Initialization of the globals
// ----------------------------------------------------------------------------
    : argc(inArgc), argv(inArgv),
      positions(),
      errors(InitErrorsAndMAIN()),
      topLevelErrors(),
      syntax(syntaxName.c_str()),
      options(inArgc, inArgv),
      compiler(new Compiler(compilerName.c_str(), inArgc, inArgv)),
      context(new Context(NULL, NULL)),
      globals(new Symbols(NULL)),
      renderer(std::cout, styleSheetName, syntax),
      reader(NULL), writer(NULL)
{
    XL_INIT_TRACES();
    Options::options = &options;
    Renderer::renderer = &renderer;
    Syntax::syntax = &syntax;
    MAIN = this;
    options.builtins = builtinsName;
    ParseOptions();
    FlightRecorder::SResize(options.flightRecorderSize);
    if (options.flightRecorderFlags)
        FlightRecorder::SFlags(options.flightRecorderFlags);
    globals->is_global = true; // Duh...
}


Sources::~Sources()
// ----------------------------------------------------------------------------
//   Destructor
// ----------------------------------------------------------------------------
{
    delete reader;
    delete writer;
}


Errors *Sources::InitErrorsAndMAIN()
// ----------------------------------------------------------------------------
//   Make sure MAIN is set so that its globals can be accessed
// ----------------------------------------------------------------------------
{
    MAIN = this;
    return NULL;
}


int Sources::ParseOptions()
// ----------------------------------------------------------------------------
//   Load all files given on the command line and compile them
// ----------------------------------------------------------------------------
{
    text cmd, end = "";
    int  filenum  = 0;

    // Make sure debug function is linked in...
    if (getenv("SHOW_INITIAL_DEBUG"))
        debug((Tree *) NULL);

    // Initialize the locale
    if (!setlocale(LC_CTYPE, ""))
        std::cerr << "WARNING: Cannot set locale.\n"
                  << "         Check LANG, LC_CTYPE, LC_ALL.\n";

    // Scan options and build list of files we need to process
    for (cmd = options.ParseFirst(); cmd != end; cmd = options.ParseNext())
    {
        if (options.doDiff)
        {
            options.parseOnly = true;
            if (++filenum > 2)
            {
                std::cerr << "Error: -diff option needs exactly 2 files\n";
                return true;
            }
        }
        file_names.push_back(cmd);
    }
    return false;
}


void Sources::SetupCompiler()
// ----------------------------------------------------------------------------
//    Setup the compiler once all possible options have been set
// ----------------------------------------------------------------------------
{
    compiler->Setup(options);
}


void Sources::CreateScope()
// ----------------------------------------------------------------------------
//   Create a new scope containing a new symbol table and context
// ----------------------------------------------------------------------------
{
    context = new Context(context, NULL);
    globals = new Symbols(globals);
    globals->is_global = true;
}


void Sources::PopScope()
// ----------------------------------------------------------------------------
//   Pop one-level of scope off the scope stack
// ----------------------------------------------------------------------------
{
    context = context->scope;
    globals = globals->parent;
}


int Sources::LoadFiles()
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


SourceFile *Sources::NewFile(text path)
// ----------------------------------------------------------------------------
//   Allocate an entry for updating programs (untitled)
// ----------------------------------------------------------------------------
{
    CreateScope();
    files[path] = SourceFile(path,xl_nil, context, globals, true);
    PopScope();
    return &files[path];
}


int Sources::LoadContextFiles(source_names &ctxFiles)
// ----------------------------------------------------------------------------
//   Load all files given on the command line and compile them
// ----------------------------------------------------------------------------
{
    source_names::iterator file;
    bool hadError = false;

    // Clear all existing symbols (#1777)
    source_files::iterator sfi;
    for (sfi = files.begin(); sfi != files.end(); sfi++)
    {
        SourceFile &sf = (*sfi).second;
        if (sf.context)
            sf.context->Clear();
        if (sf.symbols)
            sf.symbols->Clear();
    }
    files.clear();

    // Load builtins
    if (!options.builtins.empty())
        hadError |= LoadFile(options.builtins, true);

    // Loop over files we will process
    for (file = ctxFiles.begin(); file != ctxFiles.end(); file++)
        hadError |= LoadFile(*file, true);

    return hadError;
}


void Sources::EvaluateContextFiles(source_names &ctxFiles)
// ----------------------------------------------------------------------------
//   Evaluate the context files
// ----------------------------------------------------------------------------
{
    source_names::iterator  file;

    // Execute builtins.xl file first
    if (!options.builtins.empty())
    {
        SourceFile &sf = files[options.builtins];
        if (sf.tree)
        {
            IFTRACE(symbols)
                std::cerr << "Evaluating builtins in context "
                          << sf.context << '\n';
            sf.context->Evaluate(sf.tree);
        }
    }

    // Execute other context files (user.xl, theme.xl)
    for (file = ctxFiles.begin(); file != ctxFiles.end(); file++)
    {
        SourceFile &sf = files[*file];
        if (sf.tree)
            sf.context->Evaluate(sf.tree);
    }
}


void Sources::ListNames(text begin,
                     XL::name_set &names,
                     XL::name_set &infix,
                     XL::name_set &prefix,
                     XL::name_set &postfix)
// ----------------------------------------------------------------------------
//    List the names beginning with the prefix in all symbol tables
// ----------------------------------------------------------------------------
{
    XL::source_files &files = XL::MAIN->files;
    for (XL::source_files::iterator f = files.begin(); f != files.end(); ++f)
        (*f).second.ListNames(begin, names, infix, prefix, postfix);
}    


text Sources::SearchFile(text file)
// ----------------------------------------------------------------------------
//   Default is to use the file name directly
// ----------------------------------------------------------------------------
{
    return file;
}


text Sources::ParentDir(text path)
// ----------------------------------------------------------------------------
//   Return path of parent directory
// ----------------------------------------------------------------------------
{
    text resolved = SearchFile(path);
    if (resolved == "")
        return "";
    const char *p, *str = resolved.c_str();
    for (p = &str[strlen(str) - 1] ; p != str && *p == '/'; p--) {};
    if (p == str)
        return "/";
    for ( ; p != str && *p != '/'; p--) {};
    if (p != str)
        p--;
    return resolved.substr(0, p - str + 1);
}


bool Sources::Refresh(double delay)
// ----------------------------------------------------------------------------
//   Tell that the program won't execute again after the given delay
// ----------------------------------------------------------------------------
{
    (void) delay;
    return false;
}


text Sources::Decrypt(text file)
// ----------------------------------------------------------------------------
//   Decryption hook
// ----------------------------------------------------------------------------
{
    (void) file;
    return "";
}


Tree *Sources::Normalize(Tree *input)
// ----------------------------------------------------------------------------
//   Tree normalization hook
// ----------------------------------------------------------------------------
//   Normalization allows a user application to change the shape of the tree
//   to bring it in some "normal form" before using it.
{
    return input;
}


int Sources::LoadFile(text file,
                   bool updateContext,
                   Context *importContext,
                   Symbols *importSymbols)
// ----------------------------------------------------------------------------
//   Load an individual file
// ----------------------------------------------------------------------------
{
    Tree_p tree = NULL;
    bool hadError = false;

    IFTRACE(fileload)
        std::cout << "Loading: " << file << "\n";

    // Find which source file we are referencing
    SourceFile &sf = files[file];

    // Parse program - Local parser to delete scanner and close file
    // This ensures positions are updated even if there is a 'load'
    // being called during execution.
    if (options.readSerialized)
    {
        if (!reader)
            reader = new Deserializer(std::cin);
        tree = reader->ReadTree();
        if (!reader->IsValid())
        {
            errors->Log(Error("Serialized stream cannot be read: $1")
                        .Arg(file));
            hadError = true;
            return hadError;
        }
    }
    else
    {
        utf8_ifstream ifs(file.c_str(), std::ios::in | std::ios::binary);
        Deserializer ds(ifs);
        tree = ds.ReadTree();
        if (!ds.IsValid())
        {
            std::string decrypted = MAIN->Decrypt(file.c_str());
            if (decrypted != "")
            {
                // Parse decrypted string as XL source
                IFTRACE(fileload)
                    std::cerr << "Info: file was succesfully decrypted\n";
                std::istringstream iss;
                iss.str(decrypted);
                Parser parser (iss, syntax, positions, topLevelErrors);
                tree = parser.Parse();
            }
            else
            {
                // Parse as XL source
                Parser parser (file.c_str(), syntax, positions, topLevelErrors);
                tree = parser.Parse();
            }
        }
        else
        {
            IFTRACE(fileload)
                std::cerr << "Info: file is in serialized format\n";
        }
    }

    if (options.writeSerialized)
    {
        if (!writer)
            writer = new Serializer(std::cout);
        if (tree)
            tree->Do(writer);
    }

    if (tree)
    {
        tree = Normalize(tree);
    }
    else
    {
        if (options.doDiff)
        {
            files[file] = SourceFile (file, NULL, NULL, NULL);
            hadError = false;
        }
    }

    // Create new symbol table for the file, or clear it if we had one
    Context *ctx = MAIN->context;
    Context *savedCtx = ctx;
    Symbols *syms = MAIN->globals;
    Symbols *savedSyms = syms;
    if (sf.context)
    {
        updateContext = false;
        ctx = sf.context;
        syms = sf.symbols;
        ctx->Clear();
        syms->Clear();
    }
    else
    {
        ctx = new Context(ctx, NULL);
        syms = new Symbols(syms);
        syms->name = file;
    }
    MAIN->context = ctx;
    MAIN->globals = syms;
    syms->is_global = true;

    // Connect imports if any
    if (importContext)
        importContext->Import(ctx);
    if (importSymbols)
        importSymbols->Import(syms);

    // HACK - do not hide imported .xl
    if (file.find(".ddd") == file.npos &&
        file.find("tao.xl") == file.npos &&
        file.find("builtins.xl") == file.npos)
    {
        Name_p module_file = new Name("module_file"); // TODO: Position
        Name_p module_dir = new Name("module_dir");
        Text_p module_file_value = new Text(file);
        Text_p module_dir_value = new Text(ParentDir(file));
        ctx->Define(module_file, module_file_value);
        ctx->Define(module_dir, module_dir_value);
        syms->EnterName(module_file->value, module_file_value,Rewrite::LOCAL);
        syms->EnterName(module_dir->value, module_dir_value,Rewrite::LOCAL);
    }

    // Register the source file we had
    sf = SourceFile (file, tree, ctx, syms);
    if (tree)
    {
        // Set symbols and compile if required
        if (!options.parseOnly)
        {
            if (options.optimize_level == 1)
            {
                tree->SetSymbols(syms);
                if (!tree)
                    hadError = true;
                else
                    files[file].tree = tree;
                syms->ProcessDeclarations(tree);
            }

            // TODO: At -O3, do we need to do anything here?
        }
       
        // Graph of the input tree
        if (options.showGV)
        {
            SetNodeIdAction sni;
            BreadthFirstSearch<SetNodeIdAction> bfs(sni);
            tree->Do(bfs);
            GvOutput gvout(std::cout);
            tree->Do(gvout);
        }
    }

    if (options.showSource)
        std::cout << tree << "\n";
    if (options.verbose)
        debugp(tree);

    // Decide if we update symbols for next run
    if (!updateContext)
    {
        MAIN->context = savedCtx;
        MAIN->globals = savedSyms;
    }

    IFTRACE(symbols)
        std::cerr << "Loaded file " << file
                  << " with context " << MAIN->context << '\n';

    return hadError;
}


int Sources::Run(bool print)
// ----------------------------------------------------------------------------
//   Run all files given on the command line
// ----------------------------------------------------------------------------
{
    text cmd, end = "";
    source_names::iterator file;
    bool hadError = false;

    // If we only parse or compile, return
    if (options.parseOnly || options.compileOnly || options.doDiff)
        return -1;

    // Evaluate builtins
    source_names none;
    EvaluateContextFiles(none);

    // Loop over files we will process
    for (file = file_names.begin(); file != file_names.end(); file++)
    {
        SourceFile &sf = files[*file];

        // Evaluate the given tree
        Tree_p result = sf.tree;
        Errors errors;
        if (Tree *tree = result)
        {
            Context *context = sf.context;
            if (options.optimize_level == 0)
            {
                // Slow interpreted evaluation
                result = context->Evaluate(tree);
            }
            else if (options.optimize_level == 1)
            {
                result = sf.symbols->Run(context, tree);
            }
            else if (options.optimize_level == 3)
            {
                if (program_fn code = compiler->CompileProgram(context, tree))
                    result = code();
            }
        }

        if (!result)
        {
            hadError = true;
        }
        else
        {
            if (options.verbose)
                std::cout << "RESULT of " << sf.name << "\n" << result << "\n";
            else if (print)
                std::cout << result << "\n";
        }
    }

    return hadError;
}


int Sources::Diff()
// ----------------------------------------------------------------------------
//   Perform a tree diff between the two loaded files
// ----------------------------------------------------------------------------
{
    source_names::iterator file;

    file = file_names.begin();
    SourceFile &sf1 = files[*file];
    file++;
    SourceFile &sf2 = files[*file];

    Tree_p t1 = sf1.tree;
    Tree_p t2 = sf2.tree;

    TreeDiff d(t1, t2);
    return d.Diff(std::cout);
}

Sources *MAIN = NULL;

XL_END
