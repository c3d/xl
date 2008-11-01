// ****************************************************************************
//  xl.pointer.heap.xs              (C) 1992-2007 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
// 
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

module XL.POINTER.HEAP with

    type memory_size is unsigned

    function Allocate(size : memory_size) return pointer is XL.BYTECODE.allocate_memory
    function New(value : pointer.item) return pointer is XL.BYTECODE.new_memory
    to Free (in out memory : pointer) is XL.BYTECODE.free_memory

    generic [type item]
    function Byte_Size(value : item) return memory_size is XL.BYTECODE.byte_size

    generic [type item]
    function Bit_Size(value : item) return memory_size is XL.BYTECODE.bit_size




