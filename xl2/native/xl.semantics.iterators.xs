// ****************************************************************************
//  xl.semantics.iterators.xs       (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Implementation of iterators and iterator overloading
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
import FN = XL.SEMANTICS.FUNCTIONS
import TY = XL.SEMANTICS.TYPES
import SYM = XL.SYMBOLS


module XL.SEMANTICS.ITERATORS with
// ----------------------------------------------------------------------------
//    Interface of the semantics of basic C++-style iterators
// ----------------------------------------------------------------------------

    type declaration            is FN.declaration
    type declaration_list       is FN.declaration_list
    type function_type          is FN.function_type
    type any_type               is TY.any_type


    type iterator_data is FN.function_data with
    // ------------------------------------------------------------------------
    //   An iterator is essentially seen as a special function
    // ------------------------------------------------------------------------
        declaration_context     : SYM.symbol_table
    type iterator is access to iterator_data

    function GetIterator (input : PT.tree) return iterator

    function EnterIterator (FnIface : PT.tree) return PT.tree
    function EnterIterator (Names   : PT.tree;
                            FnType  : any_type;
                            Init    : PT.tree;
                            Iface   : PT.tree) return PT.tree
    function InvokeIterator(Input   : PT.tree;
                            Base    : PT.tree;
                            iter    : iterator;
                            Args    : PT.tree_list;
                            ctors   : PT.tree;
                            dtors   : PT.tree) return BC.bytecode
