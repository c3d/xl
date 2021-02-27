// *****************************************************************************
// xl.text_io.xs                                                      XL project
// *****************************************************************************
//
// File description:
//
//     This file is the interface for all text-based I/O functions
//     It is roughly equivalent to <stdio.h> in C
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2006,2008,2015,2020, Christophe de Dinechin <christophe@dinechin.org>
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

module XL.TEXT_IO with

    // ========================================================================
    //
    //   Data types
    //
    // ========================================================================

    // A file represents a data file for text I/O
    type file

    // A writable is a type which can be written to a file
    generic type writable where
        W : writable
        F : file := file("Dummy", "w")
        any.Write F, W

    // A readable is a type which can be read from a file
    generic type readable where
        W : readable
        F : file := file("Dummy", "r")
        Read F, W



    // ========================================================================
    //
    //   Creating files
    //
    // ========================================================================

    function File(Name, Mode : text) return file    // Create a file (open it)
    function Delete(F : file)                       // Delete a file (close it)
    to Copy(out Tgt : file; in Src : file) written Tgt := Src



    // ========================================================================
    //
    //   Writing to files
    //
    // ========================================================================

    // Writing to a file
    to Write(F : file)                         // Writing nothing

    to Write(F : file; C : character)          // Writing a character
    to Write(F : file; N : integer)            // Writing an integer
    to Write(F : file; N : unsigned)           // Writing an unsigned
    to Write(F : file; N : real)               // Writing a real number
    to Write(F : file; S : text)               // Writing a text

    to Write(F : file; W : writable; ...)      // Write to a file
    to WriteLn(F : file; ...)                  // Write with CR at end

