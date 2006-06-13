// ****************************************************************************
//  xl.semantics.writtenforms.xs    (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Information stored about written forms
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

import SYM = XL.SYMBOLS
import PT = XL.PARSER.TREE
import DCL = XL.SEMANTICS.DECLARATIONS
import FN = XL.SEMANTICS.FUNCTIONS
import GEN = XL.SEMANTICS.TYPES.GENERICS
import GN = XL.SEMANTICS.GENERICS


module XL.SEMANTICS.WRITTEN_FORMS with
// ----------------------------------------------------------------------------
//    Dealing with written forms
// ----------------------------------------------------------------------------

    type written_args_map is map[text, FN.declaration]


    type written_form_data is SYM.rewrite_data with
    // ------------------------------------------------------------------------
    //    Extra information stored about a written form
    // ------------------------------------------------------------------------
        function                : FN.function
        args_map                : written_args_map
        generic_args            : GN.generic_map
    type written_form is access to written_form_data


    type generic_written_form_data is SYM.rewrite_data with
    // ------------------------------------------------------------------------
    //    Extra information stored about a written form
    // ------------------------------------------------------------------------
        generic_type            : GEN.generic_type
        args_map                : written_args_map
        declaration             : DCL.declaration
    type generic_written_form is access to generic_written_form_data


    function EnterWrittenForm(fniface : PT.tree;
                              wrform  : PT.tree) return PT.tree
    procedure EnterWrittenForm(GenDecl : DCL.declaration;
                               gentype : GEN.generic_type
                               wrform  : PT.tree)

