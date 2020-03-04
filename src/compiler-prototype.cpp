// *****************************************************************************
// compiler-prototype.cpp                                             XL project
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
// This software is licensed under the GNU General Public License v3
// (C) 2010,2015-2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
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

#include "compiler-prototype.h"
#include <stdint.h>


RECORDER(compiler_prototype, 64, "Function prototypes");

XL_BEGIN

CompilerPrototype::CompilerPrototype(CompilerUnit &unit,
                                     Tree *form,
                                     Types *types,
                                     JIT::FunctionType_p type,
                                     text name)
// ----------------------------------------------------------------------------
//   Create new compiler prototype (e.g. for a C function)
// ----------------------------------------------------------------------------
    : unit(unit),
      form(form),
      types(types),
      function(unit.jit.Function(type, name))
{
    record(compiler_prototype, "Created prototype %p for %t as %v",
           this, form, function);
}


CompilerPrototype::CompilerPrototype(CompilerPrototype &caller,
                                     RewriteCandidate *rc)
// ----------------------------------------------------------------------------
//    Create a new compiler prototype for rewrites
// ----------------------------------------------------------------------------
    : unit(caller.unit),
      form(rc->RewriteForm()),
      types(rc->binding_types),
      function(rc->Prototype(unit.jit))
{
    record(compiler_prototype, "Created rewrite %p for %t as %v",
           this, form, function);
}


CompilerPrototype::~CompilerPrototype()
// ----------------------------------------------------------------------------
//   Delete compiler function
// ----------------------------------------------------------------------------
{
    record(compiler_unit, "Deleted function %p for %t", this, form);
}


JIT::Function_p CompilerPrototype::Function()
// ----------------------------------------------------------------------------
//   The LLVM function associated with the function
// ----------------------------------------------------------------------------
{
    return function;
}


bool CompilerPrototype::IsInterfaceOnly()
// ----------------------------------------------------------------------------
//   Function prototypes only provide an interface, no implementation
// ----------------------------------------------------------------------------
{
    return true;
}


Scope *CompilerPrototype::FunctionScope()
// ----------------------------------------------------------------------------
//   The declaration scope associated with the function
// ----------------------------------------------------------------------------
{
    return types->TypesScope();
}


Context *CompilerPrototype::FunctionContext()
// ----------------------------------------------------------------------------
//   The declaration context for the function
// ----------------------------------------------------------------------------
{
    return types->TypesContext();
}

XL_END
