// ****************************************************************************
//  runtime.cpp                     (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                                 XL2 project
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
#include "compiler.h"
#include "compiler-gc.h"
#include "main.h"
#include "types.h"
#include "save.h"
#include "tree-clone.h"
#include "utf8_fileutils.h"

#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <errno.h>
#include "winglob.h"
#include <sys/stat.h>


XL_BEGIN


Tree *xl_identity(Context *, Tree *what)
// ----------------------------------------------------------------------------
//   Return the input tree unchanged
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *xl_evaluate(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//   Compile the tree if necessary, then evaluate it
// ----------------------------------------------------------------------------
// This is similar to Context::Run, but we save stack space for recursion
{
    return context->Evaluate(what);
}


Tree *xl_assigned_value(Context *context, Tree *value)
// ----------------------------------------------------------------------------
//   An assigned value is returned as is
// ----------------------------------------------------------------------------
{
    (void) context;
    return value;
}


Tree *xl_named_value(Context *context, Tree *value)
// ----------------------------------------------------------------------------
//   An named value is returned as is
// ----------------------------------------------------------------------------
{
    (void) context;
    return value;
}


struct SourceInfo : Info
// ----------------------------------------------------------------------------
//   Record the source for a given tree
// ----------------------------------------------------------------------------
{
    SourceInfo(Tree *source) : Info(), source(source) {}
    Tree_p source;
};


Tree *xl_source(Tree *value)
// ----------------------------------------------------------------------------
//   Return the source for the given value
// ----------------------------------------------------------------------------
{
    while (SourceInfo *info = value->GetInfo<SourceInfo>())
        value = info->source;
    return value;
}


Tree *xl_set_source(Tree *value, Tree *source)
// ----------------------------------------------------------------------------
//   Set the source associated with the value (e.g. for integer->real casts)
// ----------------------------------------------------------------------------
{
    if (source != value && source != xl_source(value))
    {
        value->Purge<SourceInfo>();
        value->SetInfo<SourceInfo>(new SourceInfo(source));
    }
    return value;
}


Tree *xl_error(Tree *self, text msg, Tree *a1, Tree *a2, Tree *a3)
// ----------------------------------------------------------------------------
//   The default runtime error message mechanism (if not overriden)
// ----------------------------------------------------------------------------
{
    if (a1) a1 = FormatTreeForError(a1);
    if (a2) a2 = FormatTreeForError(a2);
    if (a3) a3 = FormatTreeForError(a3);
    Error err(msg, self->Position());
    if (a1) err.Arg(a1);
    if (a2) err.Arg(a2);
    if (a3) err.Arg(a3);
    MAIN->errors->Log(err);

    return self;
}


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

    return Ooops("No form match $1 at runtime", what);
}


Tree *xl_stack_overflow(Tree *what)
// ----------------------------------------------------------------------------
//   Return an error evaluating a tree
// ----------------------------------------------------------------------------
{
    return Ooops("Stack overflow evaluating $1", what);
}


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
            if (Name *name = result->AsName())
                if (Tree *bound = context->Bound(name))
                    return bound;
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
    Parser parser(input, MAIN->syntax, MAIN->positions, *MAIN->errors);
    return parser.Parse();
}


Tree *xl_bound(Context *context, Tree *form)
// ----------------------------------------------------------------------------
//   Return the bound value for a name/form, or nil if not bound
// ----------------------------------------------------------------------------
{
    // TODO: Equivalent in compiled mode
    if (Tree *bound = context->Bound(form))
        return bound;
    return XL::xl_false;
}


bool xl_same_text(Tree *what, const char *ref)
// ----------------------------------------------------------------------------
//   Compile the tree if necessary, then evaluate it
// ----------------------------------------------------------------------------
{
    Text *tval = what->AsText(); assert(tval);
    return tval->value == text(ref);
}


