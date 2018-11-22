// ****************************************************************************
//  xl.translator.xs                (C) 1992-2003 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//     The basic XL translator
//
//
//
//
//
//
//
//
// ****************************************************************************
// This program is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

import PT = XL.PARSER.TREE

module XL.TRANSLATOR with

    procedure Compile(input : PT.tree)

    verbose : boolean := false

    semantics_translations_init : text
    initializations             : text
    terminations                : text
