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
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

import PT = XL.PARSER.TREE
import BC = XL.BYTECODE
import SYM = XL.SEMANTICS.SYMBOLS


module XL.SEMANTICS with

    type symbol_table is SYM.symbol_table
    context : symbol_table := nil

    // Top-level semantics
    function Semantics (input : PT.tree) return BC.bytecode

    // Automatically generated for 'semantics' statements
    procedure SemanticsPluginsInit
