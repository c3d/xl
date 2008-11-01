// ****************************************************************************
//  xl.pointer.xs                   (C) 1992-2007 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Implementation of basic pointer functionality
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

module XL.POINTER with

    generic [type item]
    type pointer written pointer to item is XL.BYTECODE.pointer_type

    function Dereference (P : pointer) return variable pointer.item written *P          is XL.BYTECODE.deref_ptr
    function Pointer() return pointer                                                   is XL.BYTECODE.zero_ptr
    to Copy(out Tgt : pointer; Src : pointer) written Tgt := Src                        is XL.BYTECODE.copy_ptr
    to Zero(out Tgt : pointer) written Tgt := nil                                       is XL.BYTECODE.zero_ptr
    function Nil() return pointer written nil                                           is XL.BYTECODE.zero_ptr

    function IsNull(Tgt : pointer) return boolean written Tgt = nil                     is XL.BYTECODE.is_null
    function IsNotNull(Tgt : pointer) return boolean written Tgt <> nil                 is XL.BYTECODE.is_not_null
    function Equal(X, Y : pointer) return boolean               written X=Y                     is XL.BYTECODE.eq_ptr
    function Different(X, Y : pointer) return boolean           written X<>Y                    is XL.BYTECODE.ne_ptr
