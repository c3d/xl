// ****************************************************************************
//  xl.parser.tree.xs               (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
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
// ****************************************************************************
// This document is confidential.
// Do not redistribute without written permission
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

module XL.PARSER.TREE with

    // Identification of the type of tree
    type tree_kind is enumeration
        xlUNKNOWN,
        xlINTEGER, xlREAL, xlSTRING, xlNAME,    // Atoms
        xlBLOCK,                                // Unary paren / block
        xlPREFIX,                               // Binary prefix-op
        xlINFIX,                                // Ternary infix-op
        xlLAST


    // The base class for all trees
    type tree_node is record with
        kind : tree_kind
    type tree is access to tree_node


    // -- Leafs ---------------------------------------------------------------
    
    // Representation of an integer
    type integer_node is tree_node with
        value : integer
    function NewInteger(value : integer) return access to integer_node is
        result.kind := xlINTEGER
        result.value := value

    // Representation of a real
    type real_node is tree_node with
        value : real
    function NewReal(value : real) return access to real_node is
        result.kind := xlREAL
        result.value := value

    // Representation of a string
    type string_node is tree_node with
        value : text
        quote : character
    function NewString(value : text; quote : character) return access to string_node is
        result.kind := xlSTRING
        result.value := value
        result.quote := quote

    // Representation of a name
    type name_node is tree_node with
        value : text
    function NewName(value : text) return access to name_node is
        result.kind := xlNAME
        result.value := value


    // -- Non-leafs -----------------------------------------------------------

    // Block and parentheses
    type block_node is tree_node with
        child   : tree
        opening : character
        closing : character
    function NewBlock(child : tree; opening : character; closing : character) return access to block_node is
        result.kind := xlBLOCK
        result.child := child
        result.opening := opening
        result.closing := closing

    // Unary prefix operator
    type prefix_node is tree_node with
        left  : tree
        right : tree
    function NewPrefix(left : tree; right : tree) return access to prefix_node is
        result.kind := xlPREFIX
        result.left := left
        result.right := right

    // Binary infix operator
    type infix_node is tree_node with
        name  : text
        left  : tree
        right : tree
    function NewInfix(name: text; left: tree; right: tree) return access to infix_node is
        result.kind := xlINFIX
        result.name := name
        result.left := left
        result.right := right

