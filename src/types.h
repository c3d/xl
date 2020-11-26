#ifndef TYPES_H
#define TYPES_H
// ****************************************************************************
//  types.h                                                          XL project
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

#include "tree.h"
#include "context.h"
#include <map>

XL_BEGIN

class Types;
typedef GCPtr<Types> Types_p;


class Types
// ----------------------------------------------------------------------------
//   Record types information in a given context
// ----------------------------------------------------------------------------
{
    Context_p   context;        // Context for lookups
    tree_map    types;          // Type associated with an expression if any
    Types_p     parent;         // Parent type information if any

public:
    Types(Scope *scope);
    Types(Scope *scope, Types *parent);
    ~Types();
    typedef Tree *value_type;

public:
    // Main entry point: perform type analysis on a whole program
    Tree *      TypeAnalysis(Tree *source);

    // Return the type for an expression
    Tree *      Type(Tree *expr);

public:
    // Debug utilities
    void        Dump();

public:
    GARBAGE_COLLECT(Types);
};

XL_END

RECORDER_DECLARE(types);
RECORDER_DECLARE(types_ids);
RECORDER_DECLARE(types_unifications);
RECORDER_DECLARE(types_calls);
RECORDER_DECLARE(types_boxing);
RECORDER_DECLARE(types_joined);

#endif // TYPES_H
