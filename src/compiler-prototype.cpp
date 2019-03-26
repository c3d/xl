// ****************************************************************************
//  compiler-function.cpp                           XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//
//
//
//
//
//
//
//
//
// ****************************************************************************
//  (C) 2018 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
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
//  (C) 1992-2018 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "compiler-prototype.h"
#include <stdint.h>


RECORDER(compiler_prototype, 64, "Function prototypes");

XL_BEGIN

CompilerPrototype::CompilerPrototype(CompilerUnit &unit,
                                     Tree *form,
                                     Types *types,
                                     FunctionType_p type,
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


Function_p CompilerPrototype::Function()
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
