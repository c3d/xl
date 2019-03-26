// ****************************************************************************
//  module.xs                                       XL - An extensible language
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

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
