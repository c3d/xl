// *****************************************************************************
// module.xs                                                          XL project
// *****************************************************************************
//
// File description:
//
//     Interface for modules
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2019, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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

import BOOLEAN, SYMBOLS, PARSER, FILE


module MODULE with
// ----------------------------------------------------------------------------
//    Interface for module management
// ----------------------------------------------------------------------------

    interface   : type with
        Defined : SYMBOLS.symbols
        Form    : PARSER.tree
        Body    : PARSER.tree
        File    : FILE.file

    module      : like interface with
        Exports : interface


    // Creating a module interface
    ModuleInterface with Body                   as interface


    // Importing modules
    import ModuleList:module_list
    use    ModuleList:module_list


    module_list is either
    // ------------------------------------------------------------------------
    //   List of modules for `import` or `use`
    // ------------------------------------------------------------------------

        // List: `use PARSER, SCANNER`
        Head:module_list, Tail:module_list

        // Conditional import: `use RESTART[iterator] if kind.RESTART`
        Conditional:module_list if Condition:boolean

        // Conditional import if defined: `use COPY[range] when COPY[value]`
        IfDefined:module_list when MustBeDefined:code[module]

        // Short name: `import RE = REGULAR_EXPRESSION`
        Alias:PARSER.name = FullName:code[module]

        // Base name with arguments, e.g. `use COMPARISON[integer]`
        BaseName:PARSER.name [Arguments:list]

        // Simple case: 'use PARSER`
        Name:module_name
