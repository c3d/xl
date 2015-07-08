// ****************************************************************************
//  xl.semantics.functions.xs       (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Implementation of functions and function overloading
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

import DCL = XL.SEMANTICS.DECLARATIONS
import FT = XL.SEMANTICS.TYPES.FUNCTIONS
import TY = XL.SEMANTICS.TYPES
import SYM = XL.SYMBOLS


module XL.SEMANTICS.FUNCTIONS with
// ----------------------------------------------------------------------------
//    Interface of the semantics of basic C++-style functions
// ----------------------------------------------------------------------------

    type declaration            is FT.declaration
    type declaration_list       is FT.declaration_list
    type function_type          is FT.function_type
    type any_type               is TY.any_type 


    type function_data is DCL.declaration_data with
    // ------------------------------------------------------------------------
    //   The function holds the parameters and return type
    // ------------------------------------------------------------------------
        machine_interface       : PT.tree
        preconditions           : PT.tree_list
        postconditions          : PT.tree_list
        symbols                 : SYM.symbol_table // For locals
        result_machine_name     : PT.name_tree
    type function is access to function_data

    function GetFunction (input : PT.tree) return function
    procedure SetFunction (input : PT.tree; fn : function)

    function EnterFunction (FnIface : PT.tree) return PT.tree
    function EnterFunction (Names   : PT.tree;
                            FnType  : any_type;
                            Init    : PT.tree;
                            Iface   : PT.tree) return PT.tree
    function EnterBuiltinFunction(input : PT.tree;
                                  Interface : PT.tree;
                                  BuiltinName : PT.tree) return PT.tree
    function IsConstructor (F : function) return boolean
