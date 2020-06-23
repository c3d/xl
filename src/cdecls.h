#ifndef COMPILER_CDECLS_H
#define COMPILER_CDECLS_H
// *****************************************************************************
// cdecls.h                                                           XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2011,2015-2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2011-2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

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
