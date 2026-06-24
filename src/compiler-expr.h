#ifndef COMPILER_EXPR_H
#define COMPILER_EXPR_H
// *****************************************************************************
// compiler-expr.h                                                    XL project
// *****************************************************************************
//
// File description:
//
//    Compilation of expressions ("expression reduction")
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2010-2011,2015-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
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

#include "compiler.h"
#include "compiler-function.h"

XL_BEGIN

class  JITBlock;

class CompilerExpression
// ----------------------------------------------------------------------------
//   Collect parameters on the left of a rewrite
// ----------------------------------------------------------------------------
{
    CompilerFunction &  function;       // Current compilation function
    value_map           computed;       // Values we already computed
    Context_g           context;        // Context of evaluation
    bool                inTailPosition; // True when value is returned directly

private:
    void        SeedCandidateBindings(CompilerRewriteCandidate *cand);
    void        SeedPeeledKnownValues(Tree *call, Tree *rewriteCall);
    void        SeedKnownCompositeLeaves(Name *sourceName, Tree *expansion);
    void        SeedKnownCompositeLeaves(Tree *expansion,
                                         JIT::Value_p boxed,
                                         JIT::Type_p boxedTy,
                                         uint &index);
    JIT::Value_p BindingArgValue(Tree *arg);

  public:
    typedef JIT::Value_p value_type;
    CompilerExpression(CompilerFunction &function, bool tail = false);
    CompilerExpression(CompilerFunction &function, Context *context);
    CompilerExpression(CompilerFunction &function, Context *context, bool tail);

public:
    value_type  Evaluate(Tree *tree, bool force = false);

    value_type  Do(Natural *what);
    value_type  Do(Real *what);
    value_type  Do(Text *what);
    value_type  Do(Name *what);
    value_type  Do(Prefix *what);
    value_type  Do(Postfix *what);
    value_type  Do(Infix *what);
    value_type  Do(Block *what);

    value_type  DoCall(Tree *call, bool mayfail = false);
    value_type  DoRewrite(Tree *call, CompilerRewriteCandidate *candidate);
    value_type  DoAssignment(Infix *assign);
    value_type  Value(Tree *expr);
    value_type  Compare(Tree *value, Tree *test);
};

XL_END

RECORDER_DECLARE(compiler_expr);

#endif // COMPILER_EXPRED_H
