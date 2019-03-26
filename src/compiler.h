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
    PointerType_p       TreeMachineType(Tree *tree);
    Type_p              MachineType(Tree *tree);

public:
    JIT                 jit;
    Type_p              voidTy;
    IntegerType_p       booleanTy;
    IntegerType_p       integerTy;
    IntegerType_p       integer8Ty;
    IntegerType_p       integer16Ty;
    IntegerType_p       integer32Ty;
    IntegerType_p       integer64Ty;
    IntegerType_p       integer128Ty;
    IntegerType_p       unsignedTy;
    IntegerType_p       ulongTy;
    IntegerType_p       ulonglongTy;
    Type_p              realTy;
    Type_p              real32Ty;
    Type_p              real64Ty;
    IntegerType_p       characterTy;
    PointerType_p       charPtrTy;
    PointerType_p       charPtrPtrTy;
    StructType_p        textTy;
    StructType_p        infoTy;
    PointerType_p       infoPtrTy;
    StructType_p        treeTy;
    PointerType_p       treePtrTy;
    PointerType_p       treePtrPtrTy;
    StructType_p        integerTreeTy;
    PointerType_p       integerTreePtrTy;
    StructType_p        realTreeTy;
    PointerType_p       realTreePtrTy;
    StructType_p        textTreeTy;
    PointerType_p       textTreePtrTy;
    StructType_p        nameTreeTy;
    PointerType_p       nameTreePtrTy;
    StructType_p        blockTreeTy;
    PointerType_p       blockTreePtrTy;
    StructType_p        prefixTreeTy;
    PointerType_p       prefixTreePtrTy;
    StructType_p        postfixTreeTy;
    PointerType_p       postfixTreePtrTy;
    StructType_p        infixTreeTy;
    PointerType_p       infixTreePtrTy;
    StructType_p        scopeTy;
    PointerType_p       scopePtrTy;
    FunctionType_p      evalTy;
    PointerType_p       evalFnTy;
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
