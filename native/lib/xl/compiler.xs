// ****************************************************************************
//  compiler.xs                                     XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface with the XL compiler
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

// Declaration for builtin and runtime operations
builtin Operation           as anything
runtime Operation           as anything

module LITTERAL             // Litteral values
module SYMBOL               // Symbol table
module SYNTAX               // Syntax description
module SCANNER              // Scan text into XL tokens
module PARSER               // Parse tokens into XL parse trees
module SEQUENCE             // Sequence of insructions / expressions
module EVALUATOR            // Evaluate XL parse trees
module COMPILER             // Compile to machine code
module SOURCE               // Program source code
module CODE                 // Program machine code
module CLOSURE              // Closure
module RUNTIME              // Runtime environment
