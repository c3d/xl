// ****************************************************************************
//  xl.bytecode.xs                  (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     This file defines the XL "bytecode", a particular form of XL tree
//     where only low-level trees exist. The bytecode is designed to be
//     directly transcodable to a low-level language like C or assembly
// 
//     Bytecode entries are defined by the "opcode" statement
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

module XL.BYTECODE with

    // By convention, we give a special name for bytecode trees
    type bytecode is PT.tree

    // Add an instruction to a bytecode sequence
    procedure Instruction (in out instructions : bytecode;
                           opcode : text;
                           dst : text := "";
                           src1 : text := "";
                           src2 : text := "")

    // Create the bytecode for a particular procedure
    function OpenProcedure (in out parent : bytecode;
                            name : text) return bytecode
    procedure CloseProcedure (in out parent : bytecode; proc : bytecode)
