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
#include <map>
#include <set>
#include <time.h>


XL_BEGIN

struct Serializer;
struct Deserializer;
struct Compiler;


struct SourceFile
// ----------------------------------------------------------------------------
//    A source file and associated data
// ----------------------------------------------------------------------------
{
    SourceFile(text n, Tree *t, Context *c, Symbols *s, bool readOnly = false);
    SourceFile();
    ~SourceFile();

    // Info management
    template<class I>
    I*                  GetInfo() const;
    template<class I>
    void                SetInfo(I *);
    template <class I>
    bool                Purge();
    template <class I>
    I*                  Remove(I *);

    void                ListNames(text begin,
                                  name_set &names,
                                  name_set &infix,
                                  name_set &prefix,
                                  name_set &postfix);

public:
    text        name;
    Tree_p      tree;
    Context_p   context;
    Symbols_p   symbols;
    time_t      modified;
    text        hash;
    bool        changed;
    bool        readOnly;
    Info *      info;
};
typedef std::map<text, SourceFile> source_files;
typedef std::vector<text> source_names;


struct Main
// ----------------------------------------------------------------------------
//    The main entry point and associated data
// ----------------------------------------------------------------------------
{
    Main(int argc, char **argv,
         text compiler_name = "xl",
         text syntax = "xl.syntax",
         text style = "xl.stylesheet",
         text builtins = "builtins.xl");
    virtual ~Main();

    int          ParseOptions();
    void         SetupCompiler();
    void         CreateScope();
    void         PopScope();
    int          LoadContextFiles(source_names &context_file_names);
    void         EvaluateContextFiles(source_names &context_file_names);
    int          LoadFiles();
    virtual int  LoadFile(text file, bool updateContext = false,
                          Context *importContext=0, Symbols *importSymbols=0);
    SourceFile * NewFile(text path);
    virtual text SearchFile(text input);
    virtual text ParentDir(text input);
    virtual bool Refresh(double delay);
    virtual text Decrypt(text input);
    virtual Tree*Normalize(Tree *input);
    int          Run();
    int          Diff();
    void         Log(Error &e)   { errors->Log(e); }
    Errors *     InitErrorsAndMAIN();
    uint         HadErrors() { return errors->Count(); }
    void         ListNames(text begin,
                           name_set &names,
                           name_set &infix,
                           name_set &prefix,
                           name_set &postfix);

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
    Symbols_p    globals;
    Renderer     renderer;
    source_files files;
    source_names file_names;
    Deserializer *reader;
    Serializer   *writer;
};

extern Main *MAIN;



// ============================================================================
//
//    Template members for info management
//
// ============================================================================

template <class I> inline I* SourceFile::GetInfo() const
// ----------------------------------------------------------------------------
//   Find if we have an information of the right type in 'info'
// ----------------------------------------------------------------------------
{
    for (Info *i = info; i; i = i->next)
        if (I *ic = dynamic_cast<I *> (i))
            return ic;
    return NULL;
}


template <class I> inline void SourceFile::SetInfo(I *i)
// ----------------------------------------------------------------------------
//   Set the information given as an argument
// ----------------------------------------------------------------------------
{
    Info *last = i;
    while(last->next)
        last = last->next;
    last->next = info;
    info = i;
}


template <class I> inline bool SourceFile::Purge()
// ----------------------------------------------------------------------------
//   Find and purge information of the given type
// ----------------------------------------------------------------------------
{
    Info *last = NULL;
    Info *next = NULL;
    bool purged = false;
    for (Info *i = info; i; i = next)
    {
        next = i->next;
        if (I *ic = dynamic_cast<I *> (i))
        {
            if (last)
                last->next = next;
            else
                info = next;
            ic->Delete();
            purged = true;
        }
        else
        {
            last = i;
        }
    }
    return purged;
}


template <class I> inline I* SourceFile::Remove(I *toFind)
// ----------------------------------------------------------------------------
//   Find information matching input and remove it if it exists
// ----------------------------------------------------------------------------
{
    Info *prev = NULL;
    for (Info *i = info; i; i = i->next)
    {
        I *ic = dynamic_cast<I *> (i);
        if (ic == toFind)
        {
            if (prev)
                prev->next = i->next;
            else
                info = i->next;
            i->next = NULL;
            return ic;
        }
        prev = i;
    }
    return NULL;
}


XL_END

#endif // MAIN_H
