// *****************************************************************************
// xl.semantics.overload.xs                                           XL project
// *****************************************************************************
//
// File description:
//
//     Interface for overload resolution
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
// (C) 2003-2008,2015, Christophe de Dinechin <christophe@dinechin.org>
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
import BC = XL.BYTECODE

module XL.SEMANTICS.OVERLOAD with
// ----------------------------------------------------------------------------
//   Interface for overload resolution
// ----------------------------------------------------------------------------

    function Resolve(NameTree  : PT.tree;
                     Args      : PT.tree;
                     Input     : PT.tree;
                     forceCall : boolean) return BC.bytecode
    function IsFunction(T : PT.tree) return boolean
    procedure ArgsTreeToList(Args : PT.tree; in out List : PT.tree_list)
    function ResolveOverload(FunName     : PT.tree;
                             overloadSet : PT.tree;
                             toType      : PT.tree) return BC.bytecode
