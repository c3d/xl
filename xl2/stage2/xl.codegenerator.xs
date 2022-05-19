// *****************************************************************************
// xl.codegenerator.xs                                                XL project
// *****************************************************************************
//
// File description:
//
//     The XL code generator. It currently generates C code
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
// (C) 2003-2004,2006-2007,2009,2015, Christophe de Dinechin <christophe@dinechin.org>
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

import BC = XL.BYTECODE
import IO = XL.TEXT_IO

module XL.CODE_GENERATOR with

    procedure Transcode (input : BC.bytecode; file : IO.output_file)
    procedure Generate  (input : BC.bytecode; file : IO.output_file)
    function HasFormat(input : text) return boolean
    function Format(input : text) return text
    procedure InitializeBytecodeMap()
    procedure LoadBytecodeMap(filename : text)
    semantics_translations_init : text
    initializations             : text
    terminations                : text
    debug                       : boolean := false
