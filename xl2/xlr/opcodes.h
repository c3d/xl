#ifndef OPCODES_H
#define OPCODES_H
// ****************************************************************************
//  opcodes.h                       (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//    Opcodes are native trees generated as part of compilation/optimization
//    to speed up execution. They represent a step in the evaluation of
//    the code.
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

#include "tree.h"
#include "context.h"

XL_BEGIN

longlong        xl_integer_arg(Tree *arg);
double          xl_real_arg(Tree *arg);
text            xl_text_arg(Tree *arg);
int             xl_character_arg(Tree *arg);
bool            xl_boolean_arg(Tree *arg);
Tree *          ParametersTree(tree_list parameters);
extern Name *   xl_true;
extern Name *   xl_false;

#define TREE(v)         (v)
#define INT(v)          ((integer_t) (v))
#define CHAR(v)         xl_character_arg(v)
#define REAL(v)         ((real_t) (v))
#define TEXT(v)         ((text_t) (v))
#define BOOL(v)         ((boolean_t) (v))
#define RINT(val)       return new Integer(val)
#define RREAL(val)      return new Real(val)
#define RTEXT(val)      return new Text(val)
#define RBOOL(val)      return (val) ? xl_true : xl_false
#define RTREE(val)      return (val)

typedef Integer &       integer_r;
typedef Real &          real_r;
typedef Text &          text_r;
typedef Name &          boolean_r;
typedef Tree &          tree_r;
typedef Infix &         infix_r;
typedef Prefix &        prefix_r;
typedef Postfix &       postfix_r;
typedef Block &         block_r;

typedef longlong        integer_t;
typedef double          real_t;
typedef text            text_t;
typedef bool            boolean_t;
typedef Tree &          tree_t;
typedef Infix &         infix_t;
typedef Prefix &        prefix_t;
typedef Postfix &       postfix_t;
typedef Block &         block_t;

XL_END

#endif // OPCODES_H
