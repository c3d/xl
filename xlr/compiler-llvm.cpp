// ****************************************************************************
//  compiler-llvm.cpp                                               XLR project
// ****************************************************************************
//
//   File Description:
//
//    The interface between the compiler and LLVM
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
#include "compiler-llvm.h"
#include <llvm/Support/IRBuilder.h>



XL_BEGIN

// ============================================================================
// 
//   Define all the LLVM wrappers
// 
// ============================================================================

#define UNARY(name)                                                     \
static llvm_value xl_llvm_##name(llvm_builder bld, llvm_value *args)    \
{                                                                       \
    return bld->Create##name(args[0]);                                  \
}

#define BINARY(name)                                                    \
static llvm_value xl_llvm_##name(llvm_builder bld, llvm_value *args)    \
{                                                                       \
    return bld->Create##name(args[0], args[1]);                         \
}
#include "llvm.tbl"


CompilerLLVMTableEntry CompilerLLVMTable[] =
// ----------------------------------------------------------------------------
//   A table initialized with the various LLVM entry points
// ----------------------------------------------------------------------------
{
#define UNARY(name)             { #name, xl_llvm_##name, 1 },
#define BINARY(name)            { #name, xl_llvm_##name, 2 },
#include "llvm.tbl"

    // Terminator
    { NULL, NULL, 0 }
};

XL_END
