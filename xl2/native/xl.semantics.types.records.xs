// ****************************************************************************
//  xl.semantics.types.records.xs   (C) 1992-2004 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Implementation of record types
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
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

import TY = XL.SEMANTICS.TYPES
import DCL = XL.SEMANTICS.DECLARATIONS
import SYM = XL.SYMBOLS


module XL.SEMANTICS.TYPES.RECORDS with
// ----------------------------------------------------------------------------
//    Interface of the module
// ----------------------------------------------------------------------------

    type any_type_data      is TY.any_type_data
    type any_type           is TY.any_type
    type declaration        is DCL.declaration
    type declaration_list   is string of declaration


    type record_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //    Information in a record description
    // ------------------------------------------------------------------------
        fields          : declaration_list
        symbols         : SYM.symbol_table
    type record_type is access to record_type_data


    // Create a parameter list
    procedure MakeFieldList (Fields      : PT.tree;
                             in out List : declaration_list)

    // Create a function type
    function MakeRecordType(Source     : PT.tree;
                            Base       : PT.tree;
                            Fields     : PT.tree) return record_type

    function EnterType (Source   : PT.tree;
                        Base     : PT.tree;
                        Fields   : PT.tree) return PT.tree

    function GetRecordType(Record : PT.tree) return record_type
    function IsRecord (Record : PT.tree) return boolean
    function Index(Record : PT.tree;
                   Field  : PT.name_tree) return BC.bytecode
