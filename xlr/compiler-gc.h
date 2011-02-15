#ifndef COMPILER_GC_H
#define COMPILER_GC_H
// ****************************************************************************
//  compiler-gc.h                                                   XLR project
// ****************************************************************************
// 
//   File Description:
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
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

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
    CompilerInfo(Tree *tree): tree(tree), global(0), function(0), closure(0) {}
    ~CompilerInfo();
    Tree *                      tree;
    llvm::GlobalValue *         global;
    llvm::Function *            function;
    llvm::Function *            closure;
};

XL_END

#endif // COMPILER_GC_H

