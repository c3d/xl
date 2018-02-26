// ****************************************************************************
//  runtime.cpp                     (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                              XL project
// ****************************************************************************
//
//   File Description:
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

#include "runtime.h"
#include "tree.h"
#include "parser.h"
#include "scanner.h"
#include "renderer.h"
#include "context.h"
#include "options.h"
#include "opcodes.h"
#include "main.h"
#include "save.h"
#include "tree-clone.h"
#include "utf8_fileutils.h"
#include "interpreter.h"
#include "winglob.h"

#ifndef INTERPRETER_ONLY
#include "compiler.h"
#include "compiler-gc.h"
#include "types.h"
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

Tree *xl_form_error(Context *context, Tree *what)
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

    Ooops("No form match $1 at runtime", what);
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


bool xl_same_shape(Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//   Check equality of two trees
// ----------------------------------------------------------------------------
{
    return Tree::Equal(left, right);
}



// ========================================================================
//
//    Creating entities (callbacks for compiled code)
//
// ========================================================================

Integer *xl_new_integer(longlong value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Integer
// ----------------------------------------------------------------------------
{
    Integer *result = new Integer(value);
    return result;
}


Real *xl_new_real(double value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Real
// ----------------------------------------------------------------------------
{
    Real *result = new Real (value);
    return result;
}


Text *xl_new_character(char value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new single-quoted Text
// ----------------------------------------------------------------------------
{
    Text *result = new Text(text(value, 1), "'", "'");
    return result;
}


Text *xl_new_text(text value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new double-quoted Text
// ----------------------------------------------------------------------------
{
    Text *result = new Text(value);
    return result;
}


Text *xl_new_ctext(kstring value)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new double-quoted Text
// ----------------------------------------------------------------------------
{
    Text *result = new Text(text(value));
    return result;
}


Text *xl_new_xtext(kstring value, longlong len, kstring open, kstring close)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new arbitrarily-quoted Text
// ----------------------------------------------------------------------------
{
    Text *result = new Text(text(value, len), open, close);
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

Tree *xl_parse_tree_inner(Context *context, Tree *tree)
// ----------------------------------------------------------------------------
//   Build a parse tree in the current context
// ----------------------------------------------------------------------------
{
    switch(tree->Kind())
    {
    case INTEGER:
    case REAL:
    case TEXT:
    case NAME:
        return tree;
    case INFIX:
    {
        Infix *infix = (Infix *) tree;
        Tree *left = xl_parse_tree_inner(context, infix->left);
        Tree *right = xl_parse_tree_inner(context, infix->right);
        Infix *result = new Infix(infix, left, right);
        return result;
    }
    case PREFIX:
    {
        Prefix *prefix = (Prefix *) tree;
        Tree *left = xl_parse_tree_inner(context, prefix->left);
        Tree *right = xl_parse_tree_inner(context, prefix->right);
        Prefix *result = new Prefix(prefix, left, right);
        return result;
    }
    case POSTFIX:
    {
        Postfix *postfix = (Postfix *) tree;
        Tree *left = xl_parse_tree_inner(context, postfix->left);
        Tree *right = xl_parse_tree_inner(context, postfix->right);
        Postfix *result = new Postfix(postfix, left, right);
        return result;
    }
    case BLOCK:
    {
        Block *block = (Block *) tree;
        Tree *result = block->child;
        if (block->opening == "{" && block->closing == "}")
        {
            Block *child = result->AsBlock();
            if (child && child->opening == "{" && child->closing == "}")
            {
                // Case where we have parse_tree {{x}}: Return {x}
                result = xl_parse_tree_inner(context, child->child);
                result = new Block(block, result);
                return result;
            }

            // Name or expression in { }
            result = context->Evaluate(result);
            return result;
        }
        result = xl_parse_tree_inner(context, result);
        result = new Block(block, result);
        return result;
    }
    }
    return tree;
}


Tree *xl_parse_tree(Context *context, Tree *code)
// ----------------------------------------------------------------------------
//   Entry point for parse_tree
// ----------------------------------------------------------------------------
{
    if (Block *block = code->AsBlock())
        code = block->child;
    return xl_parse_tree_inner(context, code);
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
#if defined (CONFIG_MINGW)
    if (path[0] == '/' || path[0] == '\\')
        return true;
    if (path.length() >= 2 && path[1] == ':')
        return true;
    return false;
#else
    return (path[0] == '/');
#endif
}


static void xl_list_files(Context *context, Tree *patterns, Tree_p *&parent)
// ----------------------------------------------------------------------------
//   Append all files found in the parent
// ----------------------------------------------------------------------------
{
    if (Block *block = patterns->AsBlock())
    {
        xl_list_files(context, block->child, parent);
        return;
    }
    if (Infix *infix = patterns->AsInfix())
    {
        if (infix->name == "," || infix->name == ";" || infix->name == "\n")
        {
            xl_list_files(context, infix->left, parent);
            xl_list_files(context, infix->right, parent);
            return;
        }
    }

    patterns = context->Evaluate(patterns);
    if (Text *regexp = patterns->AsText())
    {
        glob_t files;
        text filename = regexp->value;
        glob(filename.c_str(), GLOB_MARK, NULL, &files);
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

Tree *xl_list_files(Context *context, Tree *patterns)
// ----------------------------------------------------------------------------
//   List all files in the given pattern
// ----------------------------------------------------------------------------
{
    Tree_p result = NULL;
    Tree_p *parent = &result;
    xl_list_files(context, patterns, parent);
    if (!result)
        result = xl_nil;
    return result;
}


bool xl_file_exists(Context *context, Tree_p self, text path)
// ----------------------------------------------------------------------------
//   Check if a file exists
// ----------------------------------------------------------------------------
{
    if (!isAbsolute(path))
    {
        path = context->ResolvePrefixedPath(path);
        if (!isAbsolute(path))
        {
            // Relative path: look in same directory as parent
            if (Tree * dir = context->Named("module_dir"))
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

struct ImportedFileInfo : Info
// ----------------------------------------------------------------------------
//   Information about a file that was imported (save full path)
// ----------------------------------------------------------------------------
{
    ImportedFileInfo(text path)
        : path(path) {}
    text      path;
};


Tree *xl_import(Context *context, Tree *self, text name, int phase)
// ----------------------------------------------------------------------------
//    Load a file from disk without evaluating it
// ----------------------------------------------------------------------------
{
    text path = "";
    ImportedFileInfo *info = self->GetInfo<ImportedFileInfo>();
    if (info)
    {
        path = info->path;
    }
    else
    {
        if (path == "")
            path = MAIN->SearchFile(name);
        if (path == "" && !isAbsolute(name))
        {
            // Relative path: look in same directory as parent
            static Name_p module_dir = new Name("module_dir");
            if (Tree * dir = context->Bound(module_dir))
            {
                if (Text * txt = dir->AsText())
                {
                    path = txt->value + "/" + name;
                    utf8_filestat_t st;
                    if (utf8_stat (path.c_str(), &st) < 0)
                        path = "";
                }
            }
        }
        if (path == "")
        {
            Ooops("Source file $2 not found for $1", self).Arg(name);
            return XL::xl_false;
        }
        info = new ImportedFileInfo(path);
        self->SetInfo<ImportedFileInfo> (info);
    }

    // Check if the file has already been loaded somehwere.
    // If so, return the loaded file
    bool exists = MAIN->files.count(path);
    if (!exists)
    {
        record(file_load, "Loading %s", path.c_str());
        bool hadError = MAIN->LoadFile(path);
        if (hadError)
        {
            Ooops("Unable to load file $2 for $1", self).Arg(path);
            return XL::xl_false;
        }
    }

    SourceFile &sf = MAIN->files[path];
    Tree *result = sf.tree;
    if (phase == EXECUTION_PHASE && result)
        result = context->Evaluate(result);
    return result;
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


Tree *xl_load_data(Context *context, Tree *self,
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

    return xl_load_data(context, self, path,
                        input, true, true,
                        prefix, fieldSeps, recordSeps,
                        body);
}


Tree *xl_load_data(Context *context, Tree *self, text inputName,
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
                    result = context->Call(prefix, row.args);
                }
                return result;
            }
            return perFile.loaded;
        }

        // Restart with clean data
        perFile.data.clear();
        perFile.loaded = NULL;
    }

    // Read data from file
    char     buffer[256];
    char    *ptr       = buffer;
    char    *end       = buffer + sizeof(buffer) - 1;
    Tree_p   tree      = NULL;
    Tree_p   line      = NULL;
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
            Tree *child = NULL;

            if (isdigit(buffer[0]) ||
                ((buffer[0] == '-' || buffer[0] == '+') && isdigit(buffer[1])))
            {
                char *ptr2 = NULL;
                longlong l = strtoll(buffer, &ptr2, 10);
                if (ptr2 == ptr-1)
                {
                    child = new Integer(l);
                }
                else
                {
                    double d = strtod (buffer, &ptr2);
                    if (ptr2 == ptr-1)
                        child = new Real(d);
                }
            }
            if (child == NULL)
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
                    tree = context->Call(prefix, row.args);
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
                    line = NULL;
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

bool xl_write_integer(longlong x)
// ----------------------------------------------------------------------------
//   Write an integer value
// ----------------------------------------------------------------------------
{
    std::cout << x;
    return true;
}


bool xl_write_real(double x)
// ----------------------------------------------------------------------------
//   Write a double value
// ----------------------------------------------------------------------------
{
    std::cout << x;
    return true;
}


bool xl_write_text(kstring x)
// ----------------------------------------------------------------------------
//   Write a text value
// ----------------------------------------------------------------------------
{
    std::cout << x;
    return true;
}


bool xl_write_tree(Tree *tree)
// ----------------------------------------------------------------------------
//   Write a text value
// ----------------------------------------------------------------------------
{
    std::cout << tree;
    return true;
}


bool xl_write_character(char x)
// ----------------------------------------------------------------------------
//   Write a character value
// ----------------------------------------------------------------------------
{
    std::cout << x;
    return true;
}


bool xl_write_cr()
// ----------------------------------------------------------------------------
//   Write a line separator
// ----------------------------------------------------------------------------
{
    std::cout << '\n';
    return true;
}


bool xl_text_eq(kstring x, kstring y)
// ----------------------------------------------------------------------------
//   Compare two texts
// ----------------------------------------------------------------------------
{
    return strcmp(x, y) == 0;
}


bool xl_text_ne(kstring x, kstring y)
// ----------------------------------------------------------------------------
//   Compare two texts
// ----------------------------------------------------------------------------
{
    return strcmp(x, y) != 0;
}


bool xl_text_lt(kstring x, kstring y)
// ----------------------------------------------------------------------------
//   Compare two texts
// ----------------------------------------------------------------------------
{
    return strcmp(x, y) < 0;
}


bool xl_text_le(kstring x, kstring y)
// ----------------------------------------------------------------------------
//   Compare two texts
// ----------------------------------------------------------------------------
{
    return strcmp(x, y) <= 0;
}


bool xl_text_gt(kstring x, kstring y)
// ----------------------------------------------------------------------------
//   Compare two texts
// ----------------------------------------------------------------------------
{
    return strcmp(x, y) > 0;
}


bool xl_text_ge(kstring x, kstring y)
// ----------------------------------------------------------------------------
//   Compare two texts
// ----------------------------------------------------------------------------
{
    return strcmp(x, y) >= 0;
}


integer_t xl_text2int(kstring tval)
// ----------------------------------------------------------------------------
//   Converts text to a numerical value
// ----------------------------------------------------------------------------
{
    text t = tval;
    std::istringstream stream(t);
    integer_t result = 0.0;
    stream >> result;
    return result;
}


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


text xl_int2text(integer_t value)
// ----------------------------------------------------------------------------
//   Convert a numerical value to text
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << value;
    return out.str();
}


text xl_real2text(real_t value)
// ----------------------------------------------------------------------------
//   Convert a numerical value to text
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << value;
    return out.str();
}


integer_t xl_mod(integer_t x, integer_t y)
// ----------------------------------------------------------------------------
//   Compute a mathematical 'mod' from the C99 % operator
// ----------------------------------------------------------------------------
{
    integer_t tmp = x % y;
    if (tmp && (x^y) < 0)
        tmp += y;
    return tmp;
}


integer_t xl_pow(integer_t x, integer_t y)
// ----------------------------------------------------------------------------
//   Compute integer power
// ----------------------------------------------------------------------------
{
    integer_t tmp = 0;
    if (y >= 0)
    {
        tmp = 1;
        while (y)
        {
            if (y & 1)
                tmp *= x;
            x *= x;
            y >>= 1;
        }
    }
    return tmp;
}


real_t xl_modf(real_t x, real_t y)
// ----------------------------------------------------------------------------
//   Compute a mathematical 'mod' from fmod
// ----------------------------------------------------------------------------
{
    real_t tmp = fmod(x,y);
    if (tmp != 0.0 && x*y < 0.0)
        tmp += y;
    return tmp;
}


real_t xl_powf(real_t x, integer_t y)
// ----------------------------------------------------------------------------
//   Compute real power with an integer on the right
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


real_t xl_time(real_t delay)
// ----------------------------------------------------------------------------
//   Return the current system time
// ----------------------------------------------------------------------------
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    MAIN->Refresh(delay);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}


#define XL_TIME(tmfield, delay)               \
    struct tm tm = { 0 };                       \
    time_t clock = { 0 };                       \
    time(&clock);                               \
    localtime_r(&clock, &tm);                   \
    MAIN->Refresh(delay);                       \
    return tm.tmfield


integer_t  xl_seconds()     { XL_TIME(tm_sec,       1.0); }
integer_t  xl_minutes()     { XL_TIME(tm_min,      60.0); }
integer_t  xl_hours()       { XL_TIME(tm_hour,    3600.0); }
integer_t  xl_month_day()   { XL_TIME(tm_mday,   86400.0); }
integer_t  xl_mon()         { XL_TIME(tm_mon,    86400.0); }
integer_t  xl_year()        { XL_TIME(tm_year,   86400.0); }
integer_t  xl_week_day()    { XL_TIME(tm_wday,   86400.0); }
integer_t  xl_year_day()    { XL_TIME(tm_yday,   86400.0); }
integer_t  xl_summer_time() { XL_TIME(tm_isdst,  86400.0); }
#ifndef CONFIG_MINGW
text_t     xl_timezone()    { XL_TIME(tm_zone,   86400.0); }
integer_t  xl_GMT_offset()  { XL_TIME(tm_gmtoff, 86400.0); }
#endif // CONFIG_MINGW


real_t xl_random()
// ----------------------------------------------------------------------------
//    Return a pseudo-random number in the low..high range
// ----------------------------------------------------------------------------
{
#ifndef CONFIG_MINGW
    return drand48();
#else
    return real_t(rand()) / RAND_MAX;
#endif // CONFIG_MINGW
}


bool xl_random_seed(int seed)
// ----------------------------------------------------------------------------
//    Initialized random number generator using the argument passed as seed
// ----------------------------------------------------------------------------
{
#ifndef CONFIG_MINGW
    srand48(seed);
#else
    srand(seed);
#endif // CONFIG_MINGW

    return true;
}

} // extern "C"


XL_END


uint xl_recursion_count = 0;
