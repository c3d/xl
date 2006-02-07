// ****************************************************************************
//  xl.semantics.generics.xs        (C) 1992-2004 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//    Interface of generic manipulations in XL
// 
// 
// 
// 
// 
// 
// 
// 
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

import PT = XL.PARSER.TREE
import SYM = XL.SYMBOLS
import DCL = XL.SEMANTICS.DECLARATIONS

module XL.SEMANTICS.GENERICS with
// ----------------------------------------------------------------------------
//    Module implementing manipulations of generics
// ----------------------------------------------------------------------------

    procedure LookupInstantiation(NameTree       : PT.tree;
                                  Args           : PT.tree;
                                  kind           : text;
                                  out BestDecl   : DCL.declaration;
                                  out ActualArgs : PT.tree_list;
                                  out ScopeExpr  : PT.tree)

    function Instantiate(Decl       : DCL.declaration;
                         Args       : PT.tree_list;
                         BaseRecord : PT.tree) return BC.bytecode

    function InstantiateType (Name : PT.tree;
                              Args : PT.tree) return BC.bytecode

    function IsGenericName(Name : PT.tree; kind : text) return boolean
