#ifndef COMPILER_EXPRED_H
#define COMPILER_EXPRED_H
// *****************************************************************************
// expred.h                                                           XL project
// *****************************************************************************
//
// File description:
//
//    Information required by the compiler for expression reduction
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

XL_BEGIN

struct RewriteCandidate;

struct CompileExpression
// ----------------------------------------------------------------------------
//   Collect parameters on the left of a rewrite
// ----------------------------------------------------------------------------
{
    typedef llvm_value value_type;

public:
    CompileExpression(CompiledUnit *unit);

public:
    llvm_value DoInteger(Integer *what);
    llvm_value DoReal(Real *what);
    llvm_value DoText(Text *what);
    llvm_value DoName(Name *what);
    llvm_value DoPrefix(Prefix *what);
    llvm_value DoPostfix(Postfix *what);
    llvm_value DoInfix(Infix *what);
    llvm_value DoBlock(Block *what);

    llvm_value DoCall(Tree *call);
    llvm_value DoRewrite(RewriteCandidate &candidate);
    llvm_value Value(Tree *expr);
    llvm_value Compare(Tree *value, Tree *test);
    llvm_value ForceEvaluation(Tree *expr);
    llvm_value TopLevelEvaluation(Tree *expr);

public:
    CompiledUnit *  unit;         // Current compilation unit
    LLVMCrap::JIT & llvm;         // JIT compiler being used
    value_map       computed;     // Values we already computed
};

XL_END

#endif // COMPILER_EXPRED_H
