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
    //    Simple name lookup
    // 
    // ========================================================================
    //  This associates a name to a list of tree values
    //  Multiple trees may be associated with a simple name.
    //  Note that callers may forbid this situation. For instance,
    //  duplicate type names or object names are not allowed in XL,
    //  and the corresponding semantics check if duplicates are entered.

    type tree_list is PT.tree_list

    type lookup_kind is enumeration
    // ------------------------------------------------------------------------
    //    Kind of lookup we care for
    // ------------------------------------------------------------------------

        lookupLocalOnly,        // Don't recurse at all
        lookupInnermost,        // Closest scope where something is found
        lookupAll               // Return all results


    procedure Enter (table          : symbol_table;
                     category       : text;
                     name           : text;
                     value          : PT.tree)
    procedure Lookup (table         : symbol_table;
                      category      : text
                      name          : text;
                      in out values : tree_list;
                      mode          : lookup_kind := lookupAll)
    function LookupOne (table       : symbol_table;
                        category    : text;
                        name        : text;
                        mode        : lookup_kind := lookupAll) return PT.tree
                     


    // ========================================================================
    // 
    //    Rewriting trees
    // 
    // ========================================================================
    //  XL uses tree rewrites at very fundamental levels, notably while
    //  processing language semantics, where tree forms are pattern-matched
    //  to find the closest matching form among all the plug-ins
    //  (or 'translation' statements).
    //  Another use of tree rewrites is for the 'written' form in
    //  function or type declarations, for instance:
    //      function AddAndMul(A, B, C: real) return real written A+B*C
    //      generic [type T] type pointer written pointer to T

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


    procedure EnterRewrite  (into : symbol_table;
                             kind : text;
                             info : rewrite)
    function  LookupRewrite (table         : symbol_table;
                             Name          : text;
                             test_tree     : PT.tree;
                             out count     : integer;
                             report_misses : boolean;
                             report_ambig  : boolean) return PT.tree;

    // Lookup helpers                      
    function  LookupRewrite (table     : symbol_table;
                             kind      : text;
                             tree      : PT.tree) return PT.tree
    function  LookupRewrite (table     : symbol_table;
                             kind      : text;
                             tree      : PT.tree;
                             out count : integer) return PT.tree
    function  RewritesCount  (table     : symbol_table;
                              kind      : text;
                              tree      : PT.tree) return integer

    // Return the name for a tree
    function TreeName (expr : PT.tree) return text

    // A default translator that rewrites as the reference form
    function RewriteAsTranslator (input       : PT.tree;
                                  info        : rewrite;
                                  in out args : PT.tree_map) return PT.tree
    function RewriteAs (reference_form : PT.tree) return rewrite


    // ========================================================================
    // 
    //    Properties
    // 
    // ========================================================================
    //
    //  Properties are component-specific information held in the symbol
    //  table. For instance, whether we are in a local scope is an
    //  integer property named "LOCAL", which is non-zero in local scopes,
    //  and zero otherwise.
    //
    //  Properties are stored as trees, though helpers manipulate
    //  forms such as text or integers, which might later be stored
    //  in separate maps if performance required it.
    //
    //  Scope items correspond to properties representing lists of
    //  statements (such as initialization, termination, declarations)
    //

    // Creating and getting scope properties
    procedure SetProperty (table : symbol_table;
                           name  : text;
                           prop  : PT.tree)
    function GetProperty (table    : symbol_table;
                          name     : text;
                          enclosed : boolean := false) return PT.tree
    procedure SetInteger (table : symbol_table;
                          name  : text;
                          value : integer)
    function GetInteger (table         : symbol_table;
                         name          : text;
                         default_value : integer := 0;
                         enclosed      : boolean := false) return integer
    procedure SetText    (table : symbol_table;
                          name  : text;
                          value : text)
    function GetText    (table         : symbol_table;
                         name          : text;
                         default_value : text := "";
                         enclosed      : boolean := false) return text

    // Creating and getting scope items
    procedure AddScopeItem(table : symbol_table; section: text; item : PT.tree)
    function ScopeItems (table : symbol_table; section : text) return PT.tree



    // ========================================================================
    // 
    //    Misc helpers
    // 
    // ========================================================================

    // Creating a temporary compiler-generated name
    function Temporary(base : text; pos : integer := -1) return PT.name_tree


    // ========================================================================
    // 
    //    Control globals
    // 
    // ========================================================================
    
