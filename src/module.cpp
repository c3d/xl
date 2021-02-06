// ****************************************************************************
//  module.cpp                                                    Tao3D project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of XL modules
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
//   This file is part of Tao3D.
//
//   Tao3D is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Tao3D is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Tao3D.  If not, see <https://www.gnu.org/licenses/>.
// ****************************************************************************

#include "module.h"

#include "errors.h"
#include "main.h"
#include "utf8_fileutils.h"

#include <recorder/recorder.h>


RECORDER(modules, 32, "Modules");

XL_BEGIN


// ============================================================================
//
//    Module class
//
// ============================================================================

Module *Module::Get(Scope *where, Tree *name, bool evaluate)
// ----------------------------------------------------------------------------
//   Lookup a module, or load it
// ----------------------------------------------------------------------------
{
    text key = Name(where, name);
    Module *module = modules[key];
    if (!module)
    {
        module = new Module(where, name, key, evaluate);
        modules[key] = module;
    }
    return module;
}


Tree *Module::Value()
// ----------------------------------------------------------------------------
//   Return the evaluated value
// ----------------------------------------------------------------------------
//   The evaluated value is either:
//   - An error value if there was any error during evaluation
//   - The specification value if there is a specification
//   - The implementation value otherwise
//   Most modules are expected to not evaluate at top-level, but return a scope
//   however, this is not mandated by the language
{
    if (!value)
    {
        if (specification)
            value = specification->Value();
        else if (implementation)
            value = implementation->Value();
        else
            value = Ooops("No specification or implementation for module $1",
                          name);
    }
    return value;
}


Scope *Module::Specification()
// ----------------------------------------------------------------------------
//    Return the specification scope
// ----------------------------------------------------------------------------
{
    if (specification)
        return specification->Scope();
    return nullptr;
}


Scope *Module::Implementation()
// ----------------------------------------------------------------------------
//    Return the implementation scope
// ----------------------------------------------------------------------------
{
    if (implementation)
        return implementation->Scope();
    return nullptr;
}


text Module::Syntax()
// ----------------------------------------------------------------------------
//   Return the custom syntax if there is one
// ----------------------------------------------------------------------------
{
    return syntax;
}


text Module::StyleSheet()
// ----------------------------------------------------------------------------
//   Return the style sheet for the module if there is one
// ----------------------------------------------------------------------------
{
    return stylesheet;
}


bool Module::HasChanged()
// ----------------------------------------------------------------------------
//   Check if the module source code has changed
// ----------------------------------------------------------------------------
{
    if (specification && specification->HasChanged())
        return true;
    if (implementation && implementation->HasChanged())
        return true;
    return false;
}


Tree_p Module::Reload()
// ----------------------------------------------------------------------------
//    Force reload the source file e.g. if it changed or syntax was changed
// ----------------------------------------------------------------------------
{
    if (specification)
        specification->Reload();
    if (implementation)
        implementation->Reload();
    return Value();
}


static text SearchModuleFile(text name, text extension)
// ----------------------------------------------------------------------------
//    Search a module file with the given extension
// ----------------------------------------------------------------------------
{
    text path = MAIN->SearchFile(name, extension);
    if (path == "")
        path = MAIN->SearchLibFile(name, extension);
    return "";
}


Module::Module(Scope *where, Tree *modname, text key, bool evaluate)
// ----------------------------------------------------------------------------
//   Create a new module
// ----------------------------------------------------------------------------
    : name(modname),
      key(key),
      value(),
      specification(),
      implementation(),
      syntax(SearchModuleFile(key, "syntax")),
      stylesheet(SearchModuleFile(key, "stylesheet"))
{
    text specPath = SearchModuleFile(key, "xs");
    text implPath = SearchModuleFile(key, "xl");

    // Load the syntax file first if it exists (will need to reload everything)
    bool hasSyntax = syntax.length();
    if (hasSyntax)
        Syntax::syntax->ReadSyntaxFile(syntax.c_str());

    // Once we have the syntax, we can render using it. Useful for debugging
    if (stylesheet.length())
        Renderer::renderer->SelectStyleSheet(stylesheet, false);

    // Load specification then implementation
    if (specPath.length())
        specification = new SourceFile(where, specPath, evaluate);
    if (implPath.length())
        implementation = new SourceFile(where, implPath, evaluate);

    // If we have a custom syntax, we need to reload existing modules
    // REVISIT - This happens during 'import',
    // i.e. Context::ProcessDeclarations() - How do we restart there?
    if (hasSyntax)
        for (auto mod : modules)
            mod.second->Reload();
}


