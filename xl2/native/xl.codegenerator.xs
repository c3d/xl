// ****************************************************************************
//  xl.codegenerator.xs             (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     The XL code generator. It currently generates C code
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

import BC = XL.BYTECODE
import IO = XL.TEXT_IO

module XL.CODE_GENERATOR with

    procedure Transcode (input : BC.bytecode; file : IO.output_file)
    semantics_translations_init : text
    initializations             : text
    terminations                : text
    debug                       : boolean := false
    verbose                     : boolean := false
