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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
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
struct Context;
struct Main;



// ============================================================================
//
//    Runtime functions with C interface
//
// ============================================================================

Tree *xl_identity(Context*, Tree *);
Tree *xl_evaluate(Context *, Tree *);
Tree *xl_evaluate_children(Context *, Tree *);
Tree *xl_assigned_value(Context *, Tree *);
Tree *xl_named_value(Context *, Tree *);
Tree *xl_source(Tree *);
Tree *xl_set_source(Tree *value, Tree *source);
Tree *xl_error(text msg, Tree *a1=0, Tree *a2=0, Tree *a3=0);
Tree *xl_form_error(Context *c, Tree *tree);

Tree *xl_parse_tree(Context *, Tree *tree);
Tree *xl_bound(Context *, Tree *form);
bool  xl_same_text(Tree * , const char *);
bool  xl_same_shape(Tree *t1, Tree *t2);
Tree *xl_infix_match_check(Tree *value, kstring name);
Tree *xl_type_check(Context *, Tree *value, Tree *type);

Integer *xl_new_integer(longlong value);
Real    *xl_new_real(double value);
Text    *xl_new_character(char value);
Text    *xl_new_ctext(kstring value);
Text    *xl_new_text(text value);
Text    *xl_new_xtext(kstring value, longlong len, kstring open, kstring close);
Block   *xl_new_block(Block *source, Tree *child);
Prefix  *xl_new_prefix(Prefix *source, Tree *left, Tree *right);
Postfix *xl_new_postfix(Postfix *source, Tree *left, Tree *right);
Infix   *xl_new_infix(Infix *source, Tree *left, Tree *right);

Tree *xl_new_closure(Tree *expr, uint ntrees, ...);

Tree *xl_boolean_cast(Context *, Tree *source, Tree *value);
Tree *xl_integer_cast(Context *, Tree *source, Tree *value);
Tree *xl_real_cast(Context *, Tree *source, Tree *value);
Tree *xl_text_cast(Context *, Tree *source, Tree *value);
Tree *xl_character_cast(Context *, Tree *source, Tree *value);
Tree *xl_tree_cast(Context *, Tree *source, Tree *value);
Tree *xl_source_cast(Context *, Tree *source, Tree *value);
Tree *xl_code_cast(Context *, Tree *source, Tree *value);
Tree *xl_lazy_cast(Context *, Tree *source, Tree *value);
Tree *xl_value_cast(Context *, Tree *source, Tree *value);
Tree *xl_symbol_cast(Context *, Tree *source, Tree *value);
Tree *xl_name_cast(Context *, Tree *source, Tree *value);
Tree *xl_operator_cast(Context *, Tree *source, Tree *value);
Tree *xl_infix_cast(Context *, Tree *source, Tree *value);
Tree *xl_prefix_cast(Context *, Tree *source, Tree *value);
Tree *xl_postfix_cast(Context *, Tree *source, Tree *value);
Tree *xl_block_cast(Context *, Tree *source, Tree *value);

Tree *xl_parameter(text name, text type);
void xl_infix_to_list(Infix *infix, TreeList &list);

Real *xl_springify(Real &value, Real &target, Real &time,
                   Real &damp, Real &kspring, Real &lt, Real &ls);


// ============================================================================
//
//   Initialization code
//
// ============================================================================

void xl_enter_builtin(Main *main, text name, Tree *to, TreeList parms);
void xl_enter_global(Main *main, Name *name, Name_p *address);



// ============================================================================
//
//    Call management
//
// ============================================================================

struct XLCall
// ----------------------------------------------------------------------------
//    A structure that encapsulates a call to an XL tree
// ----------------------------------------------------------------------------
{
    XLCall(text name):
        name(new Name(name)), arguments(NULL), pointer(&arguments) {}

    // Adding arguments
    XLCall &operator, (Tree *tree)
    {
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
        return *this;
    }
    XLCall &operator, (Tree &tree) { return *this, &tree; }
    XLCall &operator, (longlong v) { return *this, new Integer(v); }
    XLCall &operator, (double  v)  { return *this, new Real(v); }
    XLCall &operator, (text  v)    { return *this, new Text(v); }

    // Calling in a given symbol context
    Tree *  operator() (Context *context)
    {
        Tree *call = name;
        if (arguments)
            call = new Prefix(call, arguments);
        return xl_evaluate(context, call);
    }

    // Calling in a given symbol context
    Tree *  operator() (Symbols *syms = NULL,
                       bool nullIfBad = false, bool cached = true);
    Tree *  build(Symbols *syms = NULL);

public:
    Name_p      name;
    TreeList    args;
    Tree_p      arguments;
    Tree_p *    pointer;
};



// ============================================================================
//
//    Basic text I/O (temporary)
//
// ============================================================================

Tree *xl_write(Context *context, Tree *what, text sep = "");
extern "C"
{
    bool xl_write_integer(longlong);
    bool xl_write_real(double);
    bool xl_write_text(kstring);
    bool xl_write_character(char c);
    bool xl_write_cr(void);
}
Tree *xl_list_files(Context *context, Tree *patterns);



// ============================================================================
//
//    Loading trees from external files
//
// ============================================================================

Tree *xl_load(Context *, text name);
Tree *xl_import(Context *, text name);
Tree *xl_load_data(Context *,
                   text name, text prefix,
                   text fieldSeps = ",;", text recordSeps = "\n");

XL_END

#endif // RUNTIME_H
