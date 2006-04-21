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
import RT = XL.SEMANTICS.TYPES.RECORDS
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

    // Expression code
    function MakeExpression(computation : BC.bytecode;
                            mname       : PT.name_tree) return BC.bytecode
    procedure SplitExpression(in out value  : PT.tree;
                              in out code   : PT.tree)

    // Interface for function declarations
    function Entry (f : FN.function) return BC.bytecode
    function EntryPointer (f : FT.function_type) return PT.name_tree
    function FunctionBody(f     : FN.function;
                          Iface : PT.tree;
                          Body  : PT.tree) return PT.tree

    // Interface for function calls
    type machine_args is string of PT.tree
    function FunctionCall (toCall   : FN.function;
                           margs    : machine_args;
                           ctors    : PT.tree;
                           dtors    : PT.tree) return BC.bytecode
    function RecordFunctionCall (Record : PT.tree;
                                 toCall   : FN.function;
                                 margs    : machine_args;
                                 ctors    : PT.tree;
                                 dtors    : PT.tree) return BC.bytecode

    // Interface for record types
    function DeclareRecord(r : RT.record_type) return PT.name_tree
    function IndexRecord(Record  : PT.tree;
                         MField  : PT.tree;
                         MType   : PT.tree;
                         Type    : TY.any_type) return BC.bytecode

    // Declare other types
    function DeclareType(tp   : TY.any_type;
                         Name : PT.name_tree) return PT.name_tree
