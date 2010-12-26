#ifndef COMPILER_PARM_H
#define COMPILER_PARM_H
// ****************************************************************************
//  compiler-parm.h                                                XLR project
// ****************************************************************************
// 
//   File Description:
// 
//    Actions collecting parameters on the left of a rewrite
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

struct ParameterMatch : Action
// ----------------------------------------------------------------------------
//   Collect parameters on the left of a rewrite
// ----------------------------------------------------------------------------
{
    ParameterMatch (Context *s): symbols(s), defined(NULL) {}

    virtual Tree *Do(Tree *what);
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);

    Context_p symbols;          // Context in which we test
    Tree_p    defined;          // Tree beind defined, e.g. 'sin' in 'sin X'
    TreeList  order;            // Record order of parameters
};

XL_END

#endif // COMPILER_PARM_H

