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


    type info_data
    type info is access to info_data
    type infos is map [text, info]
    type info_data is record with
    // ------------------------------------------------------------------------
    //    Attributes attached to a tree by semantics
    // ------------------------------------------------------------------------
        name      : text


    type tree_node
    type tree is access to tree_node
    type tree_node is record with
    // ------------------------------------------------------------------------
    // The base class for all trees
    // ------------------------------------------------------------------------
        source     : tree
        kind       : tree_kind
        position   : integer                  // Context-dependent position
        info       : infos

    NOPOS : integer := -1         // May be changed to new default position

    function FindInfo(from : tree; name : text) return info
    procedure SetInfo(from : tree; name : text; data: info)
    procedure PurgeInfo(from : tree; name : text)

    type tree_info_data is info_data with
    // ------------------------------------------------------------------------
    //   Attaching a tree to another tree (e.g. written semantics cache)
    // ------------------------------------------------------------------------
        value   : tree
    type tree_info is access to tree_info_data

    procedure AttachTree(toTree : tree; key : text; attached : tree)
    function Attached(toTree : tree; key : text) return tree



    // -- Leafs ---------------------------------------------------------------
    
    // Representation of an integer
    type integer_node is tree_node with
        value : integer
    type integer_tree is access to integer_node
    function NewInteger(value : integer;
                        pos : integer := NOPOS) return integer_tree is
        result.source := nil
        result.kind := xlINTEGER
        result.position := pos
        result.value := value

    // Representation of a real
    type real_node is tree_node with
        value : real
    type real_tree is access to real_node
    function NewReal(value : real; pos : integer := NOPOS) return real_tree is
        result.source := nil
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
                     pos : integer := NOPOS) return text_tree is
        result.source := nil
        result.kind := xlTEXT
        result.position := pos
        result.value := value
        result.quote := quote

    // Representation of a name
    type name_node is tree_node with
        value : text
    type name_tree is access to name_node
    function NewName(value : text; pos : integer := NOPOS) return name_tree is
        result.source := nil
        result.kind := xlNAME
        result.position := pos
        result.value := value


    // -- Non-leafs -----------------------------------------------------------

    // Block and parentheses
    type block_node is tree_node with
        child   : tree
        opening : text
        closing : text
    type block_tree is access to block_node
    function NewBlock(child : tree;
                      opening : text; closing : text;
                      pos : integer := NOPOS) return block_tree is
        result.source := nil
        result.kind := xlBLOCK
        result.child := child
        result.opening := opening
        result.closing := closing
        if pos = NOPOS and child <> nil then
            pos := child.position
        result.position := pos

    // Unary prefix operator
    type prefix_node is tree_node with
        left  : tree
        right : tree
    type prefix_tree is access to prefix_node
    function NewPrefix(left : tree;
                       right : tree;
                       pos : integer := NOPOS) return prefix_tree is
        result.source := nil
        result.kind := xlPREFIX
        result.left := left
        result.right := right
        if pos = NOPOS and left <> nil then
            pos := left.position
        if pos = NOPOS and right <> nil then
            pos := right.position
        result.position := pos

    // Binary infix operator
    type infix_node is tree_node with
        name  : text
        left  : tree
        right : tree
    type infix_tree is access to infix_node
    function NewInfix(name: text;
                      left: tree;
                      right: tree;
                      pos : integer := NOPOS) return infix_tree is
        result.source := nil
        result.kind := xlINFIX
        result.name := name
        result.left := left
        result.right := right
        if pos = NOPOS and left <> nil then
            pos := left.position
        if pos = NOPOS and right <> nil then
            pos := right.position
        result.position := pos

    // -- Wildcards (for tree matching) ---------------------------------------

    type wildcard_node is tree_node with
        name : text
    type wildcard_tree is access to wildcard_node
    function NewWildcard(name : text;
                         pos : integer := NOPOS) return wildcard_tree is
        result.source := nil
        result.kind := xlWILDCARD
        result.position := pos
        result.name := name


    // ========================================================================
    // 
    //    Tree matching
    // 
    // ========================================================================

    type tree_map is map[text, tree]
    type tree_list is string of tree
    function XLNormalize (name : text) return text
    function Matches (test : tree;
                      ref : tree;
                      in out arg : tree_map) return integer
    function Matches (test : tree;
                      ref : tree) return boolean
    function LargestMatch (test : tree;
                           ref_list : tree_list;
                           in out arg : tree_map) return integer

    function Clone(original : tree) return tree
