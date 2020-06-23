// *****************************************************************************
// opcodes.cpp                                                        XL project
// *****************************************************************************
//
// File description:
//
//    Opcodes are native trees generated as part of compilation/optimization
//    to speed up execution. They represent a step in the evaluation of
//    the code.
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2010-2011, Catherine Burvelle <catherine@taodyne.com>
// (C) 2003-2004,2006-2007,2009-2020, Christophe de Dinechin <christophe@dinechin.org>
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

#include "opcodes.h"

#include "basics.h"
#include "errors.h"
#include "parser.h"
#include "renderer.h"
#include "runtime.h"

#ifndef INTERPRETER_ONLY
#include "compiler.h"
#endif // INTERPRETER_ONLY


// ============================================================================
//
//   List of recorders
//
// ============================================================================

RECORDER(opcodes, 64, "List of opcodes");


#include <typeinfo>

XL_BEGIN

// List of registered opcodes
Opcode::Opcodes *Opcode::opcodes = nullptr;


void Opcode::Enter(Context *context)
// ----------------------------------------------------------------------------
//   Enter all the opcodes declared using the macros
// ----------------------------------------------------------------------------
{
    for (Opcodes::iterator o = opcodes->begin(); o != opcodes->end(); o++)
    {
        Opcode *opcode = *o;
        opcode->Register(context);
    }
}


Opcode * Opcode::Find(Tree *self, text name)
// ----------------------------------------------------------------------------
//   Find an opcode that matches the name if there is  one
// ----------------------------------------------------------------------------
{
    for (Opcodes::iterator o = opcodes->begin(); o != opcodes->end(); o++)
    {
        Opcode *opcode = *o;
        if (opcode->OpID() == name)
            return opcode;
    }
    Ooops("Invalid builtin name in $1", self);
    return nullptr;
}


void Opcode::Register(Context *context)
// ----------------------------------------------------------------------------
//    If the opcode defines a shape, enter that shape in symbol table
// ----------------------------------------------------------------------------
{
    if (Tree *shape = this->Shape())
    {
        record(opcodes, "Opcode %+s for %t", this->OpID(), shape);

        Save<TreePosition> savePos(Tree::NOWHERE, Tree::BUILTIN);
        static Name_p builtinName = new Name("builtin");
        Infix *decl = new Infix("is", shape,
                                new Prefix(builtinName,
                                           new Name(this->OpID())));
        context->Enter(decl);
        decl->right->SetInfo<Opcode> (this);
    }
    else
    {
        record(opcodes, "Opcode %+s", this->OpID());
    }
}



// ============================================================================
//
//    Name opcodes
//
// ============================================================================

void NameOpcode::Register(Context *context)
// ----------------------------------------------------------------------------
//   For name rewrites, create the name, assign to variable, enter it
// ----------------------------------------------------------------------------
{
    record(opcodes, "Opcode %+s is name %t", this->OpID(), toDefine);

    context->Define(toDefine, toDefine);
    toDefine->SetInfo<NameOpcode> (this);
}



// ============================================================================
//
//    TypeCheck opcodes
//
// ============================================================================

void TypeCheckOpcode::Register(Context *context)
// ----------------------------------------------------------------------------
//   For name rewrites, create the name, assign to variable, enter it
// ----------------------------------------------------------------------------
{
    record(opcodes, "Opcode %+s is a type", this->OpID());
    context->Define(toDefine, toDefine);
    toDefine->SetInfo<TypeCheckOpcode> (this);
}

XL_END
