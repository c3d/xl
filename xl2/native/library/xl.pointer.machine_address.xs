// ****************************************************************************
//  xl.pointer.machine_address.xs   (C) 1992-2006 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Implementation of machine addresses
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

module XL.POINTER.MACHINE_ADDRESS with

    function Pointer(addr : integer) return pointer                                    is XL.BYTECODE.int_to_ptr
    function Pointer(addr : unsigned) return pointer                                   is XL.BYTECODE.uint_to_ptr
    function Integer(ptr : pointer) return integer                                     is XL.BYTECODE.ptr_to_int
    function Unsigned(ptr : pointer) return unsigned                                   is XL.BYTECODE.ptr_to_uint

