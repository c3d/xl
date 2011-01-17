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
// This document is released under the GNU General Public License.
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
    CompileExpression(CompiledUnit *unit) : unit(unit) {}

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

public:
    CompiledUnit *  unit;         // Current compilation unit
    value_map       computed;     // Values we already computed
};

XL_END

#endif // COMPILER_EXPRED_H

