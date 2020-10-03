// *****************************************************************************
// xl.system.xs                                                     XL project
// *****************************************************************************
//
// File description:
//
//     Interface for system utilities
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
// (C) 2019-2020, Christophe de Dinechin <christophe@dinechin.org>
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

module XL.SYSTEM with
// ----------------------------------------------------------------------------
//   Description of system-level services
// ----------------------------------------------------------------------------

    // Declaration for builtin and runtime operations
    builtin Operation           as anything
    runtime Operation           as anything

    // System modules
    module CONFIGURATION        // System configuration
    module TYPES                // System types
    module IO                   // Input/output operations
    module FILE                 // File operations
    module MEMORY               // Memory allocation, management, mapping
    module GARBAGE              // General-purpose garbage collector
    module TIME                 // Time-related operations
    module DATE                 // Date-related operations
    module FORMAT               // Text formatting
    module TEST                 // Unit testing
    module RECORDER             // Recording program operation
    module COMMAND_LINE         // Command-line options
    module ENVIRONMENT          // Environment variables
    module THREAD               // Multiple threads of execution
    module TASK                 // Operation to execute
    module MESSAGE              // Messages that can be passed around
    module CHANNEL              // Communication channel
    module NETWORK              // Network access
    module URL                  // Uniform Resource Locator
    module REGULAR_EXPRESSION   // Regular expressions
    module SEMANTIC_VERSIONING  // Semantic versioning
