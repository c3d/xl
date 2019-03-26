#ifndef COMPILER_EXPR_H
#define COMPILER_EXPR_H
// *****************************************************************************
// expred.h                                                           XL project
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
// This software is licensed under the GNU General Public License v3
// (C) 2010-2011,2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can r redistribute it and/or modify
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

#include "compiler.h"
#include "compiler-function.h"

XL_BEGIN

struct RewriteCandidate;
class  JITBlock;

class CompilerExpression
// ----------------------------------------------------------------------------
//   Collect parameters on the left of a rewrite
// ----------------------------------------------------------------------------
{
    CompilerFunction &  function;       // Current compilation function
    value_map           computed;       // Values we already computed

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

    Value_p             DoCall(Tree *call, bool mayfail = false);
    Value_p             DoRewrite(Tree *call, RewriteCandidate *candidate);
    Value_p             Value(Tree *expr);
    Value_p             Compare(Tree *value, Tree *test);
};

XL_END

RECORDER_DECLARE(compiler_expr);

#endif // COMPILER_EXPRED_H
