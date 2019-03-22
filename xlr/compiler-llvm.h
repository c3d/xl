#ifndef COMPILER_LLVM_H
#define COMPILER_LLVM_H
// *****************************************************************************
// compiler-llvm.h                                                    XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2003-2004,2006,2010-2011,2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
// (C) 2004, Sébastien Brochet <sebbrochet@sourceforge.net>
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

typedef llvm_value (*llvm_fn) (CompiledUnit &unit,
                               llvm_builder builder,
                               llvm_value *args);

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
