// ****************************************************************************
//  opcodes.cpp                     (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                               XL project
// ****************************************************************************
//
//   File Description:
//
//    Opcodes are native trees generated as part of compilation/optimization
//    to speed up execution. They represent a step in the evaluation of
//    the code.
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

#include "opcodes.h"
#include "basics.h"
#include "parser.h"
#include "errors.h"
#include "types.h"
#include "runtime.h"
#include "renderer.h"

#ifndef INTERPRETER_ONLY
#include "compiler.h"
#endif // INTERPRETER_ONLY

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


RECORDER(opcodes, 64, "List of opcodes");
void Opcode::Register(Context *context)
// ----------------------------------------------------------------------------
//    If the opcode defines a shape, enter that shape in symbol table
// ----------------------------------------------------------------------------
{
    if (Tree *shape = this->Shape())
    {
        record(opcodes, "Opcode %s for %t", this->OpID(), shape);

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
        record(opcodes, "Opcode %s", this->OpID());
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
    record(opcodes, "Opcode %s is a name", this->OpID());

    context->Define(toDefine, toDefine);
    toDefine->SetInfo<Opcode> (this);
}

XL_END
