// ****************************************************************************
//  xl.plugin.common.xs             (C) 1992-2004 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//     Common data, types for plugins
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
// * Credits    : Sebastien <sebbrochet@users.sourceforge.net> (initial version)
// ****************************************************************************
import PT = XL.PARSER.TREE

module XL.PLUGIN.COMMON with
    function  GetDefineInfo (input : text) return PT.Tree
    procedure SetDefineInfo (input : text;
                             DefineInfo : PT.Tree)