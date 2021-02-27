// *****************************************************************************
// compiler-types.cpp                                                 XL project
// *****************************************************************************
//
// File description:
//
//     Representation of machine-level types for the compiler
//
//     This keeps tracks of the boxed representation associated to all
//     type expressions
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2010, Catherine Burvelle <catherine@taodyne.com>
// (C) 2009-2021, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2011-2012, Jérôme Forissier <jerome@taodyne.com>
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

#include "compiler-types.h"
#include "compiler-rewrites.h"
#include "tree.h"
#include "runtime.h"
#include "errors.h"
#include "options.h"
#include "save.h"
#include "cdecls.h"
#include "renderer.h"
#include "builtins.h"

#include <iostream>

XL_BEGIN

// ============================================================================
//
//    Type allocation and unification algorithms (hacked Damas-Hilney-Milner)
//
// ============================================================================

CompilerTypes::CompilerTypes(Scope *scope)
// ----------------------------------------------------------------------------
//   Constructor for top-level type inferences
// ----------------------------------------------------------------------------
    : Types(scope),
      codegen(false)
{
    record(types, "Created CompilerTypes %p for scope %t", this, scope);
}


CompilerTypes::CompilerTypes(Scope *scope, CompilerTypes *parent)
// ----------------------------------------------------------------------------
//   Constructor for "child" type inferences, i.e. done within a parent
// ----------------------------------------------------------------------------
    : Types(scope, parent),
      codegen(false)
{
    record(types, "Created child CompilerTypes %p scope %t", this, scope);
}


CompilerTypes::~CompilerTypes()
// ----------------------------------------------------------------------------
//    Destructor - Nothing to explicitly delete, but useful for debugging
// ----------------------------------------------------------------------------
{
    record(types, "Deleted CompilerTypes %p", this);
}


CompilerTypes *CompilerTypes::LocalTypes()
// ----------------------------------------------------------------------------
//   Factory for local type information
// ----------------------------------------------------------------------------
{
    return new CompilerTypes(context->Symbols(), this);
}


CompilerRewriteCalls *CompilerTypes::NewRewriteCalls()
// ----------------------------------------------------------------------------
//   Return rewrite calls for CompilerTypes
// ----------------------------------------------------------------------------
{
    return new CompilerRewriteCalls(this);
}


Tree *CompilerTypes::TypeAnalysis(Tree *program)
// ----------------------------------------------------------------------------
//   Perform all the steps of type inference on the given program
// ----------------------------------------------------------------------------
{
    Tree *result = Types::TypeAnalysis(program);
    codegen = true;
    return result;
}


Tree *CompilerTypes::CodeGenerationType(Tree *expr)
// ----------------------------------------------------------------------------
//   Make sure that we have the type for the expression at code generation time
// ----------------------------------------------------------------------------
{
    Tree *result = KnownType(expr);
    if (!result)
    {
        Ooops("Internal error: No type for $1 at code generation time", expr);
        return natural_type;
    }
    return result;
}



// ============================================================================
//
//   Boxed type management
//
// ============================================================================

void CompilerTypes::AddBoxedType(Tree *type, JIT::Type_p mtype)
// ----------------------------------------------------------------------------
//   Associate a tree type to a boxed machine type
// ----------------------------------------------------------------------------
///  The tree type could be a named type, e.g. [natural], or data, e.g. [X,Y]
//   The machine type could be naturalTy or StructType({naturalTy, realTy})
{
    Tree *base = BaseType(type);
    record(types_boxing, "In %p add %T boxing %t (%t)",
           this, mtype, type, base);
    assert(!boxed[base] || boxed[base] == mtype);
    boxed[base] = mtype;
}


JIT::Type_p CompilerTypes::BoxedType(Tree *type)
// ----------------------------------------------------------------------------
//   Return the boxed type if there is one
// ----------------------------------------------------------------------------
{
    // Check if we already had this signature
    Tree *base = BaseType(type);
    for (CompilerTypes *ts = this;
         ts;
         ts = (CompilerTypes *) (Types *) ts->parent)
    {
        auto it = ts->boxed.find(base);
        if (it != ts->boxed.end())
        {

            record(types_boxing, "In %p type %T is boxing %t (%t)",
                   ts, it->second, type, base);
            return it->second;
        }
    }
    return nullptr;
}



// ============================================================================
//
//   Debug utilities
//
// ============================================================================

void CompilerTypes::Dump()
// ----------------------------------------------------------------------------
//   Dump the list of types in this
// ----------------------------------------------------------------------------
{
    Types::Dump();

    Save<intptr_t> saveTrace(RECORDER_TRACE(types_boxing), 0);
    std::cout << "\n\nMACHINE TYPES " << (void *) this << ":\n";
    for (auto &b : boxed)
    {
        Tree *type = b.first;
        JIT::Type_p mtype = b.second;
        std::cerr << type;
        JIT::Print("\t= ", mtype);
        std::cerr << "\n";
    }

    std::cout << "\n\nUNIFICATIONS " << (void *) this << ":\n";
    for (auto &t : unifications)
    {
        Tree *type = t.first;
        Tree *base = t.second;
        std::cerr << type << " (" << (void *) type << ")"
                  << "\t= " << base << " (" << (void *) base << ")\n";
    }
}

XL_END


XL::CompilerTypes *xldebug(XL::CompilerTypes *ti)
// ----------------------------------------------------------------------------
//   Dump a type inference
// ----------------------------------------------------------------------------
{
    if (!XL::Allocator<XL::CompilerTypes>::IsAllocated(ti))
    {
        std::cout << "Cowardly refusing to show bad CompilerTypes pointer "
                  << (void *) ti << "\n";
    }
    else
    {
        ti->DumpAll();
    }
    return ti;
}


XL::CompilerTypes *xldebug(XL::CompilerTypes_p ti)
// ----------------------------------------------------------------------------
//   Dump a pointer to compiler types
// ----------------------------------------------------------------------------
{
    return xldebug((XL::CompilerTypes *) ti);
}

XL::Tree *xldebug(XL::Tree *);
XL::Types *xldebug(XL::Types *);
XL::Context *xldebug(XL::Context *);
XL::RewriteCalls *xldebug(XL::RewriteCalls *);
XL::RewriteCandidate *xldebug(XL::RewriteCandidate *);


void *xldebug(uintptr_t address)
// ----------------------------------------------------------------------------
//   Debugger entry point to debug a garbage-collected pointer
// ----------------------------------------------------------------------------
{
    void *ptr = (void *) address;

#define CHECK_ALLOC(T)                                  \
    if (XL::Allocator<XL::T>::IsAllocated(ptr))         \
    {                                                   \
        std::cout << "Pointer " << ptr                  \
                  << " appears to be a " #T "\n";       \
        return xldebug((XL::T *) ptr);                  \
    }

    CHECK_ALLOC(Natural);
    CHECK_ALLOC(Real);
    CHECK_ALLOC(Text);
    CHECK_ALLOC(Name);
    CHECK_ALLOC(Block);
    CHECK_ALLOC(Prefix);
    CHECK_ALLOC(Postfix);
    CHECK_ALLOC(Infix);
    CHECK_ALLOC(Types);
    CHECK_ALLOC(CompilerTypes);
    CHECK_ALLOC(Context);
    CHECK_ALLOC(RewriteCalls);
    CHECK_ALLOC(RewriteCandidate);

    return XL::GarbageCollector::DebugPointer(ptr);
}
