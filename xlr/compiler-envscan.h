#ifndef COMPILER_ENVSCAN_H
#define COMPILER_ENVSCAN_H
// ****************************************************************************
//  compiler-envscan.h                                             XLR project
// ****************************************************************************
// 
//   File Description:
// 
//    Collect variables in a tree that are imported from environment
//    (used to build closures)
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

struct EnvironmentScan
// ----------------------------------------------------------------------------
//   Collect variables in the tree that are imported from environment
// ----------------------------------------------------------------------------
{
    typedef std::map<Tree_p, Context_p> capture_table;

public:
    EnvironmentScan (Context *s): symbols(s) {}

    Tree *Do(Tree *what);
    Tree *DoInteger(Integer *what);
    Tree *DoReal(Real *what);
    Tree *DoText(Text *what);
    Tree *DoName(Name *what);
    Tree *DoPrefix(Prefix *what);
    Tree *DoPostfix(Postfix *what);
    Tree *DoInfix(Infix *what);
    Tree *DoBlock(Block *what);

public:
    Context_p           symbols;        // Context in which we test
    capture_table       captured;       // Captured symbols
};

XL_END

#endif // COMPILER_ENVSCAN_H

