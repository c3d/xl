#ifndef COMPILER_ARGS_H
#define COMPILER_ARGS_H
// ****************************************************************************
//  compiler-args.h                                                 XL project
// ****************************************************************************
//
//   File Description:
//
//    Check if a tree matches the form on the left of a rewrite
//
//
//
//
//
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "compiler.h"
#include "renderer.h"

#include <recorder/recorder.h>

XL_BEGIN

class Types;
class CompilerFunction;
typedef GCPtr<Types> Types_p;
RECORDER_DECLARE(calltypes);

enum BindingStrength { FAILED, POSSIBLE, PERFECT };

struct RewriteBinding
// ----------------------------------------------------------------------------
//   Binding of a given parameter to a value
// ----------------------------------------------------------------------------
//   If [foo X is ...] is invoked as [foo 2], then this records the binding
//   of [X] to [2].
{
    RewriteBinding(Name *name, Tree *value)
        : name(name), value(value), closure(NULL) {}
    bool       IsDeferred();
    Value_p    Closure(CompilerFunction &function);
    Name_p     name;
    Tree_p     value;
    Value_p    closure;
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
//    A rewrite candidate for a particular tree form
// ----------------------------------------------------------------------------
{
    RewriteCandidate(Infix *rewrite, Scope *scope, Types *types);
    void Condition(Tree *value, Tree *test)
    {
        conditions.push_back(RewriteCondition(value, test));
    }
    void KindCondition(Tree *value, kind k)
    {
        record(calltypes, "Check if %t has kind %u", value, (unsigned) k);
        kinds.push_back(RewriteKind(value, k));
    }

    bool Unconditional() { return kinds.size() == 0 && conditions.size() == 0; }

    BindingStrength     Bind(Tree *ref, Tree *what);
    BindingStrength     BindBinary(Tree *form1, Tree *value1,
                                   Tree *form2, Tree *value2);
    bool                Unify(Tree *valueType, Tree *formType,
                              Tree *value, Tree *form,
                              bool declaration = false);

    Infix_p             rewrite;
    Scope_p             scope;
    RewriteBindings     bindings;
    RewriteKinds        kinds;
    RewriteConditions   conditions;
    Types_p             vtypes; // Types for values (enclosing)
    Types_p             btypes; // Types for bindings (local)
    Context_p           context;
    Tree_p              type;
};
typedef std::vector<RewriteCandidate> RewriteCandidates;


struct RewriteCalls
// ----------------------------------------------------------------------------
//   Identify the way to invoke rewrites for a particular form
// ----------------------------------------------------------------------------
{
    RewriteCalls(Types *ti);

    Tree *              Check(Scope *scope, Tree *value, Infix *candidate);

public:
    Types_p             types;
    RewriteCandidates   candidates;
    GARBAGE_COLLECT(RewriteCalls);
};
typedef GCPtr<RewriteCalls> RewriteCalls_p;
typedef std::map<Tree_p, RewriteCalls_p> rcall_map;

XL_END

#endif // COMPILER_ARGS_H
