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

    // Representation of a real
    type real_node is tree_node with
        value : real

    // Representation of a string
    type string_node is tree_node with
        value : text
        quote : character

    // Representation of a name
    type name_node is tree_node with
        value : text


    // -- Non-leafs -----------------------------------------------------------

    // Unary prefix operator
    type unary_node is tree_node with
        left  : tree
        right : tree

    // Binary infix operator
    type binary_node is tree_node with
        name  : text
        left  : tree
        right : tree

