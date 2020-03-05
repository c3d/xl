// *****************************************************************************
// compiler-types.cpp                                               XL project
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
// This software is licensed under the GNU General Public License v3
// (C) 2010, Catherine Burvelle <catherine@taodyne.com>
// (C) 2009-2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2011-2012, Jérôme Forissier <jerome@taodyne.com>
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

#include "compiler-types.h"
#include "compiler-rewrites.h"
#include "tree.h"
#include "runtime.h"
#include "errors.h"
#include "options.h"
#include "save.h"
#include "cdecls.h"
#include "renderer.h"
#include "basics.h"

#include <iostream>

XL_BEGIN

// ============================================================================
//
//    Type allocation and unification algorithms (hacked Damas-Hilney-Milner)
//
// ============================================================================

CompilerTypes::CompilerTypes(Scope *scope, Tree *source)
// ----------------------------------------------------------------------------
//   Constructor for top-level type inferences
// ----------------------------------------------------------------------------
    : Types(scope, source)
{
    record(types,
           "Created CompilerTypes %p for %t in scope %t", this, source, scope);
}


CompilerTypes::CompilerTypes(Scope *scope, Tree *source, CompilerTypes *parent)
// ----------------------------------------------------------------------------
//   Constructor for "child" type inferences, i.e. done within a parent
// ----------------------------------------------------------------------------
    : Types(scope, source, parent),
      boxed(parent->boxed)
{
    record(types, "Created child CompilerTypes %p from %p for %t in scope %t",
           this, parent, source, scope);
}


CompilerTypes::~CompilerTypes()
// ----------------------------------------------------------------------------
//    Destructor - Nothing to explicitly delete, but useful for debugging
// ----------------------------------------------------------------------------
{
    record(types, "Deleted CompilerTypes %p", this);
}



// ============================================================================
//
//   Virtual function overriden in the compiler case
//
// ============================================================================

CompilerRewriteCalls *CompilerTypes::NewRewriteCalls()
// ----------------------------------------------------------------------------
//   Create rewrite calls for compiled types
// ----------------------------------------------------------------------------
{
    return new CompilerRewriteCalls(this);
}


CompilerRewriteCandidate *CompilerTypes::NewRewriteCandidate(Rewrite *cand,
                                                             Scope *scope)
// ----------------------------------------------------------------------------
//   Create a rewrite candidate for compiled types
// ----------------------------------------------------------------------------
{
    return new CompilerRewriteCandidate(cand, scope, this);
}


CompilerTypes *CompilerTypes::DerivedTypesInScope(Scope *scope, Tree *source)
// ----------------------------------------------------------------------------
//    Create types derived from this within given parameter scope
// ----------------------------------------------------------------------------
{
    return new CompilerTypes(scope, source, this);
}


CompilerRewriteCalls *CompilerTypes::RewriteCallsFor(Tree *what)
// ----------------------------------------------------------------------------
//   Check if we have rewrite calls for this specific expression
// ----------------------------------------------------------------------------
{
    auto it = rcalls.find(what);
    RewriteCalls *result = it != rcalls.end() ? it->second : nullptr;
    record(types_calls, "In compiled %p calls for %t are %p (%u entries)",
           this, what, result, result ? result->EntriesCount() : 0);

    // In a CompilerTypes, we know they are all CompilerRewriteCalls *
    return (CompilerRewriteCalls *) result;
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
///  The tree type could be a named type, e.g. [integer], or data, e.g. [X,Y]
//   The machine type could be integerTy or StructType({integerTy, realTy})
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
    auto it = boxed.find(base);
    JIT::Type_p mtype = (it != boxed.end()) ? it->second : nullptr;
    record(types_boxing, "In %p type %T is boxing %t (%t)",
           this, mtype, type, base);
    return mtype;
}



// ============================================================================
//
//   Debug utilities
//
// ============================================================================

void CompilerTypes::DumpMachineTypes()
// ----------------------------------------------------------------------------
//   Dump the list of machine types in this
// ----------------------------------------------------------------------------
{
    uint i = 0;
    Save<intptr_t> saveTrace(RECORDER_TRACE(types_boxing), 0);

    std::cout << "MACHINE TYPES " << (void *) this << ":\n";
    for (auto &b : boxed)
    {
        Tree *type = b.first;
        JIT::Type_p mtype = b.second;
        std::cout << "#" << ++i
                  << "\t" << type;
        JIT::Print("\t= ", mtype);
        std::cout << "\n";
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
        ti->DumpRewriteCalls();
        ti->DumpUnifications();
        ti->DumpTypes();
        ti->DumpMachineTypes();
    }
    return ti;
}
XL::CompilerTypes *xldebug(XL::CompilerTypes_p ti)      { return xldebug((XL::CompilerTypes *) ti); }

XL::Tree *xldebug(XL::Tree *);
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

    CHECK_ALLOC(Integer);
    CHECK_ALLOC(Real);
    CHECK_ALLOC(Text);
    CHECK_ALLOC(Name);
    CHECK_ALLOC(Block);
    CHECK_ALLOC(Prefix);
    CHECK_ALLOC(Postfix);
    CHECK_ALLOC(Infix);
    CHECK_ALLOC(CompilerTypes);
    CHECK_ALLOC(Context);
    CHECK_ALLOC(RewriteCalls);
    CHECK_ALLOC(RewriteCandidate);

    return XL::GarbageCollector::DebugPointer(ptr);
}

RECORDER(types_boxing,          64, "Machine type boxing and unboxing");
