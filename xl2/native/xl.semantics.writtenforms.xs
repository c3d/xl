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

import PT = XL.PARSER.TREE
import DCL = XL.SEMANTICS.DECLARATIONS


module XL.SEMANTICS.WRITTEN_FORMS with
// ----------------------------------------------------------------------------
//    Dealing with written forms
// ----------------------------------------------------------------------------

    type written_form_data
    type written_form is access to written_form_data
    type written_form_data is record with
    // ------------------------------------------------------------------------
    //    Information stored about a written form
    // ------------------------------------------------------------------------
        form                    : PT.tree
        declaration             : DCL.declaration
