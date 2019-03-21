#ifndef COMPILER_GC_H
#define COMPILER_GC_H
// *****************************************************************************
// compiler-gc.h                                                      XL project
// *****************************************************************************
//
// File description:
//
//     Information connecting the XLR/LLVM compiler to the garbage collector
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

struct CompilerGarbageCollectionListener : TypeAllocator::Listener
// ----------------------------------------------------------------------------
//   Listen to the garbage collection to put away LLVM data structures
// ----------------------------------------------------------------------------
{
    CompilerGarbageCollectionListener(Compiler *compiler)
        : compiler(compiler) {}

    virtual void BeginCollection();
    virtual bool CanDelete (void *obj);
    virtual void EndCollection();

    Compiler *compiler;
};


struct CompilerInfo : Info
// ----------------------------------------------------------------------------
//   Information about compiler-related data structures
// ----------------------------------------------------------------------------
{
    CompilerInfo(Tree *tree): tree(tree), function(0), closure(0) {}
    ~CompilerInfo();
    Tree *                      tree;
    llvm::Function *            function;
    llvm::Function *            closure;

    // We must mark builtins in a special way, see bug #991
    bool        IsBuiltin()     { return function && function == closure; }
};

XL_END

#endif // COMPILER_GC_H
