// ****************************************************************************
//  xl.ui.console.xs                (C) 1992-2006 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//    The console is a simple text-based user interface with
//    input, output and error streams.
// 
// 
// 
// 
// 
// 
// 
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

import IO = XL.TEXT_IO

module XL.UI.CONSOLE with

    StandardInput  : IO.file
    StandardOutput : IO.file
    StandardError  : IO.file

    to Write (...)
    to WriteLn (...)
