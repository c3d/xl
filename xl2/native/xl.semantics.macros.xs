// ****************************************************************************
//  xl.semantics.macros.xs          (C) 1992-2004 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     XL Macro System
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
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

import PT = XL.PARSER.TREE
import SYM = XL.SYMBOLS

module XL.SEMANTICS.MACROS with
// ----------------------------------------------------------------------------
//    Implement the macro system
// ----------------------------------------------------------------------------

    type macro_data is SYM.rewrite_data with
    // ------------------------------------------------------------------------
    //    Extra information stored about macro parameters
    // ------------------------------------------------------------------------
        replacement : PT.tree
        kind        : text
    type macro is access to macro_data


    procedure EnterMacro (kind : text; from : PT.tree; to : PT.tree)
    function Preprocess (input : PT.tree) return PT.tree
