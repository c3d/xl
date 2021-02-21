#ifndef MODULE_H
#define MODULE_H
// ****************************************************************************
//  module.h                                                        XL project
// ****************************************************************************
//
//   File Description:
//
//     Class representing XL modules when loaded by the translator
//
//
//
//
//
//
//
//
// ****************************************************************************
//   (C) 2021 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
// ****************************************************************************
//   This file is part of XL.
//
//   XL is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   XL is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with XL.  If not, see <https://www.gnu.org/licenses/>.
// ****************************************************************************

#include "tree.h"

#include "context.h"
#include "syntax.h"
#include "renderer.h"


XL_BEGIN

struct Module
// ----------------------------------------------------------------------------
//    Representation of an XL module
// ----------------------------------------------------------------------------
//    An XL module contains at most one of each:
//    - A specification, from a file with an '.xs' extension
//    - An implementation, from a file with an '.xl' extension
//    - A syntax description, from a file with a '.syntax' extension
//    - A stylesheet for rendering, from a file with a '.stylesheet' extension
//    A module can
{
    static Module *Get(Tree *name);
    static Module *Get(text name);

    enum Part { SPECIFICATION = 1, IMPLEMENTATION = 2, BOTH = 3 };

    bool        IsValid(Part part = BOTH);

    Tree *      Source(Part part = BOTH);
    Tree *      Value(Part part = BOTH);
    Scope *     FileScope(Part part = BOTH);
    Scope *     FileScope(Scope *where);
    text        Syntax();               // Return custom syntax file if any
    text        StyleSheet();           // Return specific style sheet or ""
    bool        HasChanged();           // Check if file changed on disk
    Tree_p      Reload();               // Reload sources if changed

public:
    struct Info : XL::Info
    // ------------------------------------------------------------------------
    //   Information about a file that was imported (attached to import)
    // ------------------------------------------------------------------------
    {
        Info(Tree *import) : import(import), value(), alias(), module() {}
        Tree_p      import;
        Tree_p      value;
        Tree_p      alias;
        Module *    module;
    };

private:
    Module(Tree *name, text key);
    ~Module();
    static text    Name(Tree *name); // Get the file name for A.B.C

private:
    struct SourceFile
    // ------------------------------------------------------------------------
    //   Representation of an XL source file
    // ------------------------------------------------------------------------
    {
        SourceFile(text path);
        ~SourceFile();

        Tree*   Source();               // Return source code
        Scope*  FileScope();            // Return scope for that file
        Tree *  Value();                // What the file evaluated to
        bool    HasChanged();           // Check if source file was modified
        Tree *  Reload();               // Reload even if it has changed

    private:
        text    path;                   // Path to the source file
        Tree_p  source;                 // Source file when loaded
        Scope_p scope;                  // Scope created for that file
        Tree_p  value;                  // What this evaluated to
        time_t  modified;               // Last known modification date
    };
    SourceFile *File(Part part = BOTH);

private:
    Tree_p      name;                   // Can be an infix, e.g. [A.B.C]
    text        key;                    // Key for that module
    SourceFile *specification;          // Specification if any
    SourceFile *implementation;         // Implementation if any
    text        syntax;                 // Custom syntax file if any
    text        stylesheet;             // Custom stylesheet if any was found
};

XL_END

#endif // MODULE_H
