// *****************************************************************************
// xl.constants.xs                                                    XL project
// *****************************************************************************
//
// File description:
//
//     Interface for the constant evaluation of the XL compiler
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
// (C) 2003-2004,2006,2015,2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2004, Sébastien Brochet <sebbrochet@sourceforge.net>
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

import PT = XL.PARSER.TREE


module XL.CONSTANTS with
// ----------------------------------------------------------------------------
//    Functions used to evaluate if something evalutes as a constant
// ----------------------------------------------------------------------------

    // Return a tree representing the constant value, or nil
    function EvaluateConstant (value : PT.tree) return PT.tree

    // Entering and looking up constant names
    function NamedConstant(name : PT.name_tree) return PT.tree
    procedure EnterNamedConstant(name : PT.name_tree; value : PT.tree)
    function IsTrue (cst : PT.tree) return boolean
    function IsFalse (cst : PT.tree) return boolean
    function IsBoolean (cst : PT.tree) return boolean
    function IsConstant (cst : PT.tree) return boolean
