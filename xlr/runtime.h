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

Tree *  xl_form_error(Context *c, Tree *tree);
Tree *  xl_stack_overflow(Tree *tree);
bool    xl_same_shape(Tree *t1, Tree *t2);

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



// ============================================================================
// 
//    Some utility functions used in basics.tbl
// 
// ============================================================================

extern "C"
{
//#pragma GCC diagnostic ignored "-Wreturn-type-c-linkage"
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

Tree *  xl_parse_tree(Context *, Tree *tree);
Tree *  xl_parse_text(text source);



// ============================================================================
// 
//   File utilities
// 
// ============================================================================

Tree *  xl_list_files(Context *context, Tree *patterns);
bool    xl_file_exists(Context *context, Tree_p self, text path);



// ============================================================================
// 
//   Loading trees from external files
// 
// ============================================================================

Tree *  xl_import(Context *, Tree *self, text name, int phase);
Tree *  xl_load_data(Context *, Tree *self,
                     text name, text prefix,
                     text fieldSeps = ",;", text recordSeps = "\n",
                     Tree *body = NULL);
Tree *  xl_load_data(Context *, Tree *self, text inputName,
                     std::istream &source, bool cached, bool statTime,
                     text prefix, text fieldSeps = ",;", text recordSeps = "\n",
                     Tree *body = NULL);
Tree *  xl_add_search_path(Context *, text prefix, text dir);
Text *  xl_find_in_search_path(Context *, text prefix, text file);

typedef enum { PARSING_PHASE, DECLARATION_PHASE, EXECUTION_PHASE } phase_t;
typedef Tree * (*decl_fn) (Context *, Tree *source, phase_t phase);
Name *  xl_set_override_priority(Context *context, Tree *self, float priority);

XL_END


extern uint xl_recursion_count;

#endif // RUNTIME_H