bool xl_same_shape(Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//   Check equality of two trees
// ----------------------------------------------------------------------------
{
    return Tree::Equal(left, right);
}


Tree *xl_infix_match_check(Context *context, Tree *value, kstring name)
// ----------------------------------------------------------------------------
//   Check if the value is matching the given infix
// ----------------------------------------------------------------------------
{
    // The following test is a hack to detect closures
    while (Block *block = value->AsBlock())
        if (block->opening == "(" && block->closing == ")")
            value = block->child;
        else
            break;
    if (Infix *infix = value->AsInfix())
        if (infix->name == name)
            return infix;
    return NULL;
}


Tree *xl_type_check(Context *context, Tree *value, Tree *type)
// ----------------------------------------------------------------------------
//   Check if value has the type of 'type'
// ----------------------------------------------------------------------------
{
    IFTRACE(typecheck)
        std::cerr << "Type check " << value << " against " << type << ':';

    if (Tree *works = ValueMatchesType(context, value, type, true))
    {
        IFTRACE(typecheck)
            std::cerr << "Success\n";
        return works;
    }

    IFTRACE(typecheck)
        std::cerr << "Failed (mismatch)\n";

    return NULL;
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
    result->code = xl_identity;
    return result;
}


Prefix *xl_new_prefix(Prefix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Prefix
// ----------------------------------------------------------------------------
{
    Prefix *result = new Prefix(source, left, right);
    result->code = xl_identity;
    return result;
}


Postfix *xl_new_postfix(Postfix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Postfix
// ----------------------------------------------------------------------------
{
    Postfix *result = new Postfix(source, left, right);
    result->code = xl_identity;
    return result;
}


Infix *xl_new_infix(Infix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to build a new Infix
// ----------------------------------------------------------------------------
{
    Infix *result = new Infix(source, left, right);
    result->code = xl_identity;
    return result;
}


Block *xl_fill_block(Block *source, Tree *child)
// ----------------------------------------------------------------------------
//    Called by generated code to replace child of a Block
// ----------------------------------------------------------------------------
{
    if (child)  source->child = child;
    return source;
}


Prefix *xl_fill_prefix(Prefix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to replace children of a Prefix
// ----------------------------------------------------------------------------
{
    if (left)   source->left = left;
    if (right)  source->right = right;
    return source;
}


Postfix *xl_fill_postfix(Postfix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to replace children of a Postfix
// ----------------------------------------------------------------------------
{
    if (left)   source->left = left;
    if (right)  source->right = right;
    return source;
}


Infix *xl_fill_infix(Infix *source, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Called by generated code to replace children of an Infix
// ----------------------------------------------------------------------------
{
    if (left)   source->left = left;
    if (right)  source->right = right;
    return source;
}


Tree *xl_real_list(Tree *self, uint n, double *values)
// ----------------------------------------------------------------------------
//   Build an infix from a list of real numbers
// ----------------------------------------------------------------------------
{
    Tree *result = NULL;
    while (n)
    {
        Tree *left = new Real(values[--n], self->Position());
        if (result)
            result = new Infix(",", left, result, self->Position());
        else
            result = left;
        result->code = xl_identity;
    }
    return result;
}


Tree *xl_integer_list(Tree *self, uint n, longlong *values)
// ----------------------------------------------------------------------------
//   Build an infix from a list of real numbers
// ----------------------------------------------------------------------------
{
    Tree *result = NULL;
    while (n)
    {
        Tree *left = new Integer(values[--n], self->Position());
        if (result)
            result = new Infix(",", left, result, self->Position());
        else
            result = left;
        result->code = xl_identity;
    }
    return result;
}



// ============================================================================
//
//    Closure management
//
// ============================================================================

Tree *xl_tree_copy(Tree *from, Tree *to)
// ----------------------------------------------------------------------------
//    Copy information from one tree to the other
// ----------------------------------------------------------------------------
{
    if (from && to && from != to)
    {
#if 0
        if (CompilerInfo *cinfo = from->Remove<CompilerInfo>())
            to->SetInfo<CompilerInfo> (cinfo);
        if (eval_fn code = from->code)
        {
            to->code = code;
            from->code = NULL;
        }
#endif
    }
    return to;
}



// ============================================================================
//
//    Type matching
//
// ============================================================================

#pragma GCC diagnostic ignored "-Wunused-parameter"

Tree *xl_boolean_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a boolean value (true/false)
// ----------------------------------------------------------------------------
{
    value = context->Evaluate(value);
    if (value == xl_true || value == xl_false)
        return value;
    return NULL;
}


Tree *xl_integer_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an integer
// ----------------------------------------------------------------------------
{
    value = context->Evaluate(value);
    if (Integer *it = value->AsInteger())
        return it;
    return NULL;
}


Tree *xl_real_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a real
// ----------------------------------------------------------------------------
{
    value = context->Evaluate(value);
    if (Real *rt = value->AsReal())
        return rt;
    if (Integer *it = value->AsInteger())
        return new Real(it->value);
    return NULL;
}


Tree *xl_text_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a text
// ----------------------------------------------------------------------------
{
    value = context->Evaluate(value);
    if (Text *tt = value->AsText())
        return tt;
    return NULL;
}


Tree *xl_character_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a character
// ----------------------------------------------------------------------------
{
    value = context->Evaluate(value);
    if (Text *tt = value->AsText())
        if (tt->opening == "'")
            return tt;
    return NULL;
}


Tree *xl_tree_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Any input tree can be accepted
// ----------------------------------------------------------------------------
{
    return value;
}


Tree *xl_source_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Any source can be accepted
// ----------------------------------------------------------------------------
{
    return value;
}


Tree *xl_code_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Any code can be accepted
// ----------------------------------------------------------------------------
{
    return value;
}


Tree *xl_lazy_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Any lazy argument can be accepted
// ----------------------------------------------------------------------------
{
    return value;
}


Tree *xl_reference_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Any reference argument can be accepted
// ----------------------------------------------------------------------------
{
    return value;
}


Tree *xl_value_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Any value can be accepted (we evaluate first)
// ----------------------------------------------------------------------------
{
    return value;
}


Tree *xl_symbol_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a symbol
// ----------------------------------------------------------------------------
{
    if (Name *nt = value->AsName())
        return nt;
    value = context->Evaluate(value);
    if (Name *afterEval = value->AsName())
        return afterEval;
    return NULL;
}


Tree *xl_name_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a name
// ----------------------------------------------------------------------------
{
    if (Name *nt = value->AsName())
        if (nt->value.length() && isalpha(nt->value[0]))
            return nt;
    value = context->Evaluate(value);
    if (Name *afterEval = value->AsName())
        if (afterEval->value.length() && isalpha(afterEval->value[0]))
            return afterEval;
    return NULL;
}


Tree *xl_operator_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an operator symbol
// ----------------------------------------------------------------------------
{
    if (Name *nt = value->AsName())
        if (nt->value.length() && !isalpha(nt->value[0]))
            return nt;
    value = context->Evaluate(value);
    if (Name *afterEval = value->AsName())
        if (afterEval->value.length() && !isalpha(afterEval->value[0]))
            return afterEval;
    return NULL;
}


Tree *xl_infix_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an infix
// ----------------------------------------------------------------------------
{
    if (Infix *it = value->AsInfix())
        return it;
    return NULL;
}


Tree *xl_prefix_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a prefix
// ----------------------------------------------------------------------------
{
    if (Prefix *it = value->AsPrefix())
        return it;
    return NULL;
}


Tree *xl_postfix_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a postfix
// ----------------------------------------------------------------------------
{
    if (Postfix *it = value->AsPostfix())
        return it;
    return NULL;
}


Tree *xl_block_cast(Context *context, Tree *source, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a block
// ----------------------------------------------------------------------------
{
    if (Block *it = value->AsBlock())
        return it;
    return NULL;
}


Real *xl_integer2real(Integer *iv)
// ----------------------------------------------------------------------------
//   Promote integer to real
// ----------------------------------------------------------------------------
{
    return new Real(iv->value, iv->Position());
}



// ============================================================================
//
//   Animation utilities
//
// ============================================================================

Tree *xl_parameter(text symbol, text type)
// ----------------------------------------------------------------------------
//   Build an infix name:type, except if type is "lazy"
// ----------------------------------------------------------------------------
{
    Name *n = new Name(symbol);
    if (type == lazy_type->value)
        return n;
    Name *t = new Name(type);
    Infix *i = new Infix(":", n, t);
    return i;
}


void xl_infix_to_list(Infix *infix, TreeList &list)
// ----------------------------------------------------------------------------
//   Convert an infix to a list, whether left or right associative
// ----------------------------------------------------------------------------
{
    Infix *left = infix->left->AsInfix();
    if (left && left->name == infix->name)
        xl_infix_to_list(left, list);
    else
        list.push_back(infix->left);

    Infix *right = infix->right->AsInfix();
    if (right && right->name == infix->name)
        xl_infix_to_list(right, list);
    else
        list.push_back(infix->right);
}


Tree *xl_list_to_tree(TreeList v, text infix, Infix ** deepest)
// ----------------------------------------------------------------------------
//   Builds a tree from a list of tree with the given infix.
// ----------------------------------------------------------------------------
// The deepest infix carries the 2 last elements of TreeList. If latest is
// provided then the deepest infix is returned and its right leg carries xl_nil.
{
    if (v.size() <= 0) return XL::xl_nil;
    if (deepest)
    {
        *deepest = NULL;
        v.push_back(XL::xl_nil);
    }
    if (v.size() == 1) return v[0];

    Tree* result = NULL;
    TreeList::reverse_iterator rit;
    // First build the bottom of the tree
    rit = v.rbegin();
    result = *rit;
    rit++;
    XL::Infix * resInf = NULL;
    // Build the common part
    for (; rit < v.rend(); ++rit)
    {
        result = resInf = new XL::Infix(infix, *rit, result);
        // Assign only the first time to remember the bottomest Infix
        if (deepest && *deepest == NULL)
            *deepest = resInf;
    }
    return result;
}


Real *xl_springify(Real &value, Real &target, Real &time,
                   Real &damp, Real &kspring, Real &lt, Real &ls)
// ----------------------------------------------------------------------------
//   Add a "spring" effect to a value
// ----------------------------------------------------------------------------
{
    double distance = target.value - value.value;
    double t = time.value;
    double interval = t - lt.value;
    if (interval > 1.0)
        interval = 1.0;
    double v = value.value + ls.value * interval;
    double accel = kspring * distance;
    ls.value *= (1.0 - interval * damp.value);
    ls.value += accel * interval;
    lt.value = t;
    value.value = v;

    return &value;
}



// ============================================================================
//
//   Adding a builtin or a global
//
// ============================================================================

void xl_enter_builtin(Main *main, text name, Tree *from, Tree *to, eval_fn code)
// ----------------------------------------------------------------------------
//   Add a new builtin
// ----------------------------------------------------------------------------
{
    main->compiler->EnterBuiltin(name, from, to, code);
}


void xl_enter_global(Main *main, Name *name, Name_p *address)
// ----------------------------------------------------------------------------
//   Add a new global
// ----------------------------------------------------------------------------
{
    main->compiler->EnterGlobal(name, address);
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

#if defined(CONFIG_MINGW)
    struct _stat st;
#else
    struct stat st;
#endif

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
#if defined(CONFIG_MINGW)
                    struct _stat st;
#else
                    struct stat st;
#endif
                    if (utf8_stat (path.c_str(), &st) < 0)
                        path = "";
                }
            }
        }
        if (path == "")
            return Ooops("Source file $1 not found", new Text(name));
        info = new ImportedFileInfo(path);
        self->SetInfo<ImportedFileInfo> (info);
    }

    // Check if the file has already been loaded somehwere.
    // If so, return the loaded file
    bool exists = MAIN->files.count(path);
    if (!exists)
    {
        IFTRACE(fileload)
                std::cout << "Loading: " << path << "\n";
        bool hadError = MAIN->LoadFile(path, false, context);
        if (hadError)
            return Ooops("Unable to load file $1", new Text(path));
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
                   text name, text prefix, text fieldSeps, text recordSeps)
// ----------------------------------------------------------------------------
//    Load a comma-separated or tab-separated file from disk
// ----------------------------------------------------------------------------
{
    // Open data file
    text path = MAIN->SearchFile(name);
    if (path == "")
        return Ooops("CSV file $1 not found", new Text(name));

    utf8_ifstream input(path.c_str(), std::ifstream::in);
    if (!input.good())
        return Ooops("Unable to load data for $1.\n"
                     "(Accessing $2 resulted in the following error: $3)",
                     self,
                     new Text(path, "\"", "\"", self->Position()),
                     new Text(strerror(errno), "", "", self->Position()));

    return xl_load_data(context, self, path,
                        input, true, true,
                        prefix, fieldSeps, recordSeps);
}


Tree *xl_load_data(Context *context, Tree *self, text inputName,
                   std::istream &input, bool cached, bool statTime,
                   text prefix, text fieldSeps, text recordSeps)
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
#if defined(CONFIG_MINGW)
            struct _stat st;
#else
            struct stat st;
#endif
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
                    perFile.data.push_back(row);
                    tree = context->Call(prefix, row.args);
                    row.args.clear();
                }
                else
                {
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
//   Managing calls to/from XL
//
// ============================================================================

Tree *XLCall::operator() (SourceFile *sf)
// ----------------------------------------------------------------------------
//   Invoke the call in the context of a given source file
// ----------------------------------------------------------------------------
{
    Context *context = sf->context;
    Tree *call = name;
    if (arguments)
        call = new Prefix(call, arguments);
    return context->Evaluate(call);
}


Tree *XLCall::operator() (Context *context)
// ----------------------------------------------------------------------------
//    Perform the given call in the given context
// ----------------------------------------------------------------------------
{
    Tree *callee = context->Call(name->value, args);
    return callee;
}


Tree *XLCall::build(Context *context)
// ----------------------------------------------------------------------------
//    Perform the given call in the given context
// ----------------------------------------------------------------------------
{
    Tree *callee = context->Call(name->value, args);
    return callee;
}



// ============================================================================
//
//    Interfaces to make old and new compiler compatible (temporary)
//
// ============================================================================

Tree *xl_define(Context *context, Tree *self, Tree *form, Tree *definition)
// ----------------------------------------------------------------------------
//    Define a form in interpreted mode
// ----------------------------------------------------------------------------
{
    context->Define(form, definition);
    return self;
}


Tree *xl_evaluate_sequence(Context *context, Tree *first, Tree *second)
// ----------------------------------------------------------------------------
//   Evaluate a sequence
// ----------------------------------------------------------------------------
{
    context->Evaluate(first);
    return context->Evaluate(second);
}



// ============================================================================
//
//   Apply a code recursively to a data set (temporary / obsolete)
//
// ============================================================================

Tree *xl_range(longlong low, longlong high)
// ----------------------------------------------------------------------------
//   Return a range of values between low and high
// ----------------------------------------------------------------------------
//   This is so ugly, but lazy evalation doesn't work quite right yet
{
    Tree *result = new Integer(high);
    for (longlong i = high-1; i >= low; i--)
        result = new Infix(",", new Integer(i), result);
    result->code = xl_identity;
    return result;
}


Tree *xl_apply(Context *context, Tree *code, Tree *data)
// ----------------------------------------------------------------------------
//   Apply the input code on each piece of data
// ----------------------------------------------------------------------------
//   We deal with the following cases:
//   - Code is a name: We map it as a prefix to a single-argument function
//   - Code is in the form X->f(X): We map the right-hand side
//   - Code is in the form X,Y->f(X,Y): We reduce using the right-hand side
//   - Code is in the form X where f(X): We filter based on f(X)
{
    Tree *result = data;
#if 0
    // Check if we already compiled that code
    FunctionInfo *fninfo = code->GetInfo<FunctionInfo>();
    if (!fninfo)
    {
        // Identify what operation we want to perform
        Tree *toCompile = code;
        TreeList parameters;
        bool reduce = false;
        bool filter = false;

        // For syntactic convenience, the code is generally in a block
        if (Block *codeBlock = toCompile->AsBlock())
        {
            toCompile = codeBlock->child;
        }

        // Check the case where code is x->sin x  (map) or x,y->x+y (reduce)
        if (Infix *infix = toCompile->AsInfix())
        {
            Tree *ileft = infix->left;
            if (infix->name == "->")
            {
                // Case of x -> sin x
                if (Name *name = ileft->AsName())
                {
                    parameters.push_back(name);
                    toCompile = infix->right;
                }

                // Case of x,y -> x+y
                else if (Infix *op = ileft->AsInfix())
                {
                    Name *first = op->left->AsName();
                    Name *second = op->right->AsName();
                    if (first && second)
                    {
                        parameters.push_back(first);
                        parameters.push_back(second);
                        reduce = true;
                        toCompile = infix->right;
                    }
                }
            }
            else if (infix->name == "where")
            {
                // Case of x where x < 3
                if (Name *name = ileft->AsName())
                {
                    parameters.push_back(name);
                    toCompile = infix->right;
                    filter = true;
                }
            }
        }
        else if (Name *name = toCompile->AsName())
        {
            // We have a single name: consider it as a prefix to all elements
            Name *parameter = new Name("_");
            parameters.push_back(parameter);
            toCompile = new Prefix(name, parameter);
        }
        else
        {
            // OK, we don't know what to do with this stuff...
            return Ooops("Malformed map/reduce code $1", toCompile);
        }

        // We have now decided what this is, so we compile the code
        eval_fn fn = NULL;

        // Record all the parameters in the symbol table
        for (TreeList::iterator p=parameters.begin(); p!=parameters.end(); p++)
            if (Name *parmName = (*p)->AsName())
                symbols->Allocate(parmName);

        // Create a compile unit with the right number of parameters
        Compiler *compiler = MAIN->compiler;
        OCompiledUnit unit(compiler, toCompile, parameters, false);
        assert (!unit.IsForwardCall() || !"Forward call in map/reduce code");

        // Record internal declarations if any
        DeclarationAction declare(symbols);
        Tree *toDecl = toCompile->Do(declare);
        assert(toDecl); (void) toDecl;

        // Compile the body we generated
        CompileAction compile(symbols, unit, true, true, true);
        Tree *compiled = toCompile->Do(compile);

        // Generate code if compilation was successful
        if (compiled)
            fn = unit.Finalize();

        // Generate appropriate function info
        if (filter)
            fninfo = new FilterFunctionInfo;
        else if (reduce)
            fninfo = new ReduceFunctionInfo;
        else
            fninfo = new MapFunctionInfo;

        // Record generated code (or NULL in case of compilation failure)
        code->SetInfo<FunctionInfo>(fninfo);
        fninfo->function = fn;
        fninfo->context = context;
        fninfo->symbols = symbols;
        fninfo->compiled = toCompile;

        // Report compile error the first time
        if (!compiled)
            return Ooops("Cannot compile map/reduce code $1", toCompile);
    }

    if (fninfo->function)
        result = fninfo->Apply(result);
    else
        result = Ooops("Invalid map/reduce code $1", code);
#endif // 0
    return result;
}



// ============================================================================
//
//  Iterator on lists of items
//
// ============================================================================

ListIterator::ListIterator(Context *context, Tree *what)
// ----------------------------------------------------------------------------
//    Initialize the list iterator
// ----------------------------------------------------------------------------
    : context(context), data(what), separator(),
      startI(0), endI(0), stepI(0),
      startF(0), endF(0), stepF(0)
{
    if (Block *block = data->AsBlock())
    {
        // Check if we got () or (1,2,3). If so, extract child
        data = block->child;
    }

    // Check the empty list case
    if (Name *name = data->AsName())
        if (name->value == "")
            data = NULL;
}


static Tree *make(longlong i) { return new Integer(i); }
static Tree *make(double r)   { return new Real(r); }


template<class num>
Tree *ListIterator::Next(num &start, num &end, num &step)
// ----------------------------------------------------------------------------
//    Optimize special numeric operators
// ----------------------------------------------------------------------------
{
    if (step)
    {
        if (step > 0 && start <= end)
        {
            Tree *result = make(start);
            start += step;
            return result;
        }
        else if (step < 0 && start >= end)
        {
            Tree *result = make(start);
            start += step;
            return result;
        }
        step = 0;
    }
    return NULL;
}


Tree *ListIterator::EvaluateRange(Tree *input)
// ----------------------------------------------------------------------------
//   Check ranges like 1..5, otherwise evaluate input
// ----------------------------------------------------------------------------
{
    if (Infix *infix = input->AsInfix())
    {
        text sep = infix->name;

        // Check stepped numerical ranges
        if (sep == "by")
        {
            Tree_p left = infix->left;
            if (Infix *inner = left->AsInfix())
            {
                if (inner->name == "..")
                {
                    sep = inner->name;

                    Tree_p right = infix->right;
                    Integer *si = right->AsInteger();
                    Real *sr = right->AsReal();
                    if (!si && !sr)
                    {
                        right = context->Evaluate(right);
                        si = right->AsInteger();
                        sr = right->AsReal();
                    }
                    infix = inner;
                    if (si)
                        stepI = si->value;
                    else if (sr)
                        stepF = sr->value;
                    else
                        sep = "";
                }
            }
        }

        // Check numerical ranges
        if (sep == "..")
        {
            Tree_p left = infix->left;
            Tree_p right = infix->right;
            Integer *li = left->AsInteger();
            Integer *ri = right->AsInteger();
            Real *lr = left->AsReal();
            Real *rr = right->AsReal();
            if (!li && !lr)
            {
                left = context->Evaluate(left);
                li = left->AsInteger();
                lr = left->AsReal();
            }
            if (!ri && !rr)
            {
                right = context->Evaluate(right);
                ri = right->AsInteger();
                rr = right->AsReal();
            }
            if (li && ri)
            {
                startI = li->value;
                endI = ri->value;
                if (!stepI)
                    stepI = 1;
                if (separator == "")
                    separator = ",";
                return Next(startI, endI, stepI);
            }
            if (lr && rr)
            {
                startF = lr->value;
                endF = rr->value;
                if (!stepF)
                    stepF = 1.0;
                if (separator == "")
                    separator = ",";
                return Next(startF, endF, stepF);
            }
        }
    }

    // Other cases: evaluate
    input = context->Evaluate(input);
    return input;
}


Tree * ListIterator::Next()
// ----------------------------------------------------------------------------
//   Compute the next element in a list
// ----------------------------------------------------------------------------
{
    // Check if we are in an integer or float iterator
    if (Tree *ival = Next(startI, endI, stepI))
        return ival;
    if (Tree *rval = Next(startF, endF, stepF))
        return rval;

    // Check empty list
    if (!data)
        return NULL;

    if (Infix *infix = data->AsInfix())
    {
        text sep = infix->name;

        // Check which separator we may have in the list
        if (separator == "" && (sep == "," || sep == ";" || sep == "\n"))
            separator = sep;

        // Check other cases
        if (separator == sep)
        {
            Tree *result = infix->left;
            data = infix->right;
            result = EvaluateRange(result);
            if (!result)
                result = Next();
            return result;
        }
    }

    // Last item in a list: evaluate and return
    Tree *result = data;
    data = NULL;
    result = EvaluateRange(result);
    if (separator == "" && result->Kind() == INFIX)
    {
        data = result;
        result = Next();
    }
    return result;
}



// ============================================================================
//
//   Map an operation on all elements
//
// ============================================================================

typedef Tree * (*map_fn) (Context *context, Tree *self, Tree *arg);

Tree *MapFunctionInfo::Apply(Tree *what)
// ----------------------------------------------------------------------------
//   Apply a map operation
// ----------------------------------------------------------------------------
{
    ListIterator li(context, what);
    map_fn map = (map_fn) function;
    Tree_p result = NULL;
    Tree_p *parent = &result;

    // Loop on all elements
    while (Tree *next = li.Next())
    {
        next = map(context, next, next);
        if (*parent != NULL)
        {
            Infix *newList = new Infix(li.separator,
                                       *parent, next,
                                       next->Position());
            *parent = newList;
            parent = &newList->right;
            newList->code = xl_identity;
        }
        else
        {
            *parent = next;
        }
    }

    // Return built list
    if (!result)
        return what;
    return result;
}



// ============================================================================
//
//   Reduce by applying operations to consecutive elements
//
// ============================================================================

typedef Tree * (*reduce_fn) (Context *, Tree *self, Tree *t1,Tree *t2);
Tree *ReduceFunctionInfo::Apply(Tree *what)
// ----------------------------------------------------------------------------
//   Apply a reduce operation to the tree
// ----------------------------------------------------------------------------
{
    ListIterator li(context, what);
    reduce_fn reduce = (reduce_fn) function;
    Tree_p result = NULL;

    // Loop on all elements
    while (Tree *next = li.Next())
    {
        if (result)
            result = reduce(context, next, result, next);
        else
            result = next;
    }

    // Return built list
    if (!result)
        result = xl_nil;
    return result;
}



// ============================================================================
//
//   Filter by selecting elements that match a given condition
//
// ============================================================================

typedef Tree * (*filter_fn) (Context *, Tree *self, Tree *arg);
Tree *FilterFunctionInfo::Apply(Tree *what)
// ----------------------------------------------------------------------------
//   Apply a filter operation to the tree
// ----------------------------------------------------------------------------
{
    ListIterator li(context, what);
    filter_fn filter = (map_fn) function;
    Tree_p result = NULL;
    Tree_p *parent = &result;

    // Loop on all elements
    while (Tree *next = li.Next())
    {
        Tree *test = filter(context, next, next);
        if (test == xl_true)
        {
            if (*parent != NULL)
            {
                Infix *newList = new Infix(li.separator,
                                           *parent, next,
                                           next->Position());
                *parent = newList;
                parent = &newList->right;
            }
            else
            {
                *parent = next;
            }
        }
    }

    // Return built list
    if (!result)
        return xl_false;

    return result;
}



// ============================================================================
//
//   References, indexing and assignment
//
// ============================================================================

Tree *xl_assign(Context *context, Tree *var, Tree *value, Tree *type)
// ----------------------------------------------------------------------------
//   Assignment in interpreted mode
// ----------------------------------------------------------------------------
{
    IFTRACE(references)
        std::cerr << "Assigning " << var << ":=" << value << "\n";

    if (type && type != tree_type)
        var = new Infix("as", var, type, var->Position());

    return context->Assign(var, value);
}


Tree *xl_index(Context *context, Tree *data, Tree *indexTree)
// ----------------------------------------------------------------------------
//   Find the given element in a data set or return false, e.g. X.Y
// ----------------------------------------------------------------------------
{
    IFTRACE(references)
        std::cerr << "Index " << data << "[" << indexTree << "]\n";

    data = context->Evaluate(data);
    Infix_p syms = new Infix(";", data, xl_nil, data->Position());
    Context_p scope = new Context(syms);
    return context->Bound(indexTree);
}


Tree *xl_array_index(Context *context, Tree *data, Tree *indexTree)
// ----------------------------------------------------------------------------
//   Evaluate things like A[B]
// ----------------------------------------------------------------------------
{
    indexTree = context->Evaluate(indexTree);
    return xl_index(context, data, indexTree);
}


Integer *xl_size(Context *, Tree *data)
// ----------------------------------------------------------------------------
//   Return the size of a data list
// ----------------------------------------------------------------------------
{
    ulong count = 0;
    TreePosition pos = data->Position();
    if (data != xl_nil)
    {
        count = 1;
        Infix *infix = data->AsInfix();
        if (infix)
        {
            text name = infix->name;
            if (name == "\n" || name == ";" || name == ",")
            {
                while (infix && infix->name == name)
                {
                    count++;
                    infix = infix->right->AsInfix();
                }
            }
        }
    }
    return new Integer(count, pos);
}



// ============================================================================
//
//   File search path
//
// ============================================================================

Tree *xl_add_search_path(Context *context, text prefix, text dir)
// ----------------------------------------------------------------------------
//   Add directory to the search path for prefix for the current context
// ----------------------------------------------------------------------------
{ 
    assert(!"Too cassed");
   return XL::xl_true;
}


Text *xl_find_in_search_path(Context *context, text prefix, text file)
// ----------------------------------------------------------------------------
//   Add directory to the search path for prefix for the current context
// ----------------------------------------------------------------------------
{
    return new Text(file);
}


void xl_enter_declarator(Context *context, text name, decl_fn fn)
// ----------------------------------------------------------------------------
//   Enter a declarator in the given context
// ----------------------------------------------------------------------------
{
    context->EnterDeclarator(name, (eval_fn) fn);
}


Name *xl_set_override_priority(Context *context, Tree *self, float priority)
// ----------------------------------------------------------------------------
//   Set the override level for the current symbol table
// ----------------------------------------------------------------------------
{
    context->SetOverridePriority(priority);
    return xl_false;
}



// ============================================================================
//
//    Loops
//
// ============================================================================

Tree *xl_integer_for_loop(Context *context, Tree *self,
                          Tree *Variable,
                          longlong low, longlong high, longlong step,
                          Tree *body)
// ----------------------------------------------------------------------------
//    Loop over an integer variable
// ----------------------------------------------------------------------------
{
    Tree_p result = xl_false;
    Integer_p ival = new Integer(low, self->Position());
    if (step >= 0)
    {
        for (longlong i = low; i <= high; i += step)
        {
            ival->value = i;
            xl_assign(context, Variable, ival, integer_type);
            result = context->Evaluate(body);
        }
    }
    else
    {
        for (longlong i = low; i >= high; i += step)
        {
            ival->value = i;
            xl_assign(context, Variable, ival, integer_type);
            result = context->Evaluate(body);
        }
    }
    return result;
}


Tree *xl_real_for_loop(Context *context, Tree *self,
                       Tree *Variable,
                       double low, double high, double step, Tree *body)
// ----------------------------------------------------------------------------
//    Loop over a real variable
// ----------------------------------------------------------------------------
{
    Tree_p result = xl_false;
    Real_p rval = new Real(low, self->Position());
    if (step >= 0)
    {
        for (double i = low; i <= high; i += step)
        {
            rval->value = i;
            xl_assign(context, Variable, rval, real_type);
            result = context->Evaluate(body);
        }
    }
    else
    {
        for (double i = low; i >= high; i += step)
        {
            rval->value = i;
            xl_assign(context, Variable, rval, real_type);
            result = context->Evaluate(body);
        }
    }
    return result;
}


Tree *xl_list_for_loop(Context *context, Tree *self,
                       Tree *Variable,
                       Tree *list, Tree *body)
// ----------------------------------------------------------------------------
//   Loop over a list
// ----------------------------------------------------------------------------
{
    // Extract list value
    Tree_p result = xl_false;
    if (Block *block = list->AsBlock())
        list = block->child;
    if (Name *name = list->AsName())
        if (name->value == "")
            return result;      // empty list

    // Loop on list items
    Tree *next = list;
    while (next)
    {
        Tree *value = next;
        Infix *infix = value->AsInfix();
        if (infix && (infix->name == "," ||
                      infix->name == ";" ||
                      infix->name == "\n"))
        {
            value = infix->left;
            next = infix->right;
        }
        else
        {
            next = NULL;
        }

        // Set the loop index
        xl_assign(context, Variable, value);
        result = context->Evaluate(body);
    }

    return result;
}


Tree *xl_while_loop(Context *context, Tree *self,
                    Tree *condition, Tree *body, bool TestValue)
// ----------------------------------------------------------------------------
//   Conditional loop (while and until)
// ----------------------------------------------------------------------------
{
    Tree_p result = xl_false;
    Tree_p test = TestValue ? xl_true : xl_false;
    while (true)
    {
        Tree_p value = context->Evaluate(condition);
        if (value != test)
            break;
        result = context->Evaluate(body);
    }
    return result;
}



// ============================================================================
// 
//    C functions defined in builtins.xl
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

} // extern "C"

XL_END


uint xl_recursion_count = 0;
