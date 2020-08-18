// *****************************************************************************
// compiler-rewrites.cpp                                              XL project
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
// (C) 2003-2004,2006,2010-2020, Christophe de Dinechin <christophe@dinechin.org>
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

#include "compiler-rewrites.h"
#include "compiler-unit.h"
#include "compiler-function.h"
#include "compiler-types.h"
#include "compiler.h"
#include "save.h"
#include "errors.h"
#include "renderer.h"
#include "main.h"
#include "basics.h"


XL_BEGIN

CompilerRewriteCandidate::CompilerRewriteCandidate(Rewrite *rewrite,
                                                   Scope *scope,
                                                   CompilerTypes *types)
// ----------------------------------------------------------------------------
//   Create a rewrite candidate within the given types
// ----------------------------------------------------------------------------
    : RewriteCandidate(rewrite, scope, types)
{}


Tree *CompilerRewriteCandidate::ValueType(Tree *value)
// ----------------------------------------------------------------------------
//   Return the value type for this value, and associated calls
// ----------------------------------------------------------------------------
{
    Tree *vtype = value_types->Type(value);
    if (vtype && false)
    {
        while (value)
        {
            if (RewriteCalls *calls = ValueTypes()->TreeRewriteCalls(value))
                BindingTypes()->TreeRewriteCalls(value, calls);
            if (Block *block = value->AsBlock())
                value = block->child;
            else
                value = nullptr;
        }
    }
    return vtype;
}


JIT::Function_p CompilerRewriteCandidate::Prototype(JIT &jit)
// ----------------------------------------------------------------------------
//   Build the prototype for the rewrite function
// ----------------------------------------------------------------------------
{
    JIT::FunctionType_p fty = FunctionType(jit);
    text fname = FunctionName();
    JIT::Function_p function = jit.Function(fty, fname);
    return function;
}


JIT::FunctionType_p CompilerRewriteCandidate::FunctionType(JIT &jit)
// ----------------------------------------------------------------------------
//   Build the signature type for the function
// ----------------------------------------------------------------------------
{
    JIT::Signature signature = RewriteSignature();
    JIT::Type_p retTy = RewriteType();
    return jit.FunctionType(retTy, signature);
}


JIT::Signature CompilerRewriteCandidate::RewriteSignature()
// ----------------------------------------------------------------------------
//   Build the signature for the rewrite
// ----------------------------------------------------------------------------
{
    JIT::Signature signature;
    for (RewriteBinding &binding : bindings)
    {
        Tree *valueType = ValueType(binding.value);
        assert(valueType && "Type for bound value is required during codegen");
        JIT::Type_p valueTy = ValueTypes()->BoxedType(valueType);
        assert(valueTy && "Machine type for bound value should exist");
        signature.push_back(valueTy);
    }
    return signature;
}


JIT::Type_p CompilerRewriteCandidate::RewriteType()
// ----------------------------------------------------------------------------
//   Boxed type for the rewrite
// ----------------------------------------------------------------------------
{
    JIT::Type_p ty = BindingTypes()->BoxedType(type);
    return ty;
}


void CompilerRewriteCandidate::RewriteType(JIT::Type_p ty)
// ----------------------------------------------------------------------------
//   Set the boxed type for the rewrite
// ----------------------------------------------------------------------------
{
    BindingTypes()->AddBoxedType(type, ty);
}



// ============================================================================
//
//   Rewrite Calls
//
// ============================================================================

CompilerRewriteCalls::CompilerRewriteCalls(CompilerTypes *types)
// ----------------------------------------------------------------------------
//   Create a new type context to evaluate the calls for a rewrite
// ----------------------------------------------------------------------------
    : RewriteCalls(types)
{}


CompilerRewriteCandidate *CompilerRewriteCalls::Candidate(Rewrite *rewrite,
                                                          Scope *scope,
                                                          Types *types)
// ----------------------------------------------------------------------------
//   Factory for rewrite candidates
// ----------------------------------------------------------------------------
{
    return new CompilerRewriteCandidate(rewrite, scope, (CompilerTypes *)types);
}



XL_END


XL::CompilerRewriteCalls *xldebug(XL::CompilerRewriteCalls *rc)
// ----------------------------------------------------------------------------
//   Debug rewrite calls
// ----------------------------------------------------------------------------
{
    if (!XL::Allocator<XL::CompilerRewriteCalls>::IsAllocated(rc))
        std::cout << "Cowardly refusing to show bad CompilerRewriteCalls "
                  << (void *) rc << "\n";
    else
        rc->Dump();
    return rc;

}


XL::CompilerRewriteCalls *xldebug(XL::CompilerRewriteCalls_p rc)
// ----------------------------------------------------------------------------
//   Debug the GCPtr version
// ----------------------------------------------------------------------------
{
    return xldebug((XL::CompilerRewriteCalls *) rc);
}


XL::CompilerRewriteCandidate *xldebug(XL::CompilerRewriteCandidate *rc)
// ----------------------------------------------------------------------------
//   Debug rewrite calls
// ----------------------------------------------------------------------------
{
    if (!XL::Allocator<XL::CompilerRewriteCandidate>::IsAllocated(rc))
        std::cout << "Cowardly refusing to show bad CompilerRewriteCandidate "
                  << (void *) rc << "\n";
    else
        rc->Dump();
    return rc;

}


XL::CompilerRewriteCandidate *xldebug(XL::CompilerRewriteCandidate_p rc)
// ----------------------------------------------------------------------------
//   Debug the GCPtr version
// ----------------------------------------------------------------------------
{
    return xldebug((XL::CompilerRewriteCandidate *) rc);
}
