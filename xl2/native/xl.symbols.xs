// ****************************************************************************
//  xl.symbols.xs                   (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     XL symbol table
// 
// 
// 
// 
// 
// 
// 
// 
// ****************************************************************************
// This document is released under the GNU Genral Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

import PT = XL.PARSER.TREE


module XL.SYMBOLS with
// ----------------------------------------------------------------------------
//    Interface of the symbol table
// ----------------------------------------------------------------------------

    // ========================================================================
    // 
    //   Symbol table creation and properties
    // 
    // ========================================================================

    type symbol_table_data
    type symbol_table is access to symbol_table_data


    // Creating and deleting a symbol table
    function NewSymbolTable (enclosing : symbol_table) return symbol_table
    procedure DeleteSymbolTable (table : symbol_table)

    // Walking the chain of enclosing contexts
    function Enclosing (table : symbol_table) return symbol_table


    // ========================================================================
    // 
    //    Rewriting trees
    // 
    // ========================================================================

    type rewrite_data
    type rewrite is access to rewrite_data

    type rewrite_data is record with
    // ------------------------------------------------------------------------
    //   Information associated with plugin-defined rewrites
    // ------------------------------------------------------------------------
        reference_form  : PT.tree
        score_adjust    : function (test        : PT.tree;
                                    info        : rewrite;
                                    in out args : PT.tree_map;
                                    depth       : integer;
                                    base_score  : integer) return integer
        translator      : function (input       : PT.tree;
                                    info        : rewrite;
                                    in out args : PT.tree_map) return PT.tree
    type rewrites_list is string of rewrite


    procedure Enter  (into : symbol_table; kind : text; info : rewrite)
    function  Lookup (table     : symbol_table;
                      kind      : text;
                      tree      : PT.tree) return PT.tree

    // Return the name for a tree
    function TreeName (expr : PT.tree) return text
