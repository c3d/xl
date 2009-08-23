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

struct Tree;                                    // 



// ============================================================================
// 
//    Runtime functions with C interface
// 
// ============================================================================

Tree *xl_identity(Tree *);
Tree *xl_evaluate(Tree *);
bool  xl_same_text(Tree *, const char *);
bool  xl_same_shape(Tree *t1, Tree *t2);
bool  xl_type_check(Tree *value, Tree *type);

Tree *xl_new_integer(longlong value);
Tree *xl_new_real(double value);
Tree *xl_new_character(kstring value);
Tree *xl_new_text(kstring value);
Tree *xl_new_xtext(kstring value, kstring open, kstring close);

Tree *xl_new_closure(Tree *expr, uint ntrees, ...);

Tree *xl_boolean(Tree *value);    
Tree *xl_integer(Tree *value);
Tree *xl_real(Tree *value);
Tree *xl_text(Tree *value);
Tree *xl_character(Tree *value);
Tree *xl_tree(Tree *value);
Tree *xl_infix(Tree *value);
Tree *xl_prefix(Tree *value);
Tree *xl_postfix(Tree *value);
Tree *xl_block(Tree *value);

XL_END

#endif // RUNTIME_H
