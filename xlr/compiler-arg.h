#ifndef COMPILER_ARG_H
#define COMPILER_ARG_H
// ****************************************************************************
//  compiler-arg.h                                                 XLR project
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
#include "compiler-action.h"

XL_BEGIN

struct RewriteBinding
// ----------------------------------------------------------------------------
//   Structure recording the binding of a given parameter to a value
// ----------------------------------------------------------------------------
{
    RewriteBinding(Name *name, Tree *value): name(name), value(value) {}
    Name_p      name;
    Tree_p      value;
};

typedef std::vector<RewriteBinding> RewriteBindings;


struct RewriteCandidate
// ----------------------------------------------------------------------------
//    A rewrite candidate for a particular tree form
// ----------------------------------------------------------------------------
{
    RewriteCandidate(Rewrite *rewrite): rewrite(rewrite), bindings() {}

    Rewrite_p           rewrite;
    RewriteBindings     bindings;
};

typedef std::vector<RewriteCandidate> RewriteCandidates;


struct IdentifyCandidates
// ----------------------------------------------------------------------------
//   Identify the candidates for a given form
// ----------------------------------------------------------------------------
{
    IdentifyCandidates() {}

    Tree *operator() (Context *context, Tree *value, Rewrite *candidate);

public:
    RewriteCandidates   candidates;
};


struct ArgumentMatch
// ----------------------------------------------------------------------------
//   Check if a tree matches the form of the left of a rewrite
// ----------------------------------------------------------------------------
{
    ArgumentMatch (Tree *t,
                   Context *s, Context *l, Context *r,
                   CompileAction *comp, bool data):
        symbols(s), locals(l), rewrite(r),
        test(t), defined(NULL), compile(comp), unit(comp->unit), data(data) {}

    // Action callbacks
    Tree *Do(Tree *what);
    Tree *DoInteger(Integer *what);
    Tree *DoReal(Real *what);
    Tree *DoText(Text *what);
    Tree *DoName(Name *what);
    Tree *DoPrefix(Prefix *what);
    Tree *DoPostfix(Postfix *what);
    Tree *DoInfix(Infix *what);
    Tree *DoBlock(Block *what);

    // Compile a tree
    Tree *         Compile(Tree *source);
    Tree *         CompileValue(Tree *source);
    Tree *         CompileClosure(Tree *source);

public:
    Context_p      symbols;     // Context in which we evaluate values
    Context_p      locals;      // Context where we declare arguments
    Context_p      rewrite;     // Context in which the rewrite was declared
    Tree_p         test;        // Tree we test
    Tree_p         defined;     // Tree beind defined, e.g. 'sin' in 'sin X'
    CompileAction *compile;     // Action in which we are compiling
    CompiledUnit  &unit;        // JIT compiler compilation unit
    bool           data;        // Is a data form
};

XL_END

#endif // COMPILER_ARG_H

