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
// This program is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

module XL.PARSER.TREE with
// ----------------------------------------------------------------------------
//   Module implementing parse trees
// ----------------------------------------------------------------------------

    // Identification of the type of tree
    type tree_kind is enumeration
        xlUNKNOWN,
        xlINTEGER, xlREAL, xlTEXT, xlNAME,    // Atoms
        xlBLOCK,                              // Unary paren / block
        xlPREFIX,                             // Binary prefix-op
        xlINFIX,                              // Ternary infix-op
        xlWILDCARD,                           // For pattern matching
        xlLAST


    // The base class for all trees
    type tree_node is record with
        kind     : tree_kind
        position : integer                      // Context-dependent position
    type tree is access to tree_node


    // -- Leafs ---------------------------------------------------------------
    
    // Representation of an integer
    type integer_node is tree_node with
        value : integer
    type integer_tree is access to integer_node
    function NewInteger(value : integer;
                        pos : integer := -1) return integer_tree is
        result.kind := xlINTEGER
        result.position := pos
        result.value := value

    // Representation of a real
    type real_node is tree_node with
        value : real
    type real_tree is access to real_node
    function NewReal(value : real; pos : integer := -1) return real_tree is
        result.kind := xlREAL
        result.position := pos
        result.value := value

    // Representation of a text
    type text_node is tree_node with
        value : text
        quote : character
    type text_tree is access to text_node
    function NewText(value : text;
                     quote : character;
                     pos : integer := -1) return text_tree is
        result.kind := xlTEXT
        result.position := pos
        result.value := value
        result.quote := quote

    // Representation of a name
    type name_node is tree_node with
        value : text
    type name_tree is access to name_node
    function NewName(value : text; pos : integer := -1) return name_tree is
        result.kind := xlNAME
        result.position := pos
        result.value := value


    // -- Non-leafs -----------------------------------------------------------

    // Block and parentheses
    type block_node is tree_node with
        child   : tree
        opening : character
        closing : character
    type block_tree is access to block_node
    function NewBlock(child : tree;
                      opening : character; closing : character;
                      pos : integer := -1) return block_tree is
        result.kind := xlBLOCK
        result.position := pos
        result.child := child
        result.opening := opening
        result.closing := closing

    // Unary prefix operator
    type prefix_node is tree_node with
        left  : tree
        right : tree
    type prefix_tree is access to prefix_node
    function NewPrefix(left : tree;
                       right : tree;
                       pos : integer := -1) return prefix_tree is
        result.kind := xlPREFIX
        result.position := pos
        result.left := left
        result.right := right

    // Binary infix operator
    type infix_node is tree_node with
        name  : text
        left  : tree
        right : tree
    type infix_tree is access to infix_node
    function NewInfix(name: text;
                      left: tree;
                      right: tree;
                      pos : integer := -1) return infix_tree is
        result.kind := xlINFIX
        result.position := pos
        result.name := name
        result.left := left
        result.right := right

    // -- Wildcards (for tree matching) ---------------------------------------

    type wildcard_node is tree_node with
        name : text
    type wildcard_tree is access to wildcard_node
    function NewWildcard(name : text;
                         pos : integer := -1) return wildcard_tree is
        result.kind := xlWILDCARD
        result.position := pos
        result.name := name


    // ========================================================================
    // 
    //    Tree matching
    // 
    // ========================================================================

    type tree_map is map[text, PT.tree]
    function Matches (test : PT.tree;
                      ref : PT.tree;
                      in out arg : tree_map) return integer
    verbose : boolean := false
