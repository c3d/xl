// ****************************************************************************
//  pattern.xs                                      XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Description of type patterns
//
//
//
//
//
//
//
//
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

use parse_tree from XL.COMPILER.PARSER

type pattern with
// ----------------------------------------------------------------------------
//    A `pattern` identifies specific forms in the code
// ----------------------------------------------------------------------------
    Shape                   as parse_tree
    Tree:parse_tree         as pattern // Implicitly created from parse tree


type patterns with
// ----------------------------------------------------------------------------
//    A sequence of patterns, either new-line or semi-colon separated
// ----------------------------------------------------------------------------
    Patterns                as sequence of pattern
    Seq:sequence of pattern as patterns // Implicitly created from sequence
