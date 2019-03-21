#ifndef COMPILER_PARM_H
#define COMPILER_PARM_H
// *****************************************************************************
// parms.h                                                            XL project
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
// 
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010-2011,2019, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can r redistribute it and/or modify
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

struct Parameter
// ----------------------------------------------------------------------------
//   Internal representation of a parameter
// ----------------------------------------------------------------------------
{
    Parameter(Name *name, llvm_type type = 0) : name(name), type(type) {}
    Name_p              name;
    llvm_type           type;
};
typedef std::vector<Parameter>      Parameters;


struct ParameterList
// ----------------------------------------------------------------------------
//   Collect parameters on the left of a rewrite
// ----------------------------------------------------------------------------
{
    typedef bool value_type;
    
public:
    ParameterList(CompiledUnit *unit)
        : unit(unit), defined(NULL), returned(NULL) {}

public:
    bool EnterName(Name *what, bool untyped);

    bool DoInteger(Integer *what);
    bool DoReal(Real *what);
    bool DoText(Text *what);
    bool DoName(Name *what);
    bool DoPrefix(Prefix *what);
    bool DoPostfix(Postfix *what);
    bool DoInfix(Infix *what);
    bool DoBlock(Block *what);

public:
    CompiledUnit *  unit;         // Current compilation unit
    Tree_p          defined;      // Tree being defined, e.g. 'sin' in 'sin X'
    text            name;         // Name being given to the LLVM function
    Parameters      parameters;   // Parameters and their order
    llvm_type       returned;     // Returned type if specified
};

XL_END

#endif // COMPILER_PARM_H

