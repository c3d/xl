// ****************************************************************************
//  xl.codegenerator.machine.xs     (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//      Machine-dependent part of the code generator
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

import TY = XL.SEMANTICS.TYPES
import FT = XL.SEMANTICS.TYPES.FUNCTIONS
import FN = XL.SEMANTICS.FUNCTIONS
import PT = XL.PARSER.TREE
import BC = XL.BYTECODE


module XL.CODE_GENERATOR.MACHINE with
// ----------------------------------------------------------------------------
//    The interface of the machine-dependent part of the code generator
// ----------------------------------------------------------------------------

    // Computing machine names ("mangled" names in C++ parlance)
    function Name (Name : PT.name_tree;
                   Type : TY.any_type) return PT.name_tree

    // Interface for function declarations
    function Entry (f : FN.function) return BC.bytecode
    function EntryPointer (f : FT.function_type) return PT.name_tree
    function FunctionBody(f     : FN.function;
                          Iface : PT.tree;
                          Body  : PT.tree) return PT.tree

    // Interface for function calls                   
    type machine_args is string of PT.tree
    procedure EvaluateArgument(in out arg  : PT.tree;
                               in out code : PT.tree)
    function FunctionCall (toCall   : FN.function;
                           margs    : machine_args) return BC.bytecode
