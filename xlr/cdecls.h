#ifndef COMPILER_CDECLS_H
#define COMPILER_CDECLS_H
// ****************************************************************************
//  cdecls.h                                                        XLR project
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

#include "tree.h"

XL_BEGIN

struct CDeclaration : Info
// ----------------------------------------------------------------------------
//   A class that processes C declarations
// ----------------------------------------------------------------------------
{
    CDeclaration();
    typedef Tree *value_type;
    
    Infix * Declaration(Tree *input);
    Tree *  TypeAndName(Tree *input, Tree_p &type, Name_p &name, uint &mods);
    Tree *  Parameters(Block *input);
    Tree *  Type(Tree *input, uint &mods);
    Tree *  PointerType(Postfix *input);
    Tree *  ArrayType(Tree *returned);
    Name *  NamedType(Name *input, uint &mods);
    Name *  BaroqueTypeMods(Name *first, Name *second, uint &mods);
    Name *  Anonymous();

    enum { SHORT = 1, LONG = 2, UNSIGNED = 4, SIGNED = 8 };

public:
    Name_p      name;
    Tree_p      returnType;
    Infix_p     rewrite;
    uint        parameters;
};

XL_END

#endif // COMPILER_CDECLS_H
