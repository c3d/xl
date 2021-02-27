// *****************************************************************************
// action.cpp                                                         XL project
// *****************************************************************************
//
// File description:
//
//    A simple recursive action on trees
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
// (C) 2010,2015-2017,2019-2021, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
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

#include "action.h"
#include "tree.h"

XL_BEGIN

Tree *Action::Do(Natural *what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do((Tree *) what);
}


Tree *Action::Do(Real *what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do((Tree *) what);
}


Tree *Action::Do(Text *what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do((Tree *) what);
}


Tree *Action::Do(Name *what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do((Tree *) what);
}


Tree *Action::Do(Block *what)
// ----------------------------------------------------------------------------
//    Default is to firm perform action on block's child, then on self
// ----------------------------------------------------------------------------
{
    what->child = what->child->Do(this);
    return Do((Tree *) what);
}


Tree *Action::Do(Prefix *what)
// ----------------------------------------------------------------------------
//   Default is to run the action on the left, then on right
// ----------------------------------------------------------------------------
{
    what->left = what->left->Do(this);
    what->right = what->right->Do(this);
    return Do((Tree *) what);
}


Tree *Action::Do(Postfix *what)
// ----------------------------------------------------------------------------
//   Default is to run the action on the right, then on the left
// ----------------------------------------------------------------------------
{
    what->right = what->right->Do(this);
    what->left = what->left->Do(this);
    return Do((Tree *) what);
}


Tree *Action::Do(Infix *what)
// ----------------------------------------------------------------------------
//   Default is to run the action on children first
// ----------------------------------------------------------------------------
{
    what->left = what->left->Do(this);
    what->right = what->right->Do(this);
    return Do((Tree *) what);
}

XL_END
