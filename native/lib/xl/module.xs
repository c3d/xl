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

MODULE with
// ----------------------------------------------------------------------------
//    Interface for module management
// ----------------------------------------------------------------------------

    import BOOLEAN, SYMBOLS, PARSER, FILE

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
    import ModuleList:module_list               as nil
    use    ModuleList:module_list               as nil


    module_list is either
    // ------------------------------------------------------------------------
    //   List of modules for `import` or `use`
    // ------------------------------------------------------------------------
        // Simple case: 'use PARSER`
        Name:PARSER.tree

        // Sequence: `use PARSER, SCANNER`
        Head:module_list, Tail:module_list

        // Conditional import: `use RESTART[iterator] if kind.RESTART`
        Conditional:module_list if Condition:boolean

        // Conditional import if defined: `use COPY[range] when COPY[value]`
        IfDefined:module_list when MustBeDefined:PARSER.tree

        // Short name: `import RE = REGULAR_EXPRESSION`
        Alias:PARSER.name = FullName:PARSER.tree