Module::~Module()
// ----------------------------------------------------------------------------
//   Deleting a module
// ----------------------------------------------------------------------------
{
    modules.erase(key);
    delete specification;
    delete implementation;
}


text Module::Name(Scope *where, Tree *name)
// ----------------------------------------------------------------------------
//    Return the module name for a specification like [A.B.C]
// ----------------------------------------------------------------------------
{
    text modname;

    // Check if the name is something like [XL.TEXT.IO]
    while (Infix *dot = name->AsInfix())
    {
        if (XL::Name *inner = dot->right->AsName())
            modname = inner->value + "." + modname;
        else
            Ooops("Module component $1 is not a name", dot->right);
        name = dot->left;
    }

    // Check trailing name
    if (XL::Name *base = name->AsName())
        modname = base->value + "." + modname;

    // Check if we need to evaluate the module name
    if (modname == "")
    {
        Tree *evaluated = MAIN->Evaluate(where, name);
        if (Text *text = evaluated->AsText())
            modname = text->value;
        else
            Ooops("The module name $1 is not valid", name);
    }

    return modname;
}



// ============================================================================
//
//   Module::SourceFile class
//
// ============================================================================

static inline time_t FileModifiedTimeStamp(text path)
// ----------------------------------------------------------------------------
//   Return the time stamp for a given file
// ----------------------------------------------------------------------------
{
    time_t modified = 0;
    utf8_filestat_t st;
    if (utf8_stat(path.c_str(), &st) >= 0)
        modified = st.st_mtime;
    return modified;
}


Module::SourceFile::SourceFile(XL::Scope *where, text path, bool evaluate)
// ----------------------------------------------------------------------------
//   Load a source file
// ----------------------------------------------------------------------------
    : path(path),
      source(),
      scope(),
      value(),
      modified(FileModifiedTimeStamp(path)),
      evaluate(evaluate)
{
    Context context(where);
    context.CreateScope(MAIN->positions.CurrentPosition());
    scope = context.Symbols();
    Reload();
}


Module::SourceFile::~SourceFile()
// ----------------------------------------------------------------------------
//   Delete a source file
// ----------------------------------------------------------------------------
{}


Tree_p Module::SourceFile::Source()
// ----------------------------------------------------------------------------
//   Return the source code
// ----------------------------------------------------------------------------
{
    if (!source)
        Reload();
    return source;
}


Scope_p Module::SourceFile::Scope()
// ----------------------------------------------------------------------------
//    Return the scope for that module
// ----------------------------------------------------------------------------
{
    return scope;
}


Tree_p Module::SourceFile::Value()
// ----------------------------------------------------------------------------
//   Return the value for the module
// ----------------------------------------------------------------------------
{
    if (!value && evaluate)
        if (Tree *source = Source())
            value = MAIN->Evaluate(scope, source);
    return value;
}


bool Module::SourceFile::HasChanged()
// ----------------------------------------------------------------------------
//   Check if the file has changed since last time
// ----------------------------------------------------------------------------
{
    time_t now = FileModifiedTimeStamp(path);
    bool changed = now != modified;
    if (changed)
        modified = now;
    return changed;
}


Tree_p Module::SourceFile::Reload()
// ----------------------------------------------------------------------------
//   Reload the source file and recompute the value
// ----------------------------------------------------------------------------
{
    record(fileload, "Loading %s in source file %p", path, this);

    // Clear value and scope
    value = nullptr;
    scope->Clear();

    XL::Syntax  &syntax = MAIN->syntax;
    Positions   &positions = MAIN->positions;
    Errors      &errors = MAIN->topLevelErrors;
    Parser       parser(path.c_str(), syntax, positions, errors);

    source = parser.Parse();
    modified = FileModifiedTimeStamp(path);
    record(fileload, "Loaded %s as %t - time %llu", path, source, modified);

    // If we had a parse error, return an error
    if (!source)
        source =
            Ooops("Failed to load module $1", positions.CurrentPosition())
            .Arg(path);

    // Normalize source if necessary (used in Tao3D)
    source = MAIN->Normalize(source);

    // Show source if requested
    if (Opt::showSource)
        std::cout << source << "\n";

    // Enter module name, path in the scope
    Context context(scope);
    context.SetModulePath(path);

    return Value();
}


std::map<text, Module *> XL::Module::modules;


XL_END
