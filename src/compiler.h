#ifndef COMPILER_H
#define COMPILER_H
// *****************************************************************************
// compiler.h                                                         XL project
// *****************************************************************************
//
// File description:
//
//    Just-in-time compiler for the trees
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
// (C) 2009-2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
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

#include "tree.h"
#include "context.h"
#include "evaluator.h"
#include "llvm-crap.h"
#include <map>
#include <set>



// ============================================================================
//
//    Forward declarations
//
// ============================================================================

RECORDER_DECLARE(compiler);
RECORDER_DECLARE(compiler_warning);
RECORDER_DECLARE(compiler_error);

XL_BEGIN
// ============================================================================
//
//    Global structures to access the LLVM just-in-time compiler
//
// ============================================================================

struct Compiler : Evaluator
// ----------------------------------------------------------------------------
//   Just-in-time compiler data
// ----------------------------------------------------------------------------
{
    Compiler(kstring name, unsigned opts, int argc, char **argv);
    ~Compiler();

    // Interpreter interface
    Tree *              Evaluate(Scope *, Tree *source) override;
    Tree *              TypeCheck(Scope *, Tree *type, Tree *val) override;
    bool                TypeAnalysis(Scope *, Tree *tree) override;

    // Find the machine type corresponding to the tree type or value
    JIT::PointerType_p  TreeMachineType(Tree *tree);
    JIT::Type_p         MachineType(Tree *tree);

public:
    JIT                 jit;
    JIT::Type_p         voidTy;
    JIT::IntegerType_p  booleanTy;
    JIT::IntegerType_p  integerTy;
    JIT::IntegerType_p  integer8Ty;
    JIT::IntegerType_p  integer16Ty;
    JIT::IntegerType_p  integer32Ty;
    JIT::IntegerType_p  integer64Ty;
    JIT::IntegerType_p  integer128Ty;
    JIT::IntegerType_p  unsignedTy;
    JIT::IntegerType_p  ulongTy;
    JIT::IntegerType_p  ulonglongTy;
    JIT::Type_p         realTy;
    JIT::Type_p         real32Ty;
    JIT::Type_p         real64Ty;
    JIT::IntegerType_p  characterTy;
    JIT::PointerType_p  charPtrTy;
    JIT::PointerType_p  charPtrPtrTy;
    JIT::StructType_p   textTy;
    JIT::PointerType_p  textPtrTy;
    JIT::StructType_p   infoTy;
    JIT::PointerType_p  infoPtrTy;
    JIT::StructType_p   treeTy;
    JIT::PointerType_p  treePtrTy;
    JIT::PointerType_p  treePtrPtrTy;
    JIT::StructType_p   integerTreeTy;
    JIT::PointerType_p  integerTreePtrTy;
    JIT::StructType_p   realTreeTy;
    JIT::PointerType_p  realTreePtrTy;
    JIT::StructType_p   textTreeTy;
    JIT::PointerType_p  textTreePtrTy;
    JIT::StructType_p   nameTreeTy;
    JIT::PointerType_p  nameTreePtrTy;
    JIT::StructType_p   blockTreeTy;
    JIT::PointerType_p  blockTreePtrTy;
    JIT::StructType_p   prefixTreeTy;
    JIT::PointerType_p  prefixTreePtrTy;
    JIT::StructType_p   postfixTreeTy;
    JIT::PointerType_p  postfixTreePtrTy;
    JIT::StructType_p   infixTreeTy;
    JIT::PointerType_p  infixTreePtrTy;
    JIT::StructType_p   scopeTy;
    JIT::PointerType_p  scopePtrTy;
    JIT::FunctionType_p evalTy;
    JIT::PointerType_p  evalFnTy;
};



// ============================================================================
//
//   Useful macros
//
// ============================================================================

// Index in data structures of fields in Tree types
#define TAG_INDEX           0
#define INFO_INDEX          1
#define INTEGER_VALUE_INDEX 2
#define REAL_VALUE_INDEX    2
#define TEXT_VALUE_INDEX    2
#define TEXT_OPENING_INDEX  3
#define TEXT_CLOSING_INDEX  4
#define NAME_VALUE_INDEX    2
#define BLOCK_CHILD_INDEX   2
#define BLOCK_OPENING_INDEX 3
#define BLOCK_CLOSING_INDEX 4
#define LEFT_VALUE_INDEX    2
#define RIGHT_VALUE_INDEX   3
#define INFIX_NAME_INDEX    4

XL_END

#endif // COMPILER_H
