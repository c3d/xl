#ifndef REWRITES_H
#define REWRITES_H
// *****************************************************************************
// rewrites.h                                                        XL project
// *****************************************************************************
//
// File description:
//
//    Analyze what a rewrite means in a more palatable way
//
//    This is the implementation-indepenant part of the analysis.
//    The compiler portion of it is in compiler-rewrites.h
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

#include "types.h"
#include <recorder/recorder.h>


RECORDER_DECLARE(calls);
RECORDER_DECLARE(bindings);

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


struct RewriteTypeCheck
// ----------------------------------------------------------------------------
//   A dynamic type check to verify if a value has the expected type
// ----------------------------------------------------------------------------
//   For [foo X,Y], the input must be an infix, so when called "ambiguously"
//   as [foo Z], this will check that [Z] hss an infix kind.
{
    RewriteTypeCheck(Tree *value, Tree *type): value(value), type(type) {}
    Tree_p      value;
    Tree_p      type;
};
typedef std::vector<RewriteTypeCheck> RewriteTypeChecks;


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
    void TypeCheck(Tree *value, Tree *type)
    {
        record(calls, "Check if %t has type %t", value, type);
        typechecks.push_back(RewriteTypeCheck(value, type));
    }

    bool Unconditional()
    {
        return typechecks.size() == 0 && conditions.size() == 0;
    }

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
    text                FunctionName()          { return defined_name; }

    void                Dump();

public:
    Infix_p             rewrite;
    Scope_p             scope;
    RewriteBindings     bindings;
    RewriteTypeChecks   typechecks;
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

    // Factory for rewrite candidates - overloaded by compiler version
    virtual
    RewriteCandidate *  Candidate(Infix *rewrite, Scope *scope, Types *types);

    // Types
    Types *             RewriteTypes()          { return types; }

    // Candidates
    size_t              Size()                  { return candidates.size(); }
    RewriteCandidate *  Candidate(unsigned i)   { return candidates[i]; }
    RewriteCandidates & Candidates()            { return candidates; }

private:
    Types_p             types;
    RewriteCandidates   candidates;
public:
    GARBAGE_COLLECT(RewriteCalls);
};
typedef GCPtr<RewriteCalls> RewriteCalls_p;
typedef std::map<Tree_p, RewriteCalls_p> rcall_map;

XL_END

#endif // COMPILER_REWRITES_H
