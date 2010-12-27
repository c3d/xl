#ifndef COMPILER_ACTION_H
#define COMPILER_ACTION_H
// ****************************************************************************
//  compiler-action.h                                               XLR project
// ****************************************************************************
// 
//   File Description:
// 
//    Actions performed on a given tree to compile it
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

struct CompileAction : Action
// ----------------------------------------------------------------------------
//   Compute the input tree in the given compiled unit
// ----------------------------------------------------------------------------
{
    CompileAction (Context *s, CompiledUnit &, bool nullIfBad, bool keepAlt);

    virtual Tree *Do(Tree *what);
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);

    // Build code selecting among rewrites in current context
    Tree *         Rewrites(Tree *what);

    Context_p     context;
    CompiledUnit &unit;
    bool          nullIfBad;
    bool          keepAlternatives;
};


XL_END

#endif //COMPILER_ACTION_H

