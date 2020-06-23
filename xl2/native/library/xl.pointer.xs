// *****************************************************************************
// xl.pointer.xs                                                      XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2007-2008,2015, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

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
