// *****************************************************************************
// module.cpp                                                         XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2021, Christophe de Dinechin <christophe@dinechin.org>
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

Module *Module::Get(Tree *name)
// ----------------------------------------------------------------------------
//   Lookup a module, or load it
// ----------------------------------------------------------------------------
{
    Scope *scope = MAIN->context;
    if (Infix *nested = name->AsInfix())
    {
        Module *parent = Get(nested->left);
        scope = parent->FileScope(scope);
    }

    text key = Name(name);

    // Silently fail to load the module at syntax loading time
    if (key == "")
        return nullptr;

    Module *module = MAIN->modules[key];
    if (!module)
        module = new Module(scope, name, key);

    return module;
}


Module *Module::Get(text name)
// ----------------------------------------------------------------------------
//   Lookup a module by name
// ----------------------------------------------------------------------------
{
    return Get(new Text(name, Tree::COMMAND_LINE));
}


inline Module::SourceFile *Module::File(Part first, Part part)
// ----------------------------------------------------------------------------
//   Select which component we look at
// ----------------------------------------------------------------------------
{
    if (first == SPECIFICATION && (part & SPECIFICATION) && specification)
        return specification;
    if ((part & IMPLEMENTATION) && implementation)
        return implementation;
    if (first == IMPLEMENTATION && (part & SPECIFICATION) && specification)
        return specification;
    return nullptr;
}


bool Module::IsValid(Part part)
// ----------------------------------------------------------------------------
//   Check if we have a source for specification or implementation
// ----------------------------------------------------------------------------
{
    return File(SPECIFICATION, part) != nullptr;
}


Tree *Module::Source(Part part)
// ----------------------------------------------------------------------------
//   Return the source of the implementation first, or specification
// ----------------------------------------------------------------------------
{
    if (SourceFile *file = File(IMPLEMENTATION, part))
        return file->Source();
    return nullptr;
}


Tree *Module::Value(Part part)
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
    if (SourceFile *file = File(IMPLEMENTATION, part))
        return file->Value();
    return nullptr;
}


Scope *Module::FileScope(Scope *where, Part part)
// ----------------------------------------------------------------------------
//   Scope in which we search for the module symbols
// ----------------------------------------------------------------------------
{
    if (SourceFile *file = File(SPECIFICATION, part))
        return file->FileScope(where);
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
    return Source();
}


Scope *Module::Process(Initializers &inits)
// ----------------------------------------------------------------------------
//   Process module specification and implementation, fill inits
// ----------------------------------------------------------------------------
{
    // Process module implementation first
    Scope *base = scope;
    if (Tree *implSrc = Source(IMPLEMENTATION))
    {
        if (Scope *implScope = FileScope(base, IMPLEMENTATION))
        {
            Context implCtx(implScope);
            implCtx.ProcessDeclarations(implSrc, inits);
            base = implCtx.Symbols();
        }
    }

    // Process module specification and check it against ipmlementation
    if (Tree *specSrc = Source(SPECIFICATION))
    {
        if (Scope *specScope = FileScope(base, SPECIFICATION))
        {
            Context specCtx(specScope);
            Context implCtx(base);
            specCtx.ProcessSpecifications(implCtx, specSrc, inits);
            base = specCtx.Symbols();
        }
    }

    // Return the scope to use for lookup
    return base;
}


static text SearchModuleFile(text name, text extension)
// ----------------------------------------------------------------------------
//    Search a module file with the given extension
// ----------------------------------------------------------------------------
{
    text path = MAIN->SearchFile(name, extension);
    if (path == "")
        path = MAIN->SearchLibFile(name, extension);
    return path;
}


Module::Module(Scope *scope, Tree *modname, text key)
// ----------------------------------------------------------------------------
//   Create a new module
// ----------------------------------------------------------------------------
    : scope(scope),
      name(modname),
      key(key),
      specification(),
      implementation(),
      syntax(SearchModuleFile(key, "syntax")),
      stylesheet(SearchModuleFile(key, "stylesheet"))
{
    text specPath = SearchModuleFile(key, "xs");
    text implPath = SearchModuleFile(key, "xl");

    record(modules,
           "Module %t key '%s' "
           "syntax '%s' style '%s' spec '%s' impl '%s'",
           modname, key, syntax, stylesheet, specPath, implPath);

    // Load the syntax file first if it exists
    if (syntax != "")
        Syntax::syntax->ReadSyntaxFile(syntax.c_str());

    // Once we have the syntax, we can render using it. Useful for debugging
    if (stylesheet != "")
        Renderer::renderer->SelectStyleSheet(stylesheet, false);

    // Load specification then implementation
    if (specPath != "")
        specification = new SourceFile(specPath);
    if (implPath != "")
        implementation = new SourceFile(implPath);

    // Emit error message if we did not find anything
    if (!specPath.length() && !implPath.length())
        Ooops("Module $1 not found", modname);

    // Insert in the list of modules
    MAIN->modules[key] = this;
}


Module::~Module()
// ----------------------------------------------------------------------------
//   Deleting a module
// ----------------------------------------------------------------------------
{
    MAIN->modules.erase(key);
    delete specification;
    delete implementation;
}


text Module::Name(Tree *name)
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
        modname = modname.length()
            ? (base->value + "." + modname)
            : base->value;

    // Check if we need to evaluate the module name
    if (modname == "")
        if (Text *text = name->AsText())
            modname = text->value;

    if (modname == "")
        Ooops("The module name $1 is not valid", name);

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


Module::SourceFile::SourceFile(text path)
// ----------------------------------------------------------------------------
//   Load a source file
// ----------------------------------------------------------------------------
    : path(path),
      source(),
      scope(),
      value(),
      modified(FileModifiedTimeStamp(path))
{}


Module::SourceFile::~SourceFile()
// ----------------------------------------------------------------------------
//   Delete a source file
// ----------------------------------------------------------------------------
{}


Tree *Module::SourceFile::Source()
// ----------------------------------------------------------------------------
//   Return the source code
// ----------------------------------------------------------------------------
{
    if (!source)
        Reload();
    return source;
}


Scope *Module::SourceFile::FileScope(Scope *where)
// ----------------------------------------------------------------------------
//    Return the scope for that module
// ----------------------------------------------------------------------------
{
    if (!scope && where)
    {
        Context context(where);
        context.CreateScope(where->Position());
        scope = context.Symbols();

        // Enter module name, path in the scope
        context.SetModulePath(path);
    }
    return scope;
}


Tree *Module::SourceFile::Value()
// ----------------------------------------------------------------------------
//   Return the value for the module
// ----------------------------------------------------------------------------
{
    if (!value)
        if (Tree *source = Source())
            if (Scope *scope = FileScope(MAIN->context))
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


Tree *Module::SourceFile::Reload()
// ----------------------------------------------------------------------------
//   Reload the source file and recompute the value
// ----------------------------------------------------------------------------
{
    record(fileload, "Loading %s in source file %p", path, this);

    // Clear value and scope
    value = nullptr;
    scope = nullptr;

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
            Ooops("Failed to load module $1", positions.Here())
            .Arg(path);
    else
        // Normalize source if necessary (used in Tao3D)
        source = MAIN->Normalize(source);

    // Show source if requested
    if (Opt::showSource)
        std::cout << source << "\n";

    return Source();
}

XL_END
