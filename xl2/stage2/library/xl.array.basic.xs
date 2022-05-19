// *****************************************************************************
// xl.array.basic.xs                                                  XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2003-2004,2006-2008,2015, Christophe de Dinechin <christophe@dinechin.org>
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

module XL.ARRAY.BASIC with

    generic [type item; size : integer]
    type array written array[size] of item is XL.BYTECODE.array_type

    to Index(A : array; I : integer) return variable array.item written A[I]            is XL.BYTECODE.array_index

    function Range(A : array) return range of integer written A.range is
        return 0..A.size-1

    function Array() return array is
        for I in result.range loop
            any(result[I] := array.item())

    to Delete (A : array) is
        for I in A.range loop
            any.delete A[I]
