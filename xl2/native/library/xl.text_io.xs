// ****************************************************************************
//  xl.text_io.xs                   (C) 1992-2004 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
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
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

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
    to Write(F : file; N : real)               // Writing a real number
    to Write(F : file; S : text)               // Writing a text

    to Write(F : file; W : writable; ...)      // Write to a file
    to WriteLn(F : file; ...)                  // Write with CR at end

