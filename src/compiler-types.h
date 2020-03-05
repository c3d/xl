#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H
// *****************************************************************************
// compiler-types.h                                                 XL project
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
// (C) 2009-2011,2014-2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
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
//
//  The type system in XL is based on the shape of trees, for example:
//     type positive_add is matching X+Y when X > 0 and Y > 0
//
//  A value can belong to multiple types. For example, 3+5 belongs to
//  the infix type (since 3+5 is an infix) as well as to positive_add

#include "types.h"
#include "tree.h"
#include "context.h"
#include "llvm-crap.h"
#include "compiler-rewrites.h"
#include <map>


XL_BEGIN

// ============================================================================
//
//   Type definitions
//
// ============================================================================

typedef std::map<Tree_p, JIT::Type_p>    box_map;



// ============================================================================
//
//   Class used to record types in a program in the compiled case
//
// ============================================================================

class CompilerTypes : public Types
// ----------------------------------------------------------------------------
//   Record type information
// ----------------------------------------------------------------------------
{
    box_map     boxed;          // Tree type -> machine type

public:
    CompilerTypes(Scope *scope, Tree *source);
    CompilerTypes(Scope *scope, Tree *source, CompilerTypes *parent);
    virtual ~CompilerTypes();
    typedef Tree *value_type;

public:
    // Machine types management
    void        AddBoxedType(Tree *treeType, JIT::Type_p machineType);
    JIT::Type_p BoxedType(Tree *type);

    // Debug utilities
    void        DumpMachineTypes();

public:
    virtual CompilerRewriteCalls *      NewRewriteCalls() override;
    virtual CompilerRewriteCandidate *  NewRewriteCandidate(Rewrite *,
                                                            Scope *) override;
    virtual CompilerTypes *             DerivedTypesInScope(Scope *,
                                                            Tree *) override;
    virtual CompilerRewriteCalls *      RewriteCallsFor(Tree *what) override;

public:
    GARBAGE_COLLECT(CompilerTypes);
};
typedef GCPtr<CompilerTypes> CompilerTypes_p;

XL_END

XL::CompilerTypes *xldebug(XL::CompilerTypes *ti);

RECORDER_DECLARE(types_boxing);

#endif // COMPILER_TYPES_H
