#ifndef COMPILER_CHILDREN_H
#define COMPILER_CHILDREN_H
// ****************************************************************************
//  compiler-children.h                                             XLR project
// ****************************************************************************
//
//   File Description:
//
//    Build a clone of a tree, evaluating its children
//    Used when a form didn't match, so we need to try with children
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

struct EvaluateChildren : Action
// ----------------------------------------------------------------------------
//   Build a clone of a tree, evaluating its children
// ----------------------------------------------------------------------------
{
    EvaluateChildren(Context *s): symbols(s)    { assert(s); }
    ~EvaluateChildren()                         {}

    virtual Tree *Do(Tree *what)                { return what; }
    virtual Tree *DoInteger(Integer *what)      { return what; }
    virtual Tree *DoReal(Real *what)            { return what; }
    virtual Tree *DoText(Text *what)            { return what; }
    virtual Tree *DoName(Name *what)            { return what; }
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);

    Tree *        Try(Tree *what);
public:
    Context *   symbols;
};

XL_END

#endif // COMPILER_CHILDREN_H

