#ifndef COMPILER_CDECLS_H
#define COMPILER_CDECLS_H
// ****************************************************************************
//  compiler-cdecls.h                                               Tao project
// ****************************************************************************
// 
//   File Description:
// 
//     Processing and transforming C declarations into normal XL
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

#include "tree.h"

XL_BEGIN

struct ProcessCDeclaration
// ----------------------------------------------------------------------------
//   A class that processes C declarations
// ----------------------------------------------------------------------------
{
    ProcessCDeclaration();
    typedef Tree *value_type;
    
    Tree * Declaration(Tree *input);
    Tree * TypeAndName(Tree *input, Tree_p &type, Name_p &name, uint &mods);
    Tree * Parameters(Block *input);
    Tree * Type(Tree *input, uint &mods);
    Tree * PointerType(Postfix *input);
    Tree * ArrayType(Tree *returned);
    Name * NamedType(Name *input, uint &mods);
    Name * BaroqueTypeMods(Name *first, Name *second, uint &mods);
    Name * Anonymous();

    enum { SHORT = 1, LONG = 2, UNSIGNED = 4, SIGNED = 8 };

public:
    Name_p      name;
    Tree_p      returnType;
    uint        parameters;
};

XL_END

#endif COMPILER_CDECLS_H
