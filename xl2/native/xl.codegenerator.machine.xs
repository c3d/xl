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
import PT = XL.PARSER.TREE
import BC = XL.BYTECODE


module XL.CODE_GENERATOR.MACHINE with
// ----------------------------------------------------------------------------
//    The interface of the machine-dependent part of the code generator
// ----------------------------------------------------------------------------

    function Name (Name : PT.name_tree;
                   Type : TY.any_type) return PT.name_tree

    function Entry (MName   : PT.tree;
                    RetType : TY.any_type;
                    Parms   : FT.declaration_list) return BC.bytecode
    function EntryPointer (RetType : TY.any_type;
                           Parms   : FT.declaration_list) return PT.name_tree

    type machine_arg is record with
        machine_value   : PT.tree
        is_input        : boolean
        is_output       : boolean
    type machine_args is string of machine_arg

    function FunctionCall (mname    : PT.name_tree;
                           margs    : machine_args) return BC.bytecode
