// ****************************************************************************
//  xl.array.basic.xs               (C) 1992-2006 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     C-style zero-based arrays (used for the full-fledged implementation)
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

module XL.ARRAY.BASIC with

    generic [type item; size : integer]
    type array written array[size] of item is XL.BYTECODE.array_type

    to Index(A : array; I : integer) return variable array.item written A[I]            is XL.BYTECODE.array_index

    function Range(A : array) return range of integer written A.range is
        return 0..A.size-1

    function Array() return array is
        for I in result.range loop
            result[I] := array.item()

    to Delete (A : array) is
        for I in A.range loop
            any.delete A[I]
