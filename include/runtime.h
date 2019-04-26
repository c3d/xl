#ifndef RUNTIME_H
#define RUNTIME_H
// *****************************************************************************
// runtime.h                                                          XL project
// *****************************************************************************
//
// File description:
//
//     Functions required for proper run-time execution of XL programs
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2012, Baptiste Soulisse <baptiste.soulisse@taodyne.com>
// (C) 2011, Catherine Burvelle <catherine@taodyne.com>
// (C) 2009-2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010-2013, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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

#include "base.h"
#include "tree.h"
#include "main.h"
#include "basics.h"

#include <cmath>


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
struct SourceFile;



// ============================================================================
//
//    Runtime functions called by the compiler
//
// ============================================================================

extern "C"
{
Tree *  xl_evaluate(Scope *c, Tree *tree);
Tree *  xl_identity(Scope *c, Tree *tree);
Tree *  xl_typecheck(Scope *c, Tree *type, Tree *value);
Tree *  xl_call(Scope *c, text prefix, TreeList &args);
Tree *  xl_assign(Scope *c, Tree *ref, Tree *value);
Tree *  xl_form_error(Scope *c, Tree *tree);
Tree *  xl_stack_overflow(Tree *tree);
bool    xl_same_text(Tree * , const char *);
bool    xl_same_shape(Tree *t1, Tree *t2);

Integer *xl_new_integer(TreePosition pos, longlong value);
Real    *xl_new_real(TreePosition pos, double value);
Text    *xl_new_character(TreePosition pos, char value);
Text    *xl_new_ctext(TreePosition pos, kstring value);
Text    *xl_new_text(TreePosition pos, text value);
Text    *xl_new_xtext(TreePosition pos, kstring value, longlong len, kstring open, kstring close);
Block   *xl_new_block(Block *source, Tree *child);
Prefix  *xl_new_prefix(Prefix *source, Tree *left, Tree *right);
Postfix *xl_new_postfix(Postfix *source, Tree *left, Tree *right);
Infix   *xl_new_infix(Infix *source, Tree *left, Tree *right);
Tree *   xl_array_index(Scope *, Tree *data, Tree *index);
Tree     *xl_new_closure(eval_fn toCall, Tree *expr, size_t ntrees, ...);



// ============================================================================
//
//    Some utility functions used in basics.tbl
//
// ============================================================================

#pragma GCC diagnostic push
// You first have to ignore the fact that the following pragma may be ignored!
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wreturn-type-c-linkage"
integer_t       xl_text2int(kstring t);
real_t          xl_text2real(kstring t);
text            xl_int2text(integer_t value);
text            xl_real2text(real_t value);
integer_t       xl_mod(integer_t x, integer_t y);
integer_t       xl_pow(integer_t x, integer_t y);
real_t          xl_modf(real_t x, real_t y);
real_t          xl_powf(real_t x, integer_t y);
text            xl_text_replace(text txt, text before, text after);
text            xl_text_repeat(uint count, text data);

real_t          xl_time(real_t delay);
integer_t       xl_seconds();
integer_t       xl_minutes();
integer_t       xl_hours();
integer_t       xl_month_day();
integer_t       xl_mon();
integer_t       xl_year();
integer_t       xl_week_day();
integer_t       xl_year_day();
integer_t       xl_summer_time();
text_t          xl_timezone();
integer_t       xl_GMT_offset();

real_t          xl_random();
bool            xl_random_seed(int seed);
#pragma GCC diagnostic pop
}

template<typename number>
inline number   xl_random(number low, number high)
// ----------------------------------------------------------------------------
//    Return a pseudo-random number in the low..high range
// ----------------------------------------------------------------------------
{
    return number(xl_random() * (high-low) + low);
}



// ============================================================================
//
//    Basic text I/O (temporary)
//
// ============================================================================

extern "C"
{
    bool      xl_write_integer(longlong);
    bool      xl_write_real(double);
    bool      xl_write_text(kstring);
    bool      xl_write_character(char c);
    bool      xl_write_tree(XL::Tree *t);
    bool      xl_write_cr(void);
}



// ============================================================================
//
//    Basic runtime functions callable from generated code or opcodes
//
// ============================================================================

extern "C"
{
    integer_t xl_mod(integer_t, integer_t);
    real_t    xl_modf(real_t, real_t);
}


// ============================================================================
//
//    Parsing trees
//
// ============================================================================

Tree *  xl_parse_tree(Scope *, Tree *tree);
Tree *  xl_parse_text(text source);



// ============================================================================
//
//   File utilities
//
// ============================================================================

Tree *  xl_list_files(Scope *scope, Tree *patterns);
bool    xl_file_exists(Scope *scope, Tree_p self, text path);



// ============================================================================
//
//   Loading trees from external files
//
// ============================================================================

Tree *  xl_import(Scope *scope, Tree *self);
Tree *  xl_load(Scope *scope, Tree *self);
Tree *  xl_load_data(Scope *, Tree *self,
                     text name, text prefix,
                     text fieldSeps = ",;", text recordSeps = "\n",
                     Tree *body = NULL);
Tree *  xl_load_data(Scope *, Tree *self, text inputName,
                     std::istream &source, bool cached, bool statTime,
                     text prefix, text fieldSeps = ",;", text recordSeps = "\n",
                     Tree *body = NULL);
Tree *  xl_add_search_path(Scope *, text prefix, text dir);
Text *  xl_find_in_search_path(Scope *, text prefix, text file);

typedef enum { PARSING_PHASE, DECLARATION_PHASE, EXECUTION_PHASE } phase_t;
typedef Tree * (*decl_fn) (Scope *, Tree *source, phase_t phase);
Name *  xl_set_override_priority(Scope *scope, Tree *self, float priority);



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
        name(new Name(name)), arguments(NULL), pointer(&arguments), call() {}

    // Adding arguments
    XLCall &operator, (Tree *tree)
    {
        call = NULL;
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
        args.push_back(tree);
        return *this;
    }
    XLCall &operator, (Tree &tree) { return *this, &tree; }
    XLCall &operator, (longlong v) { return *this, new Integer(v); }
    XLCall &operator, (double  v)  { return *this, new Real(v); }
    XLCall &operator, (text  v)    { return *this, new Text(v); }

    // Calling in a given symbol context
    Tree *  operator() (Scope *syms);
    bool    build(Scope *syms);

public:
    Name_p      name;
    TreeList    args;
    Tree_p      arguments;
    Tree_p *    pointer;
    Prefix_p    call;
};

XL_END


extern uint xl_recursion_count;

#endif // RUNTIME_H
