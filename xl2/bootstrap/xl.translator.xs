// ****************************************************************************
//  xl.translator.xs                (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     The basic XL translator
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

import PT = XL.PARSER.TREE
import IO = XL.TEXT_IO

module XL.TRANSLATOR with

    type tree_map is map[text, PT.tree]
    function Matches (test : PT.tree; ref : PT.tree; in out arg : tree_map) return integer
    procedure Compile(input : PT.tree)

