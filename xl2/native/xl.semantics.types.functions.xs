// ****************************************************************************
//  xl.semantics.types.functions.xs (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//      Description of function types
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
import DCL = XL.SEMANTICS.DECLARATIONS
import SYM = XL.SYMBOLS


module XL.SEMANTICS.TYPES.FUNCTIONS with
// ----------------------------------------------------------------------------
//    Representation of function types
// ----------------------------------------------------------------------------

    type any_type_data      is TY.any_type_data
    type any_type           is TY.any_type
    type declaration        is DCL.declaration
    type declaration_list   is string of declaration


    type function_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //    Information in a function signature
    // ------------------------------------------------------------------------
    //    The return type is used as base type, since this is what the
    //    value returned by the function will be

        parameters      : declaration_list
        result_decl     : declaration
        symbols         : SYM.symbol_table
        outputs_count   : integer
        inputs_count    : integer
        variadic        : boolean
        iterator        : boolean

    type function_type is access to function_type_data


    // Create a function type
    function NewFnType(Source     : PT.tree;
                       Parms      : PT.tree;
                       ReturnType : PT.tree) return any_type
    function NewIteratorType(Source     : PT.tree;
                             Parms      : PT.tree) return any_type
        
    function EnterFnType (Source  : PT.tree;
                          Parms   : PT.tree;
                          Ret     : PT.tree) return PT.tree
