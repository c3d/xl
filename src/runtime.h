#ifndef RUNTIME_H
#define RUNTIME_H
// ****************************************************************************
//  runtime.h                       (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                              ELIOT project
// ****************************************************************************
//
//   File Description:
//
//     Functions required for proper run-time execution of ELIOT programs
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


ELIOT_BEGIN
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

Tree *  eliot_form_error(Context *c, Tree *tree);
Tree *  eliot_stack_overflow(Tree *tree);
bool    eliot_same_shape(Tree *t1, Tree *t2);

Integer *eliot_new_integer(longlong value);
Real    *eliot_new_real(double value);
Text    *eliot_new_character(char value);
Text    *eliot_new_ctext(kstring value);
Text    *eliot_new_text(text value);
Text    *eliot_new_xtext(kstring value, longlong len, kstring open, kstring close);
Block   *eliot_new_block(Block *source, Tree *child);
Prefix  *eliot_new_prefix(Prefix *source, Tree *left, Tree *right);
Postfix *eliot_new_postfix(Postfix *source, Tree *left, Tree *right);
Infix   *eliot_new_infix(Infix *source, Tree *left, Tree *right);



// ============================================================================
// 
//    Some utility functions used in basics.tbl
// 
// ============================================================================

extern "C"
{
#pragma GCC diagnostic ignored "-Wreturn-type-c-linkage"
integer_t       eliot_text2int(kstring t);
real_t          eliot_text2real(kstring t);
text            eliot_int2text(integer_t value);
text            eliot_real2text(real_t value);
integer_t       eliot_mod(integer_t x, integer_t y);
integer_t       eliot_pow(integer_t x, integer_t y);
real_t          eliot_modf(real_t x, real_t y);
real_t          eliot_powf(real_t x, integer_t y);
text            eliot_text_replace(text txt, text before, text after);
text            eliot_text_repeat(uint count, text data);

real_t          eliot_time(real_t delay);
integer_t       eliot_seconds();
integer_t       eliot_minutes();
integer_t       eliot_hours();
integer_t       eliot_month_day();
integer_t       eliot_mon();
integer_t       eliot_year();
integer_t       eliot_week_day();
integer_t       eliot_year_day();
integer_t       eliot_summer_time();
text_t          eliot_timezone();
integer_t       eliot_GMT_offset();

real_t          eliot_random();
bool            eliot_random_seed(int seed);
}

template<typename number>
inline number   eliot_random(number low, number high)
// ----------------------------------------------------------------------------
//    Return a pseudo-random number in the low..high range
// ----------------------------------------------------------------------------
{
    return number(eliot_random() * (high-low) + low);
}



// ============================================================================
//
//    Basic text I/O (temporary)
//
// ============================================================================

extern "C"
{
    bool      eliot_write_integer(longlong);
    bool      eliot_write_real(double);
    bool      eliot_write_text(kstring);
    bool      eliot_write_character(char c);
    bool      eliot_write_tree(ELIOT::Tree *t);
    bool      eliot_write_cr(void);
}



// ============================================================================
// 
//    Basic runtime functions callable from generated code or opcodes
// 
// ============================================================================

extern "C"
{
    integer_t eliot_mod(integer_t, integer_t);
    real_t    eliot_modf(real_t, real_t);
}


// ============================================================================
//
//    Parsing trees
//
// ============================================================================

Tree *  eliot_parse_tree(Context *, Tree *tree);
Tree *  eliot_parse_text(text source);



// ============================================================================
// 
//   File utilities
// 
// ============================================================================

Tree *  eliot_list_files(Context *context, Tree *patterns);
bool    eliot_file_exists(Context *context, Tree_p self, text path);



// ============================================================================
// 
//   Loading trees from external files
// 
// ============================================================================

Tree *  eliot_import(Context *, Tree *self, text name, int phase);
Tree *  eliot_load_data(Context *, Tree *self,
                     text name, text prefix,
                     text fieldSeps = ",;", text recordSeps = "\n",
                     Tree *body = NULL);
Tree *  eliot_load_data(Context *, Tree *self, text inputName,
                     std::istream &source, bool cached, bool statTime,
                     text prefix, text fieldSeps = ",;", text recordSeps = "\n",
                     Tree *body = NULL);
Tree *  eliot_add_search_path(Context *, text prefix, text dir);
Text *  eliot_find_in_search_path(Context *, text prefix, text file);

typedef enum { PARSING_PHASE, DECLARATION_PHASE, EXECUTION_PHASE } phase_t;
typedef Tree * (*decl_fn) (Context *, Tree *source, phase_t phase);
Name *  eliot_set_override_priority(Context *context, Tree *self, float priority);

ELIOT_END


extern uint eliot_recursion_count;

#endif // RUNTIME_H
