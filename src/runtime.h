#ifndef RUNTIME_H
#define RUNTIME_H
// ****************************************************************************
//  runtime.h                       (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                              ELFE project
// ****************************************************************************
//
//   File Description:
//
//     Functions required for proper run-time execution of ELFE programs
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

#include "base.h"
#include "tree.h"
#include "main.h"
#include "basics.h"

#include <cmath>


ELFE_BEGIN
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

Tree *  elfe_form_error(Context *c, Tree *tree);
Tree *  elfe_stack_overflow(Tree *tree);
bool    elfe_same_shape(Tree *t1, Tree *t2);

Integer *elfe_new_integer(longlong value);
Real    *elfe_new_real(double value);
Text    *elfe_new_character(char value);
Text    *elfe_new_ctext(kstring value);
Text    *elfe_new_text(text value);
Text    *elfe_new_xtext(kstring value, longlong len, kstring open, kstring close);
Block   *elfe_new_block(Block *source, Tree *child);
Prefix  *elfe_new_prefix(Prefix *source, Tree *left, Tree *right);
Postfix *elfe_new_postfix(Postfix *source, Tree *left, Tree *right);
Infix   *elfe_new_infix(Infix *source, Tree *left, Tree *right);



// ============================================================================
// 
//    Some utility functions used in basics.tbl
// 
// ============================================================================

extern "C"
{
#pragma GCC diagnostic push
// You first have to ignore the fact that the following pragma may be ignored!
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wreturn-type-c-linkage"
integer_t       elfe_text2int(kstring t);
real_t          elfe_text2real(kstring t);
text            elfe_int2text(integer_t value);
text            elfe_real2text(real_t value);
integer_t       elfe_mod(integer_t x, integer_t y);
integer_t       elfe_pow(integer_t x, integer_t y);
real_t          elfe_modf(real_t x, real_t y);
real_t          elfe_powf(real_t x, integer_t y);
text            elfe_text_replace(text txt, text before, text after);
text            elfe_text_repeat(uint count, text data);

real_t          elfe_time(real_t delay);
integer_t       elfe_seconds();
integer_t       elfe_minutes();
integer_t       elfe_hours();
integer_t       elfe_month_day();
integer_t       elfe_mon();
integer_t       elfe_year();
integer_t       elfe_week_day();
integer_t       elfe_year_day();
integer_t       elfe_summer_time();
text_t          elfe_timezone();
integer_t       elfe_GMT_offset();

real_t          elfe_random();
bool            elfe_random_seed(int seed);
#pragma GCC diagnostic pop
}

template<typename number>
inline number   elfe_random(number low, number high)
// ----------------------------------------------------------------------------
//    Return a pseudo-random number in the low..high range
// ----------------------------------------------------------------------------
{
    return number(elfe_random() * (high-low) + low);
}



// ============================================================================
//
//    Basic text I/O (temporary)
//
// ============================================================================

extern "C"
{
    bool      elfe_write_integer(longlong);
    bool      elfe_write_real(double);
    bool      elfe_write_text(kstring);
    bool      elfe_write_character(char c);
    bool      elfe_write_tree(ELFE::Tree *t);
    bool      elfe_write_cr(void);
}



// ============================================================================
// 
//    Basic runtime functions callable from generated code or opcodes
// 
// ============================================================================

extern "C"
{
    integer_t elfe_mod(integer_t, integer_t);
    real_t    elfe_modf(real_t, real_t);
}


// ============================================================================
//
//    Parsing trees
//
// ============================================================================

Tree *  elfe_parse_tree(Context *, Tree *tree);
Tree *  elfe_parse_text(text source);



// ============================================================================
// 
//   File utilities
// 
// ============================================================================

Tree *  elfe_list_files(Context *context, Tree *patterns);
bool    elfe_file_exists(Context *context, Tree_p self, text path);



// ============================================================================
// 
//   Loading trees from external files
// 
// ============================================================================

Tree *  elfe_import(Context *, Tree *self, text name, int phase);
Tree *  elfe_load_data(Context *, Tree *self,
                     text name, text prefix,
                     text fieldSeps = ",;", text recordSeps = "\n",
                     Tree *body = NULL);
Tree *  elfe_load_data(Context *, Tree *self, text inputName,
                     std::istream &source, bool cached, bool statTime,
                     text prefix, text fieldSeps = ",;", text recordSeps = "\n",
                     Tree *body = NULL);
Tree *  elfe_add_search_path(Context *, text prefix, text dir);
Text *  elfe_find_in_search_path(Context *, text prefix, text file);

typedef enum { PARSING_PHASE, DECLARATION_PHASE, EXECUTION_PHASE } phase_t;
typedef Tree * (*decl_fn) (Context *, Tree *source, phase_t phase);
Name *  elfe_set_override_priority(Context *context, Tree *self, float priority);

ELFE_END


extern uint elfe_recursion_count;

#endif // RUNTIME_H
