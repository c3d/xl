// ****************************************************************************
//  xl.text.encoding.ascii.xs       (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//      Basic module describing the ASCII encoding
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

module XL.TEXT_IO.ENCODING.ASCII with

    function to_lower(C : character) return character
    function is_space(C : character) return boolean
    function is_line_break(C : character) return boolean
    function is_digit(C : character) return boolean
    function is_letter(C : character) return boolean
    function is_nul(C : character) return boolean
