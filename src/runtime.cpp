// *****************************************************************************
// runtime.cpp                                                        XL project
// *****************************************************************************
//
// File description:
//
//     Runtime functions necessary to execute XL programs
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
// (C) 2012, Baptiste Soulisse <baptiste.soulisse@taodyne.com>
// (C) 2011-2013, Catherine Burvelle <catherine@taodyne.com>
// (C) 2009-2020, Christophe de Dinechin <christophe@dinechin.org>
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

#include "runtime.h"
#include "tree.h"
#include "parser.h"
#include "scanner.h"
#include "renderer.h"
#include "context.h"
#include "options.h"
#include "main.h"
#include "module.h"
#include "native.h"
#include "save.h"
#include "tree-clone.h"
#include "utf8_fileutils.h"
#include "interpreter.h"
#include "types.h"
#include "winglob.h"

#ifndef INTERPRETER_ONLY
#include "compiler.h"
#endif // INTERPRETER_ONLY

#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>


XL_BEGIN


text xl_version()
// ----------------------------------------------------------------------------
//   Return the current XL version
// ----------------------------------------------------------------------------
{
    return XL_VERSION;
}
NATIVE(xl_version);


text xl_compatible_version()
// ----------------------------------------------------------------------------
//   Return the current XL version
// ----------------------------------------------------------------------------
{
    return XL_COMPATIBLE_VERSION;
}
NATIVE(xl_compatible_version);


Tree *  xl_evaluate(Scope *scope, Tree *tree)
// ----------------------------------------------------------------------------
//   Dispatch evaluation to the main entry point
// ----------------------------------------------------------------------------
{
    return MAIN->Evaluate(scope, tree);
}
NATIVE(xl_evaluate);


Tree *  xl_identity(Scope *scope, Tree *tree)
// ----------------------------------------------------------------------------
//   No op, returns the input tree
// ----------------------------------------------------------------------------
{
    return tree;
}


Tree *  xl_typecheck(Scope *scope, Tree *type, Tree *value)
// ----------------------------------------------------------------------------
//   Dispatch a type check to the current evaluator
// ----------------------------------------------------------------------------
{
    return MAIN->TypeCheck(scope, type, value);
}


Tree *xl_call(Scope *scope, text prefix, TreeList &argList)
// ----------------------------------------------------------------------------
//    Generate a call to the given prefix
// ----------------------------------------------------------------------------
{
    XLCall call(prefix);
    for (Tree *arg : argList)
        call.Arg(arg);
    return call(scope);
}


Tree *xl_assign(Scope *scope, Tree *ref, Tree *value)
// ----------------------------------------------------------------------------
//   Perform an assignment in the scope (temporary? interpreter only?)
// ----------------------------------------------------------------------------
{
    Context context(scope);
    return context.Assign(ref, value);
}


Tree *xl_form_error(Scope *scope, Tree *what)
// ----------------------------------------------------------------------------
//   Raise an error if we have a form error
// ----------------------------------------------------------------------------
{
    bool quickExit = false;     // For debugging purpose
    if (quickExit)
        return what;

    static bool recursive = false;
    if (recursive)
    {
        std::cerr << "ABORT - Recursive error during error handling\n"
                  << "Error tree: " << what << "\n";
        return XL::xl_false;
    }
    Save<bool> saveRecursive(recursive, true);

    Ooops("No pattern match $1 at runtime", what);
    return what;
}


Tree *xl_stack_overflow(Tree *what)
// ----------------------------------------------------------------------------
//   Return an error evaluating a tree
// ----------------------------------------------------------------------------
{
    Ooops("Stack overflow evaluating $1", what);
    return what;
}
NATIVE(xl_stack_overflow);


bool xl_same_text(Tree *what, const char *ref)
// ----------------------------------------------------------------------------
//   Compile the tree if necessary, then evaluate it
// ----------------------------------------------------------------------------
{
    Text *tval = what->AsText(); assert(tval);
    return tval->value == text(ref);
}
NATIVE(xl_same_text);


