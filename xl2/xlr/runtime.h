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



// ============================================================================
//
//    Runtime functions with C interface
//
// ============================================================================

Tree *xl_identity(Tree *);
Tree *xl_evaluate(Tree *);
bool  xl_same_text(Tree *, const char *);
bool  xl_same_shape(Tree *t1, Tree *t2);
bool  xl_infix_match_check(Tree *value, Infix *ref);
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

Tree *xl_boolean_cast(Tree *source, Tree *value);
Tree *xl_integer_cast(Tree *source, Tree *value);
Tree *xl_real_cast(Tree *source, Tree *value);
Tree *xl_text_cast(Tree *source, Tree *value);
Tree *xl_character_cast(Tree *source, Tree *value);
Tree *xl_tree_cast(Tree *source, Tree *value);
Tree *xl_symbolicname_cast(Tree *source, Tree *value);
Tree *xl_infix_cast(Tree *source, Tree *value);
Tree *xl_prefix_cast(Tree *source, Tree *value);
Tree *xl_postfix_cast(Tree *source, Tree *value);
Tree *xl_block_cast(Tree *source, Tree *value);

Tree *xl_invoke(Tree *(*toCall)(Tree *),
                Tree *source, uint numarg, Tree **args);

Tree *xl_call(text name);
Tree *xl_call(text name, text arg);
Tree *xl_call(text name, double x, double y);
Tree *xl_call(text name, double x, double y, double w, double h);

Tree *xl_load(text name);
Tree *xl_load_csv(text name);
Tree *xl_load_tsv(text name);

XL_END

#endif // RUNTIME_H
