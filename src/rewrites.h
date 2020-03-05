#ifndef REWRITES_H
#define REWRITES_H
// *****************************************************************************
// rewrites.h                                                        XL project
// *****************************************************************************
//
// File description:
//
//     Record the transformation of an expression into a sequence of calls
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010-2012,2015-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
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

#include "context.h"
#include "renderer.h"

#include <recorder/recorder.h>


RECORDER_DECLARE(call_types);
RECORDER_DECLARE(argument_bindings);

XL_BEGIN

class Types;
typedef GCPtr<Types> Types_p;

enum BindingStrength { FAILED, POSSIBLE, PERFECT };

struct RewriteBinding
// ----------------------------------------------------------------------------
//   Binding of a given parameter to a value
// ----------------------------------------------------------------------------
//   If [foo X is ...] is invoked as [foo 2], then this records the binding
//   of [X] to [2].
{
    RewriteBinding(Name *name, Tree *value)
        : name(name), value(value) {}
    bool       IsDeferred();
    Name_p     name;
    Tree_p     value;
};
typedef std::vector<RewriteBinding> RewriteBindings;


struct RewriteCondition
// ----------------------------------------------------------------------------
//   A condition for a given rewrite to be valid
// ----------------------------------------------------------------------------
//   For [foo X when X > 0 is ...] being called as [foo 2], this records
//   the condition [X > 0] along with [2].
{
    RewriteCondition(Tree *value, Tree *test): value(value), test(test) {}
    Tree_p      value;
    Tree_p      test;
};
typedef std::vector<RewriteCondition> RewriteConditions;


struct RewriteKind
// ----------------------------------------------------------------------------
//   A kind-based condition for a given rewrite to be valid
// ----------------------------------------------------------------------------
//   For [foo X,Y], the input must be an infix, so when called "ambiguously"
//   as [foo Z], this will check that [Z] hss an infix kind.
{
    RewriteKind(Tree *value, kind test): value(value), test(test) {}
    Tree_p      value;
    kind        test;
};
typedef std::vector<RewriteKind> RewriteKinds;


struct RewriteCandidate
// ----------------------------------------------------------------------------
//    A rewrite candidate for a particular tree pattern
// ----------------------------------------------------------------------------
{
    RewriteCandidate(Infix *rewrite, Scope *scope, Types *types);
    virtual ~RewriteCandidate();

    void Condition(Tree *value, Tree *test)
    {
        conditions.push_back(RewriteCondition(value, test));
    }
    void KindCondition(Tree *value, kind k)
    {
        record(call_types, "Check if %t has kind %u", value, (unsigned) k);
        kinds.push_back(RewriteKind(value, k));
    }

    bool Unconditional() { return kinds.size() == 0 && conditions.size() == 0; }

    // Argument binding
    Tree *              ValueType(Tree *value);
    BindingStrength     Bind(Tree *ref, Tree *what);
    BindingStrength     BindBinary(Tree *form1, Tree *value1,
                                   Tree *form2, Tree *value2);
    bool                Unify(Tree *valueType, Tree *formType,
                              Tree *value, Tree *pattern,
                              bool declaration = false);

    // Code generation
    Tree *              RewritePattern()        { return rewrite->left; }
    Tree *              RewriteBody()           { return rewrite->right; }
    text                FunctionName();

    // Access to the underlying fields
    Types *             Arguments()             { return value_types; }
    Types *             Parameters()            { return binding_types; }

    void                Dump();

public:
    Infix_p             rewrite;
    Scope_p             scope;
    RewriteBindings     bindings;
    RewriteKinds        kinds;
    RewriteConditions   conditions;
    Types_p             value_types;
    Types_p             binding_types;
    Tree_p              type;
    Tree_p              defined;
    text                defined_name;

    GARBAGE_COLLECT(RewriteCandidate);
};
typedef GCPtr<RewriteCandidate> RewriteCandidate_p;
typedef std::vector<RewriteCandidate_p> RewriteCandidates;


struct RewriteCalls
// ----------------------------------------------------------------------------
//   Identify the way to invoke rewrites for a particular pattern
// ----------------------------------------------------------------------------
{
    RewriteCalls(Types *ti);
    virtual ~RewriteCalls();

    Tree *              Check(Scope *scope, Tree *value, Infix *candidate);
    void                Dump();
    size_t              EntriesCount()  { return candidates.size(); }
    virtual
    RewriteCandidate *  Candidate(size_t nth);

public:
    Types_p             types;
    RewriteCandidates   candidates;
    GARBAGE_COLLECT(RewriteCalls);
};
typedef GCPtr<RewriteCalls> RewriteCalls_p;
typedef std::map<Tree_p, RewriteCalls_p> rcall_map;

XL_END

#endif // COMPILER_REWRITES_H
