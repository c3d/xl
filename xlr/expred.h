#ifndef COMPILER_EXPRED_H
#define COMPILER_EXPRED_H
// ****************************************************************************
//  expred.h                                                        XLR project
// ****************************************************************************
//
//   File Description:
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
