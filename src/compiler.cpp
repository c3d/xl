// *****************************************************************************
// compiler.cpp                                                       XL project
// *****************************************************************************
//
// File description:
//
//    Just-in-time (JIT) compiler for XL trees using LLVM as a back-end
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
// (C) 2009-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010-2012, Jérôme Forissier <jerome@taodyne.com>
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
#include "compiler-unit.h"
#include "builtins.h"
#include "errors.h"
#include "main.h"               // For Opt::emitIR

#include <recorder/recorder.h>
#include <iostream>
#include <sstream>
#include <cstdarg>


RECORDER(compiler,              16, "Compilation of XL trees");
RECORDER(compiler_warning,      16, "Warnings during XL compilation");
RECORDER(compiler_error,        16, "Errors during XL compilation");

XL_BEGIN
// ============================================================================
//
//    Compiler - Global information about the LLVM compiler
//
// ============================================================================
// The Compiler class is where we store all the global information that
// persists during the lifetime of the program: LLVM data structures,
// LLVM definitions for frequently used types, XL runtime functions, ...
//

using namespace llvm;

Compiler::Compiler(kstring moduleName, unsigned opts, int argc, char **argv)
// ----------------------------------------------------------------------------
//   Initialize the various types and global functions we may need
// ----------------------------------------------------------------------------
    : jit               (argc, argv),
      voidTy            (jit.VoidType()),
      booleanTy         (jit.IntegerType(1)),
      naturalTy         (jit.IntegerType<longlong>()),
      natural8Ty        (jit.IntegerType(8)),
      natural16Ty       (jit.IntegerType(16)),
      natural32Ty       (jit.IntegerType(32)),
      natural64Ty       (jit.IntegerType(64)),
      natural128Ty      (jit.IntegerType(128)),
      unsignedTy        (jit.IntegerType<unsigned>()),
      ulongTy           (jit.IntegerType<ulong>()),
      ulonglongTy       (jit.IntegerType<ulonglong>()),
      realTy            (jit.FloatType(64)),
      real16Ty          (jit.FloatType(16)),
      real32Ty          (jit.FloatType(32)),
      real64Ty          (jit.FloatType(64)),
      characterTy       (jit.IntegerType<char>()),
      charPtrTy         (jit.PointerType(characterTy)),
      textTy            (jit.StructType({charPtrTy}, "text")),
      textPtrTy         (jit.PointerType(textTy)),
      infoTy            (jit.OpaqueType("Info")),
      infoPtrTy         (jit.PointerType(infoTy)),

#define TREE    ulongTy, infoPtrTy
#define TREE1   TREE, treePtrTy
#define TREE2   TREE1, treePtrTy
      treeTy            (jit.StructType({TREE},                 "Tree")),
      treePtrTy         (jit.PointerType(treeTy)),
      treePtrPtrTy      (jit.PointerType(treePtrTy)),
      naturalTreeTy     (jit.StructType({TREE, ulonglongTy},    "Natural")),
      naturalTreePtrTy  (jit.PointerType(naturalTreeTy)),
      realTreeTy        (jit.StructType({TREE, realTy},         "Real")),
      realTreePtrTy     (jit.PointerType(realTreeTy)),
      textTreeTy        (jit.StructType({TREE, textTy},         "Text")),
      textTreePtrTy     (jit.PointerType(textTreeTy)),
      nameTreeTy        (jit.StructType({TREE, textTy},         "Name")),
      nameTreePtrTy     (jit.PointerType(nameTreeTy)),
      blockTreeTy       (jit.StructType({TREE1},                "Block")),
      blockTreePtrTy    (jit.PointerType(blockTreeTy)),
      prefixTreeTy      (jit.StructType({TREE2},                "Prefix")),
      prefixTreePtrTy   (jit.PointerType(prefixTreeTy)),
      postfixTreeTy     (jit.StructType({TREE2},                "Postfix")),
      postfixTreePtrTy  (jit.PointerType(postfixTreeTy)),
      infixTreeTy       (jit.StructType({TREE2, textTy},        "Infix")),
      infixTreePtrTy    (jit.PointerType(infixTreeTy)),
      scopeTy           (jit.StructType({TREE2},                "Scope")),
      scopePtrTy        (prefixTreePtrTy),
#undef TREE
#undef TREE1
#undef TREE2

      evalTy            (jit.FunctionType(treePtrTy, {scopePtrTy, treePtrTy})),
      evalFnTy          (jit.PointerType(evalTy))
{
    record(compiler, "Created compiler %p", this);

    // Set optimization level
    jit.SetOptimizationLevel(opts);
}


Compiler::~Compiler()
// ----------------------------------------------------------------------------
//    Destructor deletes the various things we had created
// ----------------------------------------------------------------------------
{
    record(llvm_stats, "LLVM statistics");
    if (RECORDER_TRACE(llvm_stats))
        jit.PrintStatistics();
}


Tree * Compiler::Evaluate(Scope *scope, Tree *source)
// ----------------------------------------------------------------------------
//   Compile the tree, then run the evaluation function
// ----------------------------------------------------------------------------
//   This is the entry point used to compile a top-level XL program.
//   It will process all the declarations in the program and then compile
//   the rest of the code as a function taking no arguments.
{
    record(compiler, "Compiling program %t in scope %t", source, scope);
    if (!source || !scope)
        return nullptr;

    CompilerUnit unit(*this, scope, source);
    eval_fn code = unit.Compile();
    record(compiler, "Compiled %t in scope %t as %p",
           source, scope, (void *) code);

    if (!code)
    {
        Ooops("Error compiling $1", source);
        return xl_error;
    }

    Tree *result = source;
    if (!Opt::emitIR)
        result = code(scope, source);
    return result;
}


Tree * Compiler::TypeCheck(Scope *, Tree *type, Tree *val)
// ----------------------------------------------------------------------------
//   Compile a type check
// ----------------------------------------------------------------------------
{
    return val;
}


JIT::PointerType_p Compiler::TreeMachineType(Tree *tree)
// ----------------------------------------------------------------------------
//    Return the LLVM tree type associated to a given XL expression
// ----------------------------------------------------------------------------
{
    switch(tree->Kind())
    {
    case NATURAL:
        return naturalTreePtrTy;
    case REAL:
        return realTreePtrTy;
    case TEXT:
        return textTreePtrTy;
    case NAME:
        return nameTreePtrTy;
    case INFIX:
        return infixTreePtrTy;
    case PREFIX:
        return prefixTreePtrTy;
    case POSTFIX:
        return postfixTreePtrTy;
    case BLOCK:
        return blockTreePtrTy;
    }
    assert(!"Invalid tree type");
    return treePtrTy;
}

#include "gc.h"
INIT_ALLOCATOR(CompilerTypes);
INIT_ALLOCATOR(CompilerRewriteCandidate);
INIT_ALLOCATOR(CompilerRewriteCalls);

XL_END
