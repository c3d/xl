#ifndef COMPILER_REWRITES_H
#define COMPILER_REWRITES_H
// *****************************************************************************
// compiler-rewrites.h                                                XL project
// *****************************************************************************
//
// File description:
//
//    Check if a tree matches the pattern on the left of a rewrite
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2010-2012,2015-2020, Christophe de Dinechin <christophe@dinechin.org>
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

#include "rewrites.h"
#include "compiler.h"
#include "renderer.h"

#include <recorder/recorder.h>


XL_BEGIN

class CompilerTypes;

struct CompilerRewriteCandidate : RewriteCandidate
// ----------------------------------------------------------------------------
//    A rewrite candidate for a particular tree pattern
// ----------------------------------------------------------------------------
{
    CompilerRewriteCandidate(Infix *rw, Scope *scope, CompilerTypes *types);

    // REVIIST - Hacks from older implemnetation, remove
    Tree *              ValueType(Tree *value);

    // Code generation
    JIT::Function_p     Prototype(JIT &jit);
    JIT::FunctionType_p FunctionType(JIT &jit);
    JIT::Signature      RewriteSignature();
    JIT::Type_p         RewriteType();
    void                RewriteType(JIT::Type_p type);

    // Access types in Compiler form
    CompilerTypes *     ValueTypes()
    {
        return (CompilerTypes *) (Types *) value_types;
    }
    CompilerTypes *     BindingTypes()
    {
        return (CompilerTypes *) (Types *) binding_types;
    }

public:
    GARBAGE_COLLECT(CompilerRewriteCandidate);
};
typedef GCPtr<CompilerRewriteCandidate> CompilerRewriteCandidate_p;
typedef std::vector<CompilerRewriteCandidate_p> CompilerRewriteCandidates;


struct CompilerRewriteCalls : RewriteCalls
// ----------------------------------------------------------------------------
//   Identify the way to invoke rewrites for a particular pattern
// ----------------------------------------------------------------------------
{
    CompilerRewriteCalls(CompilerTypes *ti);

    // Factory for rewrite candidates - overloaded by compiler version
    virtual
    CompilerRewriteCandidate *Candidate(Infix *rewrite,
                                        Scope *scope,
                                        Types *types) override;

    // Type-adjusted functions
    CompilerTypes *RewriteTypes()
    {
        return (CompilerTypes *) RewriteCalls::RewriteTypes();
    }
    CompilerRewriteCandidate *Candidate(unsigned i)
    {
        return (CompilerRewriteCandidate *) RewriteCalls::Candidate(i);
    }

public:
    GARBAGE_COLLECT(CompilerRewriteCalls);
};
typedef GCPtr<CompilerRewriteCalls> CompilerRewriteCalls_p;

XL_END

#endif // COMPILER_REWRITES_H
