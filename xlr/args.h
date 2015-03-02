#ifndef COMPILER_ARG_H
#define COMPILER_ARG_H
// ****************************************************************************
//  args.h                                                          XLR project
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

XL_BEGIN

struct Types;
typedef GCPtr<Types> Types_p;


struct RewriteBinding
// ----------------------------------------------------------------------------
//   Structure recording the binding of a given parameter to a value
// ----------------------------------------------------------------------------
{
    RewriteBinding(Name *name, Tree *value)
        : name(name), value(value), closure(NULL) {}
    bool       IsDeferred();
    llvm_value Closure(CompiledUnit *unit);
    Name_p     name;
    Tree_p     value;
    llvm_value closure;
};
typedef std::vector<RewriteBinding> RewriteBindings;


struct RewriteCondition
// ----------------------------------------------------------------------------
//   Structure recording a condition for a given rewrite to be valid
// ----------------------------------------------------------------------------
{
    RewriteCondition(Tree *value, Tree *test): value(value), test(test) {}
    Tree_p      value;
    Tree_p      test;
};
typedef std::vector<RewriteCondition> RewriteConditions;


struct RewriteCandidate
// ----------------------------------------------------------------------------
//    A rewrite candidate for a particular tree form
// ----------------------------------------------------------------------------
{
    RewriteCandidate(Infix *rewrite)
        : rewrite(rewrite), bindings(), type(NULL), types(NULL) {}
    void Condition(Tree *value, Tree *test)
    {
        conditions.push_back(RewriteCondition(value, test));
    }

    Infix_p             rewrite;
    RewriteBindings     bindings;
    RewriteConditions   conditions;
    Tree_p              type;
    Types_p             types;
};
typedef std::vector<RewriteCandidate> RewriteCandidates;


struct RewriteCalls
// ----------------------------------------------------------------------------
//   Identify the way to invoke rewrites for a particular form
// ----------------------------------------------------------------------------
{
    RewriteCalls(Types *ti): types(ti), candidates() {}

    enum BindingStrength { FAILED, POSSIBLE, PERFECT };

    Tree *              Check(Prefix *scope, Tree *value, Infix *candidate);
    BindingStrength     Bind(Context *context,
                             Tree *ref, Tree *what, RewriteCandidate &rc);
    BindingStrength     BindBinary(Context *context,
                                   Tree *form1, Tree *value1,
                                   Tree *form2, Tree *value2,
                                   RewriteCandidate &rc);

public:
    Types *             types;
    RewriteCandidates   candidates;
    GARBAGE_COLLECT(RewriteCalls);
};
typedef GCPtr<RewriteCalls> RewriteCalls_p;
typedef std::map<Tree_p, RewriteCalls_p> rcall_map;

XL_END

#endif // COMPILER_ARG_H

