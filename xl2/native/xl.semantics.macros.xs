// *****************************************************************************
// xl.semantics.macros.xs                                             XL project
// *****************************************************************************
//
// File description:
//
//     XL Macro System
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
// (C) 2003-2007,2015,2020, Christophe de Dinechin <christophe@dinechin.org>
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
import SYM = XL.SYMBOLS

module XL.SEMANTICS.MACROS with
// ----------------------------------------------------------------------------
//    Implement the macro system
// ----------------------------------------------------------------------------

    include_path : string of text


    type macro_data is SYM.rewrite_data with
    // ------------------------------------------------------------------------
    //    Extra information stored about macro parameters
    // ------------------------------------------------------------------------
        replacement : PT.tree
        kind        : text
    type macro is access to macro_data


    procedure AddPath (path : text)
    procedure ReplacePath (oldPath : text; newPath : text)
    procedure EnterMacro (kind : text; from : PT.tree; to : PT.tree)
    function Preprocess (input : PT.tree) return PT.tree
    function Include (filename : PT.tree) return PT.tree
