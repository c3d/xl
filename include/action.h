#ifndef ACTION_H
#define ACTION_H
// *****************************************************************************
// action.h                                                           XL project
// *****************************************************************************
//
// File description:
//
//    Actions that can be performed on trees
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
// (C) 2010,2014-2017,2019-2021, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
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

#include "base.h"

XL_BEGIN

struct Tree;                                    // Base tree
struct Natural;                                 // Natural: 0, 3, 8
struct Real;                                    // Real: 3.2, 1.6e4
struct Text;                                    // Text: "ABC"
struct Name;                                    // Name / symbol: ABC, ++-
struct Prefix;                                  // Prefix: sin X
struct Postfix;                                 // Postfix: 3!
struct Infix;                                   // Infix: A+B, newline
struct Block;                                   // Block: (A), {A}

struct Action
// ----------------------------------------------------------------------------
//   An operation we do recursively on trees
// ----------------------------------------------------------------------------
{
    Action () {}
    virtual ~Action() {}

    typedef Tree *value_type;

    virtual Tree *Do (Tree *what) = 0;

    // Specialization for the canonical nodes, default is to run them
    virtual Tree *Do(Natural *what);
    virtual Tree *Do(Real *what);
    virtual Tree *Do(Text *what);
    virtual Tree *Do(Name *what);
    virtual Tree *Do(Prefix *what);
    virtual Tree *Do(Postfix *what);
    virtual Tree *Do(Infix *what);
    virtual Tree *Do(Block *what);
};

XL_END

#endif // ACTION_H
