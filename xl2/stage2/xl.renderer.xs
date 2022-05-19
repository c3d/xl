// *****************************************************************************
// xl.renderer.xs                                                     XL project
// *****************************************************************************
//
// File description:
//
//     Renders an XL tree to (configurable) text form
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
// (C) 2004,2007,2015, Christophe de Dinechin <christophe@dinechin.org>
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
import IO = XL.TEXT_IO

module XL.RENDERER with

    // Mapping of the types
    type priority_table is map[text, integer]
    type rendering_table is map[text, PT.tree]

    // Data actualy used by the renderer
    type renderer_data is record with
         output             : IO.output_file
         infix_priority     : priority_table
         prefix_priority    : priority_table
         postfix_priority   : priority_table
         formats            : rendering_table
         indent             : integer
         self               : text
         left               : PT.tree
         right              : PT.tree
         current_quote      : text
         had_space          : boolean
         had_punctuation    : boolean
         need_separator     : boolean
         statement_priority : integer
         function_priority  : integer
         default_priority   : integer
         priority           : integer
    type renderer is access to renderer_data

    function  Open(format : text := ""; syntax : text := "") return renderer
    procedure Close(toClose : renderer)

    procedure RenderTo(output : renderer; stream : IO.output_file)
    procedure Render(output : renderer; what : PT.tree)

    DefaultRenderer : renderer := nil


module XL.TEXT_IO with
// ----------------------------------------------------------------------------
//    I/O customization
// ----------------------------------------------------------------------------

    procedure write(what : PT.tree)

procedure Debug (tree : PT.tree)
