// ****************************************************************************
//  xl.renderer.xs                  (C) 1992-2004 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
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
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

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

    function  Open(format : text := "xl.stylesheet";
                   syntax : text := "xl.syntax") return renderer
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
