#ifndef COMPILER_PROTOTYPE_H
#define COMPILER_PROTOTYPE_H
// *****************************************************************************
// compiler-prototype.h                                               XL project
// *****************************************************************************
//
// File description:
//
//     A function prototype generated in the CompilerUnit
//
//     Prototypes are generated for references to external functions
//     They are also used as a base class for CompilerFunction
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2010-2011,2015-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
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
#include "compiler-rewrites.h"
#include "compiler-unit.h"
#include "compiler-types.h"


XL_BEGIN

class CompilerPrototype
// ----------------------------------------------------------------------------
//    A function generated in a compile unit
// ----------------------------------------------------------------------------
{
protected:
    CompilerUnit &      unit;       // The unit we compile from
    Tree_p              pattern;    // Interface for this function
    Types_p             types;      // Type system for this function
    JIT::Function_p     function;   // The LLVM function we are building

    friend class CompilerExpression;

public:
    // Constructors for the top-level functions
    CompilerPrototype(CompilerUnit &unit,
                      Tree *pattern,
                      CompilerTypes *types,
                      JIT::FunctionType_p ftype,
                      text name);
    CompilerPrototype(CompilerPrototype &caller, RewriteCandidate *rc);
    ~CompilerPrototype();

public:
    JIT::Function_p     Function();
    virtual bool        IsInterfaceOnly();
    Scope *             FunctionScope();
    Context *           FunctionContext();
};

XL_END

RECORDER_DECLARE(compiler_prototype);

#endif // COMPILER_FUNCTION_H
