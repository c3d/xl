// ****************************************************************************
//  xl.semantics.xs                 (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//      This is the semantics phase of the XL compiler
//      It takes an XL tree as input, and returns an XL tree as output
//      after checking that the semantics are correct.
// 
//      The phase also performs what other compilers might call "expansion",
//      that is the generation of lower-level trees from high-level
//      constructs. The low-level constructs are found in XL.BYTECODE
//      and can be used to directly generate code.
// 
// 
// ****************************************************************************
// This document is released under the GNU Genral Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

import PT = XL.PARSER.TREE
import BC = XL.BYTECODE

module XL.SEMANTICS with

    type symbol_table_data
    type symbol_table is access to symbol_table_data

    function Semantics (input : PT.tree) return BC.bytecode
    function Semantics (input : PT.tree;
                        context : symbol_table) return BC.bytecode

