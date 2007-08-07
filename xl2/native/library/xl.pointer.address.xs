// ****************************************************************************
//  xl.pointer.address.xs           (C) 1992-2007 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Taking the address of objects
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

use XL.POINTER

module XL.POINTER.ADDRESS with

    function Address(in out item : pointer.item) return pointer written &item           is XL.BYTECODE.address_ptr

