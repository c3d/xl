// *****************************************************************************
// xl.semantics.writtenforms.xs                                       XL project
// *****************************************************************************
//
// File description:
//
//     Information stored about written forms
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
// (C) 2003-2006,2015, Christophe de Dinechin <christophe@dinechin.org>
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

import SYM = XL.SYMBOLS
import PT = XL.PARSER.TREE
import DCL = XL.SEMANTICS.DECLARATIONS
import FN = XL.SEMANTICS.FUNCTIONS
import GEN = XL.SEMANTICS.TYPES.GENERICS
import GN = XL.SEMANTICS.GENERICS


module XL.SEMANTICS.WRITTEN_FORMS with
// ----------------------------------------------------------------------------
//    Dealing with written forms
// ----------------------------------------------------------------------------

    type written_args_map is map[text, FN.declaration]


    type written_form_data is SYM.rewrite_data with
    // ------------------------------------------------------------------------
    //    Extra information stored about a written form
    // ------------------------------------------------------------------------
        function                : FN.function
        args_map                : written_args_map
        generic_args            : GN.generic_map
    type written_form is access to written_form_data


    type generic_written_form_data is SYM.rewrite_data with
    // ------------------------------------------------------------------------
    //    Extra information stored about a written form
    // ------------------------------------------------------------------------
        generic_type            : GEN.generic_type
        args_map                : written_args_map
        declaration             : DCL.declaration
    type generic_written_form is access to generic_written_form_data


    function EnterWrittenForm(fniface : PT.tree;
                              wrform  : PT.tree) return PT.tree
    procedure EnterWrittenForm(GenDecl : DCL.declaration;
                               gentype : GEN.generic_type
                               wrform  : PT.tree)

