// *****************************************************************************
// compiler-llvm.cpp                                                  XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2011,2014,2017,2019, Christophe de Dinechin <christophe@dinechin.org>
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
#include "compiler-llvm.h"



XL_BEGIN

// ============================================================================
//
//   Define all the LLVM wrappers
//
// ============================================================================

#define UNARY(name)                                                     \
static llvm_value xl_llvm_##name(CompiledUnit &,                        \
                                    llvm_builder bld, llvm_value *args) \
{                                                                       \
    return bld->Create##name(args[0]);                                  \
}

#define BINARY(name)                                                    \
static llvm_value xl_llvm_##name(CompiledUnit &,                        \
                                    llvm_builder bld, llvm_value *args) \
{                                                                       \
    return bld->Create##name(args[0], args[1]);                         \
}

#define SPECIAL(name, arity, code)                                      \
static llvm_value xl_llvm_##name(CompiledUnit &unit,                    \
                                    llvm_builder bld, llvm_value *args) \
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
#define UNARY(name)                     { #name, xl_llvm_##name, 1 },
#define BINARY(name)                    { #name, xl_llvm_##name, 2 },
#define SPECIAL(name, arity, code)      { #name, xl_llvm_##name, arity },
#define ALIAS(from, arity, to)          { #from, xl_llvm_##to, arity },

#include "llvm.tbl"

    // Terminator
    { NULL, NULL, 0 }
};

XL_END
