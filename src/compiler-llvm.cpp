// ****************************************************************************
//  compiler-llvm.cpp                                            ELIOT project
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
#include "compiler-llvm.h"
#include "unit.h"



ELIOT_BEGIN

// ============================================================================
// 
//   Define all the LLVM wrappers
// 
// ============================================================================

#define UNARY(name)                                                     \
static llvm_value eliot_llvm_##name(CompiledUnit &,                     \
                                 llvm_builder bld, llvm_value *args)    \
{                                                                       \
    return bld->Create##name(args[0]);
}

#define BINARY(name)                                                    \
static llvm_value eliot_llvm_##name(CompiledUnit &,                     \
                                 llvm_builder bld, llvm_value *args)    \
{                                                                       \
    return bld->Create##name(args[0], args[1]);
}

#define SPECIAL(name, arity, code)                                      \
static llvm_value eliot_llvm_##name(CompiledUnit &unit,                 \
                                 llvm_builder bld, llvm_value *args)    \
{                                                                       \
    Compiler &compiler = *unit.compiler;                                \
    llvm::LLVMContext &llvm = unit.llvm;                                \
    (void) compiler;                                                    \
    (void) llvm;                                                        \
    code                                                                \
}

#define ALIAS(from, arity, to)

#include "llvm.tbl"


CompilerLLVMTableEntry CompilerLLVMTable[] =
// ----------------------------------------------------------------------------
//   A table initialized with the various LLVM entry points
// ----------------------------------------------------------------------------
{
#define UNARY(name)                     { #name, eliot_llvm_##name, 1 },
#define BINARY(name)                    { #name, eliot_llvm_##name, 2 },
#define SPECIAL(name, arity, code)      { #name, eliot_llvm_##name, arity },
#define ALIAS(from, arity, to)          { #from, eliot_llvm_##to, arity },
    
#include "llvm.tbl"

    // Terminator
    { NULL, NULL, 0 }
};

ELIOT_END
