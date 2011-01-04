#ifndef COMPILER_LLVM_H
#define COMPILER_LLVM_H
// ****************************************************************************
//  compiler-llvm.h                                                 XLR project
// ****************************************************************************
//
//   File Description:
//
//     The interface between the XLR compiler and the LLVM backend
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

typedef llvm_value (*llvm_fn) (llvm_builder builder, llvm_value *args);

struct CompilerLLVMTableEntry
// ----------------------------------------------------------------------------
//   An entry describing an LLVM primitive
// ----------------------------------------------------------------------------
{
    kstring     name;
    llvm_fn     function;
    uint        arity;
};

extern CompilerLLVMTableEntry CompilerLLVMTable[];

XL_END

#endif // COMPILER_LLVM_H
