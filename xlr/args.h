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
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "compiler.h"

XL_BEGIN

struct TypeInference;
typedef GCPtr<TypeInference> TypeInference_p;


struct RewriteBinding
// ----------------------------------------------------------------------------
//   Structure recording the binding of a given parameter to a value
// ----------------------------------------------------------------------------
{
    RewriteBinding(Name *name, Tree *value)
        : name(name), value(value), type(NULL), closure(NULL) {}
    bool IsDeferred();
    Name_p     name;
    Tree_p     value;
    Tree_p     type;
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
    RewriteCandidate(Rewrite *rewrite)
        : rewrite(rewrite), bindings(), type(NULL), types(NULL) {}

    Rewrite_p           rewrite;
    RewriteBindings     bindings;
    RewriteConditions   conditions;
    Tree_p              type;
    TypeInference_p     types;
};
typedef std::vector<RewriteCandidate> RewriteCandidates;


struct RewriteCalls
// ----------------------------------------------------------------------------
//   Identify the way to invoke rewrites for a particular form
// ----------------------------------------------------------------------------
{
    RewriteCalls(TypeInference *ti): inference(ti), candidates() {}

    enum BindingStrength { FAILED, POSSIBLE, PERFECT };

    Tree *operator() (Context *context, Tree *value, Rewrite *candidate);
    BindingStrength     Bind(Context *context,
                             Tree *ref, Tree *what, RewriteCandidate &rc);

public:
    TypeInference *     inference;
    RewriteCandidates   candidates;

    GARBAGE_COLLECT(RewriteCalls);
};
typedef GCPtr<RewriteCalls> RewriteCalls_p;
typedef std::map<Tree_p, RewriteCalls_p> rcall_map;

XL_END

#endif // COMPILER_ARG_H

