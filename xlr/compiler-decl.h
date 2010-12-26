#ifndef COMPILER_DECL_H
#define COMPILER_DECL_H
// ****************************************************************************
//  compiler-decl.h                                                XLR project
// ****************************************************************************
// 
//   File Description:
// 
//    Actions performed on a given tree to record declarations within
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

struct DeclarationAction : Action
// ----------------------------------------------------------------------------
//   Record data and rewrite declarations in the input tree
// ----------------------------------------------------------------------------
{
    DeclarationAction (Context *c): symbols(c) {}

    virtual Tree *Do(Tree *what);
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);

    void        EnterRewrite(Tree *defined, Tree *definition, Tree *where);

    Context_p symbols;
};

XL_END

#endif // COMPILER_DECL_H