bool xl_same_shape(Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//   Check equality of two trees
// ----------------------------------------------------------------------------
{
    return Tree::Equal(left, right);
}


kstring xl_infix_name(Infix *infix)
// ----------------------------------------------------------------------------
//   Return the name of an infix as a C string
// ----------------------------------------------------------------------------
{
    return infix->name.c_str();
}



// ========================================================================
//
//    Creating entities (callbacks for compiled code)
//
// ========================================================================

Natural *xl_new_natural(TreePosition pos, ulonglong value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Natural
// ----------------------------------------------------------------------------
{
    Natural *result = new Natural(value, pos);
    return result;
}


Real *xl_new_real(TreePosition pos, double value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Real
// ----------------------------------------------------------------------------
{
    Real *result = new Real (value, pos);
    return result;
}


Text *xl_new_character(TreePosition pos, char value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new single-quoted Text
// ----------------------------------------------------------------------------
{
    Text *result = new Text(text(&value, 1), "'", "'", pos);
    return result;
}


Text *xl_new_text(TreePosition pos, text value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new double-quoted Text
// ----------------------------------------------------------------------------
{
    Text *result = new Text(value, pos);
    return result;
}


Text *xl_new_text_ptr(TreePosition pos, text *value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new double-quoted Text
// ----------------------------------------------------------------------------
{
    Text *result = new Text(*value, pos);
    return result;
}


Text *xl_new_ctext(TreePosition pos, kstring value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new double-quoted Text
// ----------------------------------------------------------------------------
{
    Text *result = new Text(text(value), pos);
    return result;
}


Text *xl_new_xtext(TreePosition pos, kstring value, longlong len,
                   kstring open, kstring close)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new arbitrarily-quoted Text
// ----------------------------------------------------------------------------
{
    Text *result = new Text(text(value, len), open, close, pos);
    return result;
}


Block *xl_new_block(Block *source, Tree *child)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new block
// ----------------------------------------------------------------------------
{
    Block *result = new Block(source, child);
    return result;
}


Prefix *xl_new_prefix(Prefix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Prefix
// ----------------------------------------------------------------------------
{
    Prefix *result = new Prefix(source, left, right);
    return result;
}


Postfix *xl_new_postfix(Postfix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Postfix
// ----------------------------------------------------------------------------
{
    Postfix *result = new Postfix(source, left, right);
    return result;
}


Infix *xl_new_infix(Infix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Infix
// ----------------------------------------------------------------------------
{
    Infix *result = new Infix(source, left, right);
    return result;
}



// ============================================================================
//
//    Parsing trees
//
// ============================================================================

static Tree *xl_parse_tree_inner(Scope *scope, Tree *tree)
// ----------------------------------------------------------------------------
//   Build a parse tree in the current scope
// ----------------------------------------------------------------------------
{
    Tree_p result = tree;
    switch(tree->Kind())
    {
    case NATURAL:
    case REAL:
    case TEXT:
    case NAME:
        return tree;

    case INFIX:
    {
        Infix *infix = (Infix *) tree;
        Tree *left = xl_parse_tree_inner(scope, infix->left);
        Tree *right = xl_parse_tree_inner(scope, infix->right);
        if (left != infix->left || right != infix->right)
            result = new Infix(infix, left, right);
        return result;
    }
    case PREFIX:
    {
        Prefix *prefix = (Prefix *) tree;
        Tree *left = xl_parse_tree_inner(scope, prefix->left);
        Tree *right = xl_parse_tree_inner(scope, prefix->right);
        if (left != prefix->left || right != prefix->right)
            result = new Prefix(prefix, left, right);
        return result;
    }
    case POSTFIX:
    {
        Postfix *postfix = (Postfix *) tree;
        Tree *left = xl_parse_tree_inner(scope, postfix->left);
        Tree *right = xl_parse_tree_inner(scope, postfix->right);
        if (left != postfix->left || right != postfix->right)
            result = new Postfix(postfix, left, right);
        break;
    }
    case BLOCK:
    {
        Block *block = (Block *) tree;
        if (Tree *expr = block->IsMetaBox())
        {
            result = xl_evaluate(scope, expr);
            if (!result)
                result = LastErrorAsErrorTree();
        }
        else
        {
            Tree *child = xl_parse_tree_inner(scope, block->child);
            if (child != block->child)
                result = new Block(block, child);
        }
        break;
    }
    }
    return result;
}


Tree *xl_parse_tree(Scope *scope, Tree *code)
// ----------------------------------------------------------------------------
//   Entry point for parse_tree
// ----------------------------------------------------------------------------
{
    if (Block *block = code->AsBlock())
        code = block->child;
    return xl_parse_tree_inner(scope, code);
}


Tree *xl_parse_text(text source)
// ----------------------------------------------------------------------------
//   Generate a tree from text
// ----------------------------------------------------------------------------
{
    std::istringstream input(source);
    Parser parser(input, MAIN->syntax,MAIN->positions,*MAIN->errors, "<text>");
    return parser.Parse();
}
NATIVE(xl_parse_text);



// ============================================================================
//
//   Write a tree to standard output (temporary hack)
//
// ============================================================================

static bool isAbsolute(text path)
// ----------------------------------------------------------------------------
//    Test absolute/relative path
// ----------------------------------------------------------------------------
{
    if (path == "")
        return false;
#ifdef HAVE_WINDOWS_H
    if (path[0] == '/' || path[0] == '\\')
        return true;
    if (path.length() >= 2 && path[1] == ':')
        return true;
    return false;
#else
    return (path[0] == '/');
#endif
}


static void xl_list_files(Scope *scope, Tree *patterns, Tree_p *&parent)
// ----------------------------------------------------------------------------
//   Append all files found in the parent
// ----------------------------------------------------------------------------
{
    if (Block *block = patterns->AsBlock())
    {
        xl_list_files(scope, block->child, parent);
        return;
    }
    if (Infix *infix = patterns->AsInfix())
    {
        if (IsCommaList(infix))
        {
            xl_list_files(scope, infix->left, parent);
            xl_list_files(scope, infix->right, parent);
            return;
        }
    }

    patterns = xl_evaluate(scope, patterns);
    if (Text *regexp = patterns->AsText())
    {
        glob_t files;
        text filename = regexp->value;
        glob(filename.c_str(), GLOB_MARK, nullptr, &files);
        for (uint i = 0; i < files.gl_pathc; i++)
        {
            std::string entry(files.gl_pathv[i]);
            Text *listed = new Text(entry);
            if (*parent)
            {
                Infix *added = new Infix(",", *parent, listed);
                *parent = added;
                parent = &added->right;
            }
            else
            {
                *parent = listed;
            }
        }
        globfree(&files);
        return;
    }
    Ooops("Malformed files list $1", patterns);
}



// ============================================================================
//
//   File utilities
//
// ============================================================================

Tree *xl_list_files(Scope *scope, Tree *patterns)
// ----------------------------------------------------------------------------
//   List all files in the given pattern
// ----------------------------------------------------------------------------
{
    Tree_p result = nullptr;
    Tree_p *parent = &result;
    xl_list_files(scope, patterns, parent);
    if (!result)
        result = xl_nil;
    return result;
}


bool xl_file_exists(Scope *scope, Tree_p self, text path)
// ----------------------------------------------------------------------------
//   Check if a file exists
// ----------------------------------------------------------------------------
{
    if (!isAbsolute(path))
    {
        if (!isAbsolute(path))
        {
            Context context(scope);
            // Relative path: look in same directory as parent
            if (Tree * dir = context.Named("module_dir"))
            {
                if (Text * txt = dir->AsText())
                    path = txt->value + "/" + path;
            }
        }
   }

    utf8_filestat_t st;
    return (utf8_stat (path.c_str(), &st) < 0) ? false : true;
}



// ============================================================================
//
//    Loading trees from external files
//
// ============================================================================

RECORDER(import, 32, "Import statements");


static Module::Info *xl_load_file(Scope *scope, Tree *self)
// ----------------------------------------------------------------------------
//    Load a file from disk, may evaluate it or not
// ----------------------------------------------------------------------------
{
    record(import, "Importing %t", self);
    Module::Info *info = self->GetInfo<Module::Info>();
    if (info)
        return info;

    // Create the info about the module
    info = new Module::Info(self);
    self->SetInfo(info);

    // Check that the import "looks good"
    Prefix *prefix = self->AsPrefix();
    if (!prefix)
    {
        Ooops("Malformed import statement $1", self);
        return info;
    }
    Tree *modname = prefix->right;

    // Check if we have something like [import IO = TEXT.IO]
    Name_p alias = nullptr;
    if (Infix *equal = modname->AsInfix())
    {
        if (equal->name == "=")
        {
            if (Name *name = equal->left->AsName())
                alias = name;
            else
                Ooops("Alias $1 is not a name", equal->left);
            modname = equal->right;
        }
    }
    info->alias = alias;

    // Load the module and compute the module's value
    Module *module = Module::Get(modname);
    info->module = module;
    record(import, "Imported %t into module %p info %p",
           prefix->right, module, info);
    return info;
}


Tree *xl_import(Scope *scope, Tree *self)
// ----------------------------------------------------------------------------
//    Import a file, which means loading it from disk and evaluating it
// ----------------------------------------------------------------------------
{
    xl_load_file(scope, self);
    return self;

}


Tree *xl_parse_file(Scope *scope, Tree *self)
// ----------------------------------------------------------------------------
//    Import a file, which means loading it from disk and evaluating it
// ----------------------------------------------------------------------------
{
    Module::Info *info = xl_load_file(scope, self);
    if (scope)
    {
        if (info->module)
            return info->module->Source();
        else
            return xl_nil;
    }
    return self;
}


struct LoadDataInfo : Info
// ----------------------------------------------------------------------------
//   Information about the data that was loaded
// ----------------------------------------------------------------------------
{
    LoadDataInfo(): files() {}
    struct PerFile
    {
        PerFile(): data(), loaded(), mtime(0) {}
        struct Row
        {
            TreeList        args;
        };
        typedef std::vector<Row> Data;
        Data        data;
        Tree_p      loaded;
        time_t      mtime;
    };
    std::map<text, PerFile> files;
};


Tree *xl_load_data(Scope *scope, Tree *self,
                   text name, text prefix, text fieldSeps, text recordSeps,
                   Tree *body)
// ----------------------------------------------------------------------------
//    Load a comma-separated or tab-separated file from disk
// ----------------------------------------------------------------------------
{
    // Open data file
    text path = MAIN->SearchFile(name);
    if (path == "")
    {
        Ooops("CSV file $2 not found in $1", self).Arg(name);
        return XL::xl_false;
    }

    utf8_ifstream input(path.c_str(), std::ifstream::in);
    if (!input.good())
    {
        Ooops("Unable to load data for $1.\n"
                     "(Accessing $2 resulted in the following error: $3)",
              self).Arg(path).Arg(strerror(errno));
        return XL::xl_nil;
    }

    return xl_load_data(scope, self, path,
                        input, true, true,
                        prefix, fieldSeps, recordSeps,
                        body);
}


Tree *xl_load_data(Scope *scope, Tree *self, text inputName,
                   std::istream &input, bool cached, bool statTime,
                   text prefix, text fieldSeps, text recordSeps,
                   Tree *body)
// ----------------------------------------------------------------------------
//   Variant reading from a stream directly
// ----------------------------------------------------------------------------
{
    // Check if the file has already been loaded somehwere.
    // If so, return the loaded data
    LoadDataInfo *info = self->GetInfo<LoadDataInfo>();
    if (!info)
    {
        // Create cache
        info = new LoadDataInfo();
        self->SetInfo<LoadDataInfo> (info);
    }

    // Load per-file cached data
    LoadDataInfo::PerFile &perFile = info->files[inputName];
    if (perFile.loaded)
    {
        if (statTime)
        {
            // Check if we need to reload the contents of the file
            // because it has been modified
            utf8_filestat_t st;
            utf8_stat (inputName.c_str(), &st);
            if (perFile.mtime != st.st_mtime)
                cached = false;
            perFile.mtime = st.st_mtime;
        }

        if (cached)
        {
            Tree_p result = xl_false;
            if (prefix.size())
            {
                ulong i, max = perFile.data.size();
                for (i = 0; i < max; i++)
                {
                    LoadDataInfo::PerFile::Row &row = perFile.data[i];
                    result = xl_call(scope, prefix, row.args);
                }
                return result;
            }
            return perFile.loaded;
        }

        // Restart with clean data
        perFile.data.clear();
        perFile.loaded = nullptr;
    }

    // Read data from file
    char     buffer[256];
    char    *ptr       = buffer;
    char    *end       = buffer + sizeof(buffer) - 1;
    Tree_p   tree      = nullptr;
    Tree_p   line      = nullptr;
    Tree_p  *treePtr   = &tree;
    Tree_p  *linePtr   = &line;
    bool     hasQuote  = false;
    bool     hasRecord = false;
    bool     hasField  = false;
    bool     hasPrefix = prefix.size() != 0;

    LoadDataInfo::PerFile::Row row;
    *end = 0;
    while (input.good())
    {
        int c = input.get();

        if (!hasQuote && ptr == buffer && c != '\n' && isspace(c))
            continue;

        if (hasQuote)
        {
            hasRecord = false;
            hasField = false;
        }
        else
        {
            hasRecord = recordSeps.find(c) != recordSeps.npos || c == EOF;
            hasField = fieldSeps.find(c) != fieldSeps.npos;
        }
        if (hasRecord || hasField)
        {
            c = 0;
        }
        else if (c == '"')
        {
            int next = input.peek();
            bool escapedQuote = (hasQuote && next == '"');
            if (escapedQuote)
                input.get();
            else
                hasQuote = !hasQuote;
        }

        if (ptr < end)
            *ptr++ = c;

        if (!c)
        {
            text token;
            Tree *child = nullptr;

            if (isdigit(buffer[0]) ||
                ((buffer[0] == '-' || buffer[0] == '+') && isdigit(buffer[1])))
            {
                char *ptr2 = nullptr;
                longlong l = strtoll(buffer, &ptr2, 10);
                if (ptr2 == ptr-1)
                {
                    child = new Natural(l);
                }
                else
                {
                    double d = strtod (buffer, &ptr2);
                    if (ptr2 == ptr-1)
                        child = new Real(d);
                }
            }
            if (child == nullptr)
            {
                if (buffer[0] == '"' && ptr > buffer+2 && ptr[-2] == '"')
                    token = text(buffer+1, ptr - buffer - 3);
                else
                    token = text(buffer, ptr - buffer - 1);
                child = new Text(token);
            }

            // Combine data that we just read to current line
            if (hasPrefix)
            {
                row.args.push_back(child);
            }
            else
            {
                if (*linePtr)
                {
                    Infix *infix = new Infix(",", *linePtr, child);
                    *linePtr = infix;
                    linePtr = &infix->right;
                }
                else
                {
                    *linePtr = child;
                }
            }

            // Combine as line if necessary
            if (hasRecord)
            {
                if (hasPrefix)
                {
                    if (body)
                        row.args.push_back(body);
                    perFile.data.push_back(row);
                    tree = xl_call(scope, prefix, row.args);
                    row.args.clear();
                }
                else
                {
                    if (body)
                    {
                        if (*linePtr)
                        {
                            Infix *infix = new Infix(",", *linePtr, body);
                            *linePtr = infix;
                            linePtr = &infix->right;
                        }
                        else
                        {
                            *linePtr = body;
                        }
                    }
                    if (*treePtr)
                    {
                        Infix *infix = new Infix("\n", *treePtr, line);
                        *treePtr = infix;
                        treePtr = &infix->right;
                    }
                    else
                    {
                        *treePtr = line;
                    }
                    line = nullptr;
                    linePtr = &line;
                }
            }
            ptr = buffer;
        }
    }
    if (!tree)
        tree = xl_false;
    perFile.loaded = tree;

    return tree;
}



// ============================================================================
//
//    C functions called from builtins.xl
//
// ============================================================================

extern "C"
{

bool xl_write_natural(ulonglong x)
// ----------------------------------------------------------------------------
//   Write a natural value
// ----------------------------------------------------------------------------
{
    // HACK: Until type system is more robust, print as signed value
    std::cout << (longlong) x;
    return true;
}
NATIVE(xl_write_natural);


bool xl_write_integer(longlong x)
// ----------------------------------------------------------------------------
//   Write an integer value
// ----------------------------------------------------------------------------
{
    std::cout << x;
    return true;
}
NATIVE(xl_write_integer);


bool xl_write_real(double x)
// ----------------------------------------------------------------------------
//   Write a double value
// ----------------------------------------------------------------------------
{
    std::cout << x;
    return true;
}
NATIVE(xl_write_real);


bool xl_write_text(kstring x)
// ----------------------------------------------------------------------------
//   Write a text value
// ----------------------------------------------------------------------------
{
    std::cout << x;
    return true;
}
NATIVE(xl_write_text);


bool xl_write_tree(Tree *tree)
// ----------------------------------------------------------------------------
//   Write a text value
// ----------------------------------------------------------------------------
{
    std::cout << tree;
    return true;
}
NATIVE(xl_write_tree);


bool xl_write_character(char x)
// ----------------------------------------------------------------------------
//   Write a character value
// ----------------------------------------------------------------------------
{
    std::cout << x;
    return true;
}
NATIVE(xl_write_character);


bool xl_write_cr()
// ----------------------------------------------------------------------------
//   Write a line separator
// ----------------------------------------------------------------------------
{
    std::cout << '\n';
    return true;
}
NATIVE(xl_write_cr);


bool xl_text_eq(kstring x, kstring y)
// ----------------------------------------------------------------------------
//   Compare two texts
// ----------------------------------------------------------------------------
{
    return strcmp(x, y) == 0;
}
NATIVE(xl_text_eq);


bool xl_text_ne(kstring x, kstring y)
// ----------------------------------------------------------------------------
//   Compare two texts
// ----------------------------------------------------------------------------
{
    return strcmp(x, y) != 0;
}
NATIVE(xl_text_ne);


bool xl_text_lt(kstring x, kstring y)
// ----------------------------------------------------------------------------
//   Compare two texts
// ----------------------------------------------------------------------------
{
    return strcmp(x, y) < 0;
}
NATIVE(xl_text_lt);


bool xl_text_le(kstring x, kstring y)
// ----------------------------------------------------------------------------
//   Compare two texts
// ----------------------------------------------------------------------------
{
    return strcmp(x, y) <= 0;
}
NATIVE(xl_text_le);


bool xl_text_gt(kstring x, kstring y)
// ----------------------------------------------------------------------------
//   Compare two texts
// ----------------------------------------------------------------------------
{
    return strcmp(x, y) > 0;
}
NATIVE(xl_text_gt);


bool xl_text_ge(kstring x, kstring y)
// ----------------------------------------------------------------------------
//   Compare two texts
// ----------------------------------------------------------------------------
{
    return strcmp(x, y) >= 0;
}
NATIVE(xl_text_ge);


natural_t xl_text2int(kstring tval)
// ----------------------------------------------------------------------------
//   Converts text to a numerical value
// ----------------------------------------------------------------------------
{
    text t = tval;
    std::istringstream stream(t);
    natural_t result = 0;
    stream >> result;
    return result;
}
NATIVE(xl_text2int);


real_t xl_text2real(kstring tval)
// ----------------------------------------------------------------------------
//   Converts text to a numerical value
// ----------------------------------------------------------------------------
{
    text t = tval;
    std::istringstream stream(t);
    real_t result = 0.0;
    stream >> result;
    return result;
}
NATIVE(xl_text2real);


text xl_natural2text(natural_t value)
// ----------------------------------------------------------------------------
//   Convert a numerical value to text
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << value;
    return out.str();
}
NATIVE(xl_natural2text);


text xl_integer2text(integer_t value)
// ----------------------------------------------------------------------------
//   Convert an integer value to text
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << value;
    return out.str();
}
NATIVE(xl_integer2text);


text xl_real2text(real_t value)
// ----------------------------------------------------------------------------
//   Convert a numerical value to text
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << value;
    return out.str();
}
NATIVE(xl_real2text);


natural_t xl_natural_pow(natural_t x, natural_t y)
// ----------------------------------------------------------------------------
//   Compute natural power
// ----------------------------------------------------------------------------
{
    natural_t tmp = 1;
    while (y)
    {
        if (y & 1)
            tmp *= x;
        x *= x;
        y >>= 1;
    }
    return tmp;
}
NATIVE(xl_natural_pow);


integer_t xl_integer_mod(integer_t x, integer_t y)
// ----------------------------------------------------------------------------
//   Compute a mathematical 'mod' from the C99 % operator
// ----------------------------------------------------------------------------
{
    integer_t tmp = x % y;
    if (tmp && (x^y) < 0)
        tmp += y;
    return tmp;
}
NATIVE(xl_integer_mod);


integer_t xl_integer_rem(integer_t x, integer_t y)
// ----------------------------------------------------------------------------
//   Compute a mathematical 'rem' from the C99 % operator
// ----------------------------------------------------------------------------
{
    integer_t rem = x % y;
    return rem;
}
NATIVE(xl_integer_rem);


integer_t xl_integer_pow(integer_t x, integer_t y)
// ----------------------------------------------------------------------------
//   Compute natural power
// ----------------------------------------------------------------------------
{
    natural_t tmp = y >= 0 ? 1 : 0;
    while (y > 0)
    {
        if (y & 1)
            tmp *= x;
        x *= x;
        y >>= 1;
    }
    return tmp;
}
NATIVE(xl_integer_pow);


real_t xl_real_mod(real_t x, real_t y)
// ----------------------------------------------------------------------------
//   Compute a mathematical 'mod' from fmod
// ----------------------------------------------------------------------------
{
    real_t mod = fmod(x,y);
    if (mod != 0.0 && x*y < 0.0)
        mod += y;
    return mod;
}
NATIVE(xl_real_mod);


real_t xl_real_rem(real_t x, real_t y)
// ----------------------------------------------------------------------------
//   Compute a mathematical 'rem' from fmod
// ----------------------------------------------------------------------------
{
    return fmod(x,y);
}
NATIVE(xl_real_rem);


real_t xl_real_pow(real_t x, integer_t y)
// ----------------------------------------------------------------------------
//   Compute real power with an natural on the right
// ----------------------------------------------------------------------------
{
    bool     negative = y < 0;
    real_t   tmp      = 1.0;
    if (negative)
        y = -y;
    while (y)
    {
        if (y & 1)
            tmp *= x;
        x *= x;
        y >>= 1;
    }
    if (negative)
        tmp = 1.0/tmp;
    return tmp;
}
NATIVE(xl_real_pow);


text xl_text_replace(text txt, text before, text after)
// ----------------------------------------------------------------------------
//   Return a copy of txt with all occurrences of before replaced with after
// ----------------------------------------------------------------------------
{
    size_t pos = 0;
    while ((pos = txt.find(before, pos)) != text::npos)
    {
        txt.replace(pos, before.length(), after);
        pos += after.length();
    }
    return txt;
}
NATIVE(xl_text_replace);


integer_t xl_text_index(text haystack, text needle, size_t pos)
// ----------------------------------------------------------------------------
//   Return a copy of txt with all occurrences of before replaced with after
// ----------------------------------------------------------------------------
{
    return haystack.find(needle, pos);
}
NATIVE(xl_text_index);


text_t xl_text_range(text Source, natural_t first, natural_t last)
// ----------------------------------------------------------------------------
//   Extract a substring from a string
// ----------------------------------------------------------------------------
{
    if (last < first)
        return "";
    return Source.substr(first, last - first);
}
NATIVE(xl_text_range);


natural_t xl_text_length(text t)
// ----------------------------------------------------------------------------
//   Return the length of the text
// ----------------------------------------------------------------------------
{
    return t.length();
}
NATIVE(xl_text_length);



text xl_text_concat(text first, text second)
// ----------------------------------------------------------------------------
//   Return a concatenation of two strings
// ----------------------------------------------------------------------------
{
    return first + second;
}
NATIVE(xl_text_concat);


text xl_text_repeat(uint count, text txt)
// ----------------------------------------------------------------------------
//   Return a copy of txt with all occurrences of before replaced with after
// ----------------------------------------------------------------------------
{
    text result = "";
    uint shift = 1;
    while (count)
    {
        if (count & shift)
        {
            result += txt;
            count &= ~shift;
        }
        if (count)
        {
            shift <<= 1;
            txt += txt;
        }
    }
    return result;
}
NATIVE(xl_text_repeat);


real_t xl_time(real_t delay)
// ----------------------------------------------------------------------------
//   Return the current system time
// ----------------------------------------------------------------------------
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    MAIN->Refresh(delay);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}
NATIVE(xl_time);


#define XL_TIME(tmfield, delay)               \
    struct tm tm = { 0 };                       \
    time_t clock = { 0 };                       \
    time(&clock);                               \
    localtime_r(&clock, &tm);                   \
    MAIN->Refresh(delay);                       \
    return tm.tmfield


natural_t  xl_seconds()     { XL_TIME(tm_sec,       1.0); }
natural_t  xl_minutes()     { XL_TIME(tm_min,      60.0); }
natural_t  xl_hours()       { XL_TIME(tm_hour,    3600.0); }
natural_t  xl_month_day()   { XL_TIME(tm_mday,   86400.0); }
natural_t  xl_month()       { XL_TIME(tm_mon,    86400.0); }
natural_t  xl_year()        { XL_TIME(tm_year,   86400.0); }
natural_t  xl_week_day()    { XL_TIME(tm_wday,   86400.0); }
natural_t  xl_year_day()    { XL_TIME(tm_yday,   86400.0); }
natural_t  xl_summer_time() { XL_TIME(tm_isdst,  86400.0); }
#ifndef CONFIG_MINGW
text_t     xl_timezone()    { XL_TIME(tm_zone,   86400.0); }
natural_t  xl_GMT_offset()  { XL_TIME(tm_gmtoff, 86400.0); }
#endif // CONFIG_MINGW

NATIVE(xl_seconds);
NATIVE(xl_minutes);
NATIVE(xl_hours);
NATIVE(xl_month_day);
NATIVE(xl_month);
NATIVE(xl_year);
NATIVE(xl_week_day);
NATIVE(xl_year_day);
NATIVE(xl_summer_time);
#ifndef CONFIG_MINGW
NATIVE(xl_timezone);
NATIVE(xl_GMT_offset);
#endif // CONFIG_MINGW

real_t xl_random()
// ----------------------------------------------------------------------------
//    Return a pseudo-random number in the low..high range
// ----------------------------------------------------------------------------
{
#ifdef HAVE_DRAND48
    return drand48();
#else // !HAVE_DRAND48
    return real_t(rand()) / RAND_MAX;
#endif // HAVE_DRAND48
}
NATIVE(random);


bool xl_random_seed(int seed)
// ----------------------------------------------------------------------------
//    Initialized random number generator using the argument passed as seed
// ----------------------------------------------------------------------------
{
#ifdef HAVE_DRAND48
    srand48(seed);
#else // !HAVE_DRAND48
    srand(seed);
#endif // HAVE_DRAND48

    return true;
}
NATIVE(xl_random_seed);

} // extern "C"


// ============================================================================
//
//   Managing calls to/from XL
//
// ============================================================================

XLCall &XLCall::Arg(Tree *tree)
// ----------------------------------------------------------------------------
//   Create an argument from the given tree
// ----------------------------------------------------------------------------
{
    call = nullptr;
    if (*pointer)
    {
        Infix *infix = new Infix(",", *pointer, tree);
        *pointer = infix;
        pointer = (Tree_p *) &infix->right;
    }
    else
    {
        *pointer = tree;
    }
    call = nullptr;             // Make sure we rebuild
    return *this;
}


Tree *XLCall::Build()
// ----------------------------------------------------------------------------
//   Build the final call
// ----------------------------------------------------------------------------
{
    if (!call)
    {
        if (arguments)
            call = new Prefix(name, arguments);
        else
            call = name;
    }
    return call;
}


bool XLCall::Analyze(Scope *scope)
// ----------------------------------------------------------------------------
//    Perform the given call in the given context
// ----------------------------------------------------------------------------
{
    if (!call)
        Build();
    assert(scope && call);

    Errors errors;
    errors.Log (Error("Unable to evaluate call $1:", call), true);

    Types types(scope);
    Tree *type = types.TypeAnalysis(call);
    bool result = type != nullptr && type != xl_error && !errors.HadErrors();

    return result;
}


Tree *XLCall::operator() (Scope *scope)
// ----------------------------------------------------------------------------
//    Perform the given call in the given context
// ----------------------------------------------------------------------------
{
    Tree *result = xl_nil;
    if (Analyze(scope))
        result = MAIN->Evaluate(scope, call);
    return result;
}


XL_END


uint xl_recursion_count = 0;
