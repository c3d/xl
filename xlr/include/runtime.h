#ifndef RUNTIME_H
#define RUNTIME_H
// ****************************************************************************
//  runtime.h                       (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//     Functions required for proper run-time execution of XL programs
//
//
//
//
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "base.h"
#include "tree.h"
#include <set>


XL_BEGIN
// ============================================================================
//
//    Forward declarations
//
// ============================================================================

struct Tree;
struct Block;
struct Infix;
struct Prefix;
struct Postfix;
struct Main;



// ============================================================================
//
//    Runtime functions with C interface
//
// ============================================================================

Tree *xl_identity(Tree *);
Tree *xl_error(text msg, Tree *a1=0, Tree *a2=0, Tree *a3=0);
Tree *xl_evaluate(Tree *);
Tree *xl_repeat(Tree *self, Tree *code, longlong count);
Tree *xl_source(Tree *);
Tree *xl_set_source(Tree *val, Tree *src);
bool  xl_same_text(Tree * , const char *);
bool  xl_same_shape(Tree *t1, Tree *t2);
Tree *xl_infix_match_check(Tree *value, kstring name);
Tree *xl_type_check(Tree *value, Tree *type);

Tree *xl_new_integer(longlong value);
Tree *xl_new_real(double value);
Tree *xl_new_character(kstring value);
Tree *xl_new_text(kstring value);
Tree *xl_new_xtext(kstring value, kstring open, kstring close);
Tree *xl_new_block(Block *source, Tree *child);
Tree *xl_new_prefix(Prefix *source, Tree *left, Tree *right);
Tree *xl_new_postfix(Postfix *source, Tree *left, Tree *right);
Tree *xl_new_infix(Infix *source, Tree *left, Tree *right);

Tree *xl_new_closure(Tree *expr, uint ntrees, ...);
Tree *xl_type_error(Tree *tree);
Tree *xl_evaluate_children(Tree *tree);

Tree *xl_boolean_cast(Tree *source, Tree *value);
Tree *xl_integer_cast(Tree *source, Tree *value);
Tree *xl_real_cast(Tree *source, Tree *value);
Tree *xl_text_cast(Tree *source, Tree *value);
Tree *xl_character_cast(Tree *source, Tree *value);
Tree *xl_tree_cast(Tree *source, Tree *value);
Tree *xl_symbol_cast(Tree *source, Tree *value);
Tree *xl_name_symbol_cast(Tree *source, Tree *value);
Tree *xl_operator_symbol_cast(Tree *source, Tree *value);
Tree *xl_infix_cast(Tree *source, Tree *value);
Tree *xl_prefix_cast(Tree *source, Tree *value);
Tree *xl_postfix_cast(Tree *source, Tree *value);
Tree *xl_block_cast(Tree *source, Tree *value);

Real *xl_springify(Real &value, Real &target, Real &time,
                   Real &damp, Real &kspring, Real &lt, Real &ls);

void xl_enter_builtin(Main *main, text name, Tree *to, TreeList parms,
                      eval_fn code);
void xl_enter_global(Main *main, Name *name, Name_p *address);


// ============================================================================
//
//    Call management
//
// ============================================================================

Tree *xl_invoke(eval_fn toCall, Tree *source, TreeList &args);

struct XLCall
// ----------------------------------------------------------------------------
//    A structure that encapsulates a call to an XL tree
// ----------------------------------------------------------------------------
{
    XLCall(text name): name(name), args() {}

    // Adding arguments
    XLCall &operator, (Tree *tree) { args.push_back(tree); return *this; }
    XLCall &operator, (Tree &tree) { return *this, &tree; }
    XLCall &operator, (longlong v) { return *this, new Integer(v); }
    XLCall &operator, (double  v)  { return *this, new Real(v); }
    XLCall &operator, (text  v)    { return *this, new Text(v); }

    // Calling in a given symbol context
    Tree *  operator() (Symbols *syms = NULL,
                       bool nullIfBad = false, bool cached = true);
    Tree *  build(Symbols *syms = NULL);

public:
    text        name;
    TreeList    args;
};



// ============================================================================
// 
//    Actions used for functional applications
// 
// ============================================================================

Tree *xl_apply(Tree *code, Tree *data);
Tree *xl_range(longlong l, longlong h);
void xl_infix_to_list(Infix *infix, TreeList &list);
Tree *xl_nth(Tree *data, longlong index);


