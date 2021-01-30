#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H
// *****************************************************************************
// compiler-types.h                                                   XL project
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
// (C) 2009-2011,2014-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
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

typedef std::map<Tree_p, JIT::Type_p>    box_map;


// ============================================================================
//
//   Class used to record types in a program
//
// ============================================================================

class CompilerTypes : public Types
// ----------------------------------------------------------------------------
//   Record type information
// ----------------------------------------------------------------------------
{
    box_map     boxed;          // Tree type -> machine type
    bool        codegen;        // Code generation started

public:
    CompilerTypes(Scope *scope);
    ~CompilerTypes();
    typedef Tree *value_type;

public:
    // Main entry point
    Tree *      TypeAnalysis(Tree *source) override;

    // Machine types management
    void        AddBoxedType(Tree *treeType, JIT::Type_p machineType);
    JIT::Type_p BoxedType(Tree *type);

public:
    // Get type, used during code generation (checks that it is already known)
    Tree *      CodeGenerationType(Tree *expr);

    // Debug utilities
    void        Dump() override;

public:
    // Type adjusted overrides
    CompilerRewriteCalls *TreeRewriteCalls(Tree *what, bool recurse = true);
    void                TreeRewriteCalls(Tree *what, CompilerRewriteCalls *rc);


protected:
    CompilerTypes(Scope *scope, CompilerTypes *parent);

    // Create a types structure for local processing
    CompilerTypes *     LocalTypes() override;

    // Create rewrite calls for this class
    CompilerRewriteCalls *NewRewriteCalls() override;

public:
    GARBAGE_COLLECT(CompilerTypes);
};
typedef GCPtr<CompilerTypes> CompilerTypes_p;



// ============================================================================
//
//    Representation of types
//
// ============================================================================

struct TypeInfo : Info
// ----------------------------------------------------------------------------
//    Information recording the type of a given tree
// ----------------------------------------------------------------------------
{
    TypeInfo(Tree *type): type(type) {}
    typedef Tree_p       data_t;
    operator             data_t()  { return type; }
    Tree_p               type;
};



// ============================================================================
//
//   Inline functions
//
// ============================================================================

inline CompilerRewriteCalls *
CompilerTypes::TreeRewriteCalls(Tree *what, bool recurse)
// ----------------------------------------------------------------------------
//   Type-adjusted variant
// ----------------------------------------------------------------------------
{
    return (CompilerRewriteCalls *) Types::TreeRewriteCalls(what, recurse);
}


inline void
CompilerTypes::TreeRewriteCalls(Tree *what, CompilerRewriteCalls *rc)
// ----------------------------------------------------------------------------
//   Type-adjusted variant
// ----------------------------------------------------------------------------
{
    Types::TreeRewriteCalls(what, rc);
}


XL_END

XL::CompilerTypes *xldebug(XL::CompilerTypes *ti);

#endif // COMPILER_TYPES_H
