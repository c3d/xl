#ifndef COMPILER_EXPR_H
#define COMPILER_EXPR_H
// ****************************************************************************
//  compiler-expr.h                                                 XL project
// ****************************************************************************
//
//   File Description:
//
//    Compilation of expression ("expression reduction")
//
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

#include "compiler.h"
#include "compiler-function.h"

XL_BEGIN

struct RewriteCandidate;
class  JITBlock;

typedef std::map<Types_p, value_map>    typed_value_map;

class CompilerExpression
// ----------------------------------------------------------------------------
//   Collect parameters on the left of a rewrite
// ----------------------------------------------------------------------------
{
    CompilerFunction &  function;       // Current compilation function
    CompilerUnit &      unit;           // Compilation unit
    Compiler &          compiler;       // The compiler being used
    JITBlock &          code;           // Code block used to generate stuff
    typed_value_map     computed;       // Values we already computed

public:
    typedef Value_p value_type;
    CompilerExpression(CompilerFunction &function);

public:
    Value_p             Evaluate(Tree *tree, bool force = false);

    Value_p             DoInteger(Integer *what);
    Value_p             DoReal(Real *what);
    Value_p             DoText(Text *what);
    Value_p             DoName(Name *what);
    Value_p             DoPrefix(Prefix *what);
    Value_p             DoPostfix(Postfix *what);
    Value_p             DoInfix(Infix *what);
    Value_p             DoBlock(Block *what);

    Value_p             DoCall(Tree *call);
    Value_p             DoRewrite(Tree *call, RewriteCandidate *candidate);
    Value_p             Value(Tree *expr);
    Value_p             Compare(Tree *value, Tree *test);
};

XL_END

RECORDER_DECLARE(compiler_expr);

#endif // COMPILER_EXPRED_H
