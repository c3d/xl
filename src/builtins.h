#ifndef BUILTINS_H
#define BUILTINS_H
// *****************************************************************************
// builtins.h                                                         XL project
// *****************************************************************************
//
// File description:
//
//    Description of builtin entities in XL, i.e. what follows "builtins"
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
// (C) 2020-2021, Christophe de Dinechin <christophe@dinechin.org>
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

#define NAME(Name)              extern Name_p xl_##Name;
#define TYPE(Name, Body)        extern Name_p Name##_type;

#define UNARY(Name, ReturnType, ArgType, Body)
#define BINARY(Name, ReturnType, LeftType, RightType, Body)

namespace XL
{
#include "builtins.tbl"
}

#endif // BUILTINS_H
