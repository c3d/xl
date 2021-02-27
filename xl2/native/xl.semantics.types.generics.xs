// *****************************************************************************
// xl.semantics.types.generics.xs                                     XL project
// *****************************************************************************
//
// File description:
//
//     Description of generic types
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
// (C) 2003-2004,2006,2015,2020, Christophe de Dinechin <christophe@dinechin.org>
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

import TY = XL.SEMANTICS.TYPES
import DCL = XL.SEMANTICS.DECLARATIONS
import PT = XL.PARSER.TREE


module XL.SEMANTICS.TYPES.GENERICS with
// ----------------------------------------------------------------------------
//    Representation of generic types
// ----------------------------------------------------------------------------

    type any_type_data      is TY.any_type_data
    type any_type           is TY.any_type
    type declaration        is DCL.declaration
    type declaration_list   is string of declaration


    type generic_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //    Information in a generic type
    // ------------------------------------------------------------------------
        parameters      : declaration_list
        context         : SYM.symbol_table
        symbols         : SYM.symbol_table
        validation      : PT.tree
        in_validation   : boolean
    type generic_type is access to generic_type_data


    type generic_info_data is PT.info_data with
    // ------------------------------------------------------------------------
    //   Information about a context where generics are being declared
    // ------------------------------------------------------------------------
        context         : SYM.symbol_table
        symbols         : SYM.symbol_table
        generic_context : SYM.symbol_table
        parameters      : declaration_list
        initializer     : PT.tree
        validation      : PT.tree
        variadicity     : PT.tree
        generic_types   : string of generic_type
        rest            : PT.tree
    type generic_info is access to generic_info_data


    type generic_map is map[PT.tree, PT.tree]

    type instantiated_type_data is any_type_data with
    // ------------------------------------------------------------------------
    //   Representation for partial instantiation of generic types
    // ------------------------------------------------------------------------
        args            : generic_map
    type instantiated_type is access to instantiated_type_data


    // Access to the current generic context
    function IsGenericContext() return boolean
    function IsGenericType (tp : any_type) return boolean
    function IsGenericDeclaration (decl : declaration) return boolean
    function DeclGenericType (decl : declaration) return generic_type
    function NonGenericType (tp : any_type) return any_type
    function MakeGeneric(tp : any_type) return any_type
    procedure AddGenericDependency (tp : any_type)
    procedure AddVariadicity (anchor : PT.tree)
    function VariadicExpression() return PT.tree
    function VariadicDeclarations() return PT.tree

    // Enter a generic declaration
    function EnterGenericDecl(Decl  : PT.tree) return PT.tree
