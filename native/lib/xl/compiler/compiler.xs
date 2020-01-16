// *****************************************************************************
// compiler.xs                                                        XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2019, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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
