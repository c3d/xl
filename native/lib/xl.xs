// ****************************************************************************
//  xl.xs                                           XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//    The specification of the top-level XL modules
//
//   The XL module contains all the standard XL capabilities.
//   It is pre-loaded by any XL program, so that its content can be
//   used without any `import` or `use` statement.
//
//
//
//
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************


// Modules that are visible by default without an `import`
use TYPE
use BOOLEAN, INTEGER, UNSIGNED, REAL, CHARACTER
use TEXT
use RANGE, ITERATOR, CONVERSIONS
use RECORD, MODULE, ENUMERATION, ACCESS, TUPLE
use CONDITIONAL, LOOP, CASE


module TYPES                    // Basic data types (integer, boolean, ...)
module OPERATIONS               // Elementary operations (arithmetic, ...)
module DATA                     // Data structures (arrays, lists, ...)
module PROGRAM                  // Program structures (loop, tests, ...)
module ALGORITHMS               // General algorithms (sort, find, ...)
module MATH                     // Mathematics (functions, types, ...)
module SYSTEM                   // System (memory, files, ...)
module COMPILER                 // Compiler (scanner, parser, ...)


module_info
// ----------------------------------------------------------------------------
//   Information about the top-level XL module
// ----------------------------------------------------------------------------
    Version             is 0.0.1
    Date                is February 10, 2019
    URL                 is "http://github.com/c3d/xl"
    License             is "GPL v3"
    Creator             is "Christophe de Dinechin <christophe@dinechin.org>"
    Authors             is
        Creator
    Email               is "xl-devel@lists.sourceforge.net"
    Copyright           is "(C) " & Year & ", " & Join(Authors, ", ")
