// ****************************************************************************
//  xl.bytecode.xs                  (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     This file defines the XL "bytecode", a particular form of XL tree
//     used as an intermediate language by the XL compiler. XL bytecode is
//     relatively machine independent, and intended to be usable as an
//     actual bytecode one day. The compiler performs most of its
//     optimizations on this bytecode, and then hands the bytecode over
//     to the machine-description layer, which turns it into real assembly.
//
//     Bytecode may sometimes coexist with a higher level description
//     In that case, it appears in the high-level as @ [bytecode],
//     where [bytecode] is an arbitrary tree.
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

