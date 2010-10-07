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
Tree *          ParametersTree(TreeList parameters);
void            setDocumentation(Tree *node, text doc);

#define XL_TREE(v)      (v)
#define XL_INT(v)       ((integer_t) (v))
#define XL_CHAR(v)      xl_character_arg(v)
#define XL_REAL(v)      ((real_t) (v))
#define XL_TEXT(v)      ((text_t) (v))
#define XL_BOOL(v)      ((boolean_t) (v))
#define XL_RTREE(val)   return (val)
#define XL_RINT(val)    return (XL::Integer *) xl_set_source(new XL::Integer(val), self)
#define XL_RREAL(val)   return (XL::Real *) xl_set_source(new XL::Real(val), self)
#define XL_RTEXT(val)   return (XL::Text *) xl_set_source(new XL::Text(val), self);
#define XL_RBOOL(val)   return (val) ? XL::xl_true : XL::xl_false

typedef Integer &       integer_r;
typedef Real &          real_r;
typedef Text &          text_r;
typedef Name &          boolean_r;
typedef Name &          symbol_r;
typedef Tree &          tree_r;
typedef Infix &         infix_r;
typedef Prefix &        prefix_r;
typedef Postfix &       postfix_r;
typedef Block &         block_r;

typedef Integer_p       integer_p;
typedef Real_p          real_p;
typedef Text_p          text_p;
typedef Name_p          boolean_p;
typedef Name_p          symbol_p;
typedef Tree_p          tree_p;
typedef Infix_p         infix_p;
typedef Prefix_p        prefix_p;
typedef Postfix_p       postfix_p;
typedef Block_p         block_p;

typedef longlong        integer_t;
typedef double          real_t;
typedef text            text_t;
typedef bool            boolean_t;
typedef Name &          symbol_t;
typedef Tree &          tree_t;
typedef Infix &         infix_t;
typedef Prefix &        prefix_t;
typedef Postfix &       postfix_t;
typedef Block &         block_t;

typedef Integer *       integer_nkp;
typedef Real *          real_nkp;
typedef Text *          text_nkp;
typedef Name *          boolean_nkp;
typedef Name *          symbol_nkp;
typedef Tree *          tree_nkp;
typedef Infix *         infix_nkp;
typedef Prefix *        prefix_nkp;
typedef Postfix *       postfix_nkp;
typedef Block *         block_nkp;

XL_END

#endif // OPCODES_H
