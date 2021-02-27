// *****************************************************************************
// xl.semantics.modules.xs                                            XL project
// *****************************************************************************
//
// File description:
//
//     Implementation of modules and import statement
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
// (C) 2003-2004,2007,2009-2010,2015-2017,2020, Christophe de Dinechin <christophe@dinechin.org>
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

import SYM = XL.SYMBOLS

module XL.SEMANTICS.MODULES with

    modules_path : string of text
    procedure AddPath (path : text)
    procedure ReplacePath (oldPath : text; newPath : text)
    function AddBuiltins(Input : PT.tree) return PT.tree
    function InXlBuiltinsModule() return boolean
    function GetSymbols (value : PT.tree) return SYM.symbol_table
