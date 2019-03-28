#ifndef COMPILER_PARMS_H
#define COMPILER_PARMS_H
// *****************************************************************************
// compiler-parms.h                                                   XL project
// *****************************************************************************
//
// File description:
//
//    Actions collecting parameters on the left of a rewrite
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010-2011,2015-2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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

#include "compiler.h"

XL_BEGIN

class CompilerFunction;

struct Parameter
// ----------------------------------------------------------------------------
//   Internal representation of a parameter
// ----------------------------------------------------------------------------
{
    Parameter(Name *name, Type_p type = 0) : name(name), type(type) {}
    Name_p name;
    Type_p type;
};
typedef std::vector<Parameter> Parameters;


struct ParameterList
// ----------------------------------------------------------------------------
//   Collect parameters on the left of a rewrite
// ----------------------------------------------------------------------------
{
    typedef bool value_type;

public:
    ParameterList(CompilerFunction &function)
        : function(function), defined(nullptr), returned(nullptr) {}

public:
    bool                EnterName(Name *what, Type_p declaredType = nullptr);

    bool                Do(Integer *what);
    bool                Do(Real *what);
    bool                Do(Text *what);
    bool                Do(Name *what);
    bool                Do(Prefix *what);
    bool                Do(Postfix *what);
    bool                Do(Infix *what);
    bool                Do(Block *what);

public:
    CompilerFunction &  function;       // Current function
    Tree_p              defined;        // Tree being defined, 'sin' in sin X
    text                name;           // Name being given to the LLVM function
    Parameters          parameters;     // Parameters and their order
    Type_p              returned;       // Returned type if specified
};

XL_END

#endif // COMPILER_PARMS_H