struct MapAction : Action
// ----------------------------------------------------------------------------
//   Map a given operation onto each element in a data set
// ----------------------------------------------------------------------------
{
    typedef Tree * (*map_fn) (Tree *self, Tree *arg);

public:
    MapAction(eval_fn function, std::set<text> &sep)
        : function((map_fn) function), separators(sep) {}

    virtual Tree *Do(Tree *what);

    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoBlock(Block *what);

public:
    map_fn              function;
    std::set<text>      separators;
};


struct ReduceAction : Action
// ----------------------------------------------------------------------------
//   Reduce a given operation by combining successive elements
// ----------------------------------------------------------------------------
{
    typedef Tree * (*reduce_fn) (Tree *self, Tree *first, Tree *second);

public:
    ReduceAction(eval_fn function, std::set<text> &sep)
        : function((reduce_fn) function), separators(sep) {}

    virtual Tree *Do(Tree *what);

    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoBlock(Block *what);

public:
    reduce_fn           function;
    std::set<text>      separators;
};


struct FilterAction : Action
// ----------------------------------------------------------------------------
//   Filter a given operation onto each element in a data set
// ----------------------------------------------------------------------------
{
    typedef Tree * (*filter_fn) (Tree *self, Tree *arg);

public:
    FilterAction(eval_fn function, std::set<text> &sep)
        : function((filter_fn) function), separators(sep) {}

    virtual Tree *Do(Tree *what);

    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoBlock(Block *what);

public:
    filter_fn           function;
    std::set<text>      separators;
};


struct FunctionInfo : Info
// ----------------------------------------------------------------------------
//   Hold a single-argument function for a given tree
// ----------------------------------------------------------------------------
//   REVISIT: According to Wikipedia, really a Moses Schönfinkel function
{
    FunctionInfo(): function(NULL), symbols(NULL) {}
    
    virtual Tree * Apply(Tree *what) { return what; }

public:
    eval_fn        function;
    Symbols_p      symbols;
    Tree_p         compiled;
    std::set<text> separators;
};


struct MapFunctionInfo : FunctionInfo
// ----------------------------------------------------------------------------
//   Record the code for a map operation
// ----------------------------------------------------------------------------
{
    virtual Tree * Apply(Tree *what);
};


struct ReduceFunctionInfo : FunctionInfo
// ----------------------------------------------------------------------------
//   Record the code for a reduce operation
// ----------------------------------------------------------------------------
{
    virtual Tree * Apply(Tree *what);
};


struct FilterFunctionInfo : FunctionInfo
// ----------------------------------------------------------------------------
//   Record the code for a filter operation
// ----------------------------------------------------------------------------
{
    virtual Tree * Apply(Tree *what);
};



// ============================================================================
// 
//   Stack depth management
// 
// ============================================================================

struct StackDepthCheck
// ----------------------------------------------------------------------------
//   Verify that we don't go too deep into the stack
// ----------------------------------------------------------------------------
{
    StackDepthCheck(Tree *what)
    {
        stack_depth++;
        if (stack_depth > max_stack_depth)
            StackOverflow(what);
    }
    ~StackDepthCheck()
    {
        stack_depth--;
        if (stack_depth == 0 && !in_error_handler)
            in_error = false;
    }
    operator bool()
    {
        return in_error && !in_error_handler;
    }
    void StackOverflow(Tree *what);

protected:
    static uint         stack_depth;
    static uint         max_stack_depth;
    static bool         in_error_handler;
    static bool         in_error;
};



// ============================================================================
// 
//    Basic text I/O (temporary)
// 
// ============================================================================

Tree *xl_write(Symbols *symbols, Tree *what, text sep = "");
Tree *xl_list_files(Tree *patterns);


// ============================================================================
//
//    Loading trees from external files
//
// ============================================================================

Tree *xl_load(text name);
Tree *xl_import(text name);
Tree *xl_load_data(Tree *self,
                   text name, text prefix,
                   text fieldSeps = ",;", text recordSeps = "\n");



// ============================================================================
// 
//    Inline functions
// 
// ============================================================================

inline Tree *xl_source(Tree *value)
// ----------------------------------------------------------------------------
//   Return the source that led to the evaluation of a given tree
// ----------------------------------------------------------------------------
{
    if (Tree *source = value->source)
        return source;
    return value;
}


inline Tree *xl_set_source(Tree *value, Tree *source)
// ----------------------------------------------------------------------------
//   Return the source that led to the evaluation of a given tree
// ----------------------------------------------------------------------------
{
    source = xl_source(source);
    if (source == value)
        source = NULL;
    value->source = source;
    return value;
}

XL_END

#endif // RUNTIME_H
