// ****************************************************************************
//  types.cpp                                                        XL project
// ****************************************************************************
//
//   File Description:
//
//    Common type analysis for XL programs
//
//    The Types class holds the implementation-independant portions of the type
//    analysis for XL, i.e. the part that is the same for the interpreter and
//    the compiler.
//
//
//
//
// ****************************************************************************
//   (C) 2020 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
// ****************************************************************************
//   This file is part of XL.
//
//   XL is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   XL is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with XL.  If not, see <https://www.gnu.org/licenses/>.
// ****************************************************************************

#include "types.h"
#include "errors.h"


XL_BEGIN

Types::Types(Scope *scope)
// ----------------------------------------------------------------------------
//   Constructor for top-level type inferences
// ----------------------------------------------------------------------------
    : context(new Context(scope)), parent(nullptr)
{
    record(types, "Created Types %p for scope %t", this, scope);
}


Types::Types(Scope *scope, Types *parent)
// ----------------------------------------------------------------------------
//   Constructor for "child" type inferences, i.e. done within a parent
// ----------------------------------------------------------------------------
    : context(new Context(scope)), types(), parent(parent)
{
    context->CreateScope();
    scope = context->Symbols();
    record(types, "Created child Types %p scope %t", this, scope);
}


Types::~Types()
// ----------------------------------------------------------------------------
//    Destructor - Nothing to explicitly delete, but useful for debugging
// ----------------------------------------------------------------------------
{
    record(types, "Deleted Types %p", this);
}


Tree *Types::TypeAnalysis(Tree *program)
// ----------------------------------------------------------------------------
//   Perform all the steps of type inference on the given program
// ----------------------------------------------------------------------------
{
    // Once this is done, record all type information for the program
    record(types, "Type analysis for %t in %p", program, this);
    Tree *result = Type(program);
    record(types, "Type for %t in %p is %t", program, this, result);

    // Dump debug information if approriate
    if (RECORDER_TRACE(types))
        Dump();

    return result;
}


Tree *Types::Type(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the type associated with a given expression
// ----------------------------------------------------------------------------
{
    // First check if we already computed the type for this expression

    // Check type, make sure we don't destroy a nullptr type
    Tree *type = expr;
    record(types, "In %p created type for %t is %t", this, expr, type);
    return type;
}



// ============================================================================
//
//   Debug utilities
//
// ============================================================================

void Types::Dump()
// ----------------------------------------------------------------------------
//   Dump the list of types in this
// ----------------------------------------------------------------------------
{
    for (Types *types = this; types; types = types->parent)
    {
        std::cout << "TYPES " << (void *) types << ":\n";
        for (auto &t : types->types)
        {
            Tree *value = t.first;
            Tree *type = t.second;
            std::cout << "\t" << ShortTreeForm(value)
                      << "\t: " << type
                      << "\n";
        }
    }
}


XL_END


XL::Types *xldebug(XL::Types *ti)
// ----------------------------------------------------------------------------
//   Dump a type inference
// ----------------------------------------------------------------------------
{
    if (!XL::Allocator<XL::Types>::IsAllocated(ti))
    {
        std::cout << "Cowardly refusing to show bad Types pointer "
                  << (void *) ti << "\n";
    }
    else
    {
        ti->Dump();
    }
    return ti;
}


XL::Types *xldebug(XL::Types_p ti)
// ----------------------------------------------------------------------------
//   Dump a pointer to compiler types
// ----------------------------------------------------------------------------
{
    return xldebug((XL::Types *) ti);
}


RECORDER(types,                 64, "Type analysis");
RECORDER(types_ids,             64, "Assigned type identifiers");
RECORDER(types_unifications,    64, "Type unifications");
RECORDER(types_calls,           64, "Type deductions in rewrites (calls)");
RECORDER(types_boxing,          64, "Machine type boxing and unboxing");
RECORDER(types_joined,          64, "Joined types");
