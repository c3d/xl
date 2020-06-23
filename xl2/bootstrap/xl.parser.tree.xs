// *****************************************************************************
// xl.parser.tree.xs                                                  XL project
// *****************************************************************************
//
// File description:
//
//    Basic XL0 syntax tree for the XL programming language
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
// (C) 2003-2004,2015, Christophe de Dinechin <christophe@dinechin.org>
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

module XL.PARSER.TREE with

    // Identification of the type of tree
    type tree_kind is enumeration
        xlUNKNOWN,
        xlINTEGER, xlREAL, xlTEXT, xlNAME,    // Atoms
        xlBLOCK,                                // Unary paren / block
        xlPREFIX,                               // Binary prefix-op
        xlINFIX,                                // Ternary infix-op
        xlWILDCARD,                             // For pattern matching
        xlLAST


    // The base class for all trees
    type tree_node is record with
        kind : tree_kind
    type tree is access to tree_node


    // -- Leafs ---------------------------------------------------------------

    // Representation of an integer
    type integer_node is tree_node with
        value : integer
    type integer_tree is access to integer_node
    function NewInteger(value : integer) return integer_tree is
        result.kind := xlINTEGER
        result.value := value

    // Representation of a real
    type real_node is tree_node with
        value : real
    type real_tree is access to real_node
    function NewReal(value : real) return real_tree is
        result.kind := xlREAL
        result.value := value

    // Representation of a text
    type text_node is tree_node with
        value : text
        quote : character
    type text_tree is access to text_node
    function NewText(value : text; quote : character) return text_tree is
        result.kind := xlTEXT
        result.value := value
        result.quote := quote

    // Representation of a name
    type name_node is tree_node with
        value : text
    type name_tree is access to name_node
    function NewName(value : text) return name_tree is
        result.kind := xlNAME
        result.value := value


    // -- Non-leafs -----------------------------------------------------------

    // Block and parentheses
    type block_node is tree_node with
        child   : tree
        opening : text
        closing : text
    type block_tree is access to block_node
    function NewBlock(child : tree; opening : text; closing : text) return block_tree is
        result.kind := xlBLOCK
        result.child := child
        result.opening := opening
        result.closing := closing

    // Unary prefix operator
    type prefix_node is tree_node with
        left  : tree
        right : tree
    type prefix_tree is access to prefix_node
    function NewPrefix(left : tree; right : tree) return prefix_tree is
        result.kind := xlPREFIX
        result.left := left
        result.right := right

    // Binary infix operator
    type infix_node is tree_node with
        name  : text
        left  : tree
        right : tree
    type infix_tree is access to infix_node
    function NewInfix(name: text; left: tree; right: tree) return infix_tree is
        result.kind := xlINFIX
        result.name := name
        result.left := left
        result.right := right

    // -- Wildcards (for tree matching) ---------------------------------------

    type wildcard_node is tree_node with
        name : text
    type wildcard_tree is access to wildcard_node
    function NewWildcard(name : text) return wildcard_tree is
        result.kind := xlWILDCARD
        result.name := name


    // ========================================================================
    //
    //    Tree matching
    //
    // ========================================================================

    type tree_map is map[text, PT.tree]
    function Matches (test : PT.tree; ref : PT.tree; in out arg : tree_map) return integer
    verbose : boolean := false


procedure Debug(tree: PT.tree)
