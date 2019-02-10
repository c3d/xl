// ****************************************************************************
//  xl.xs                                           XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//    The specificationd of the top-level XL module
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

XL is
// ----------------------------------------------------------------------------
//   Top-level module providing basic XL amenities
// ----------------------------------------------------------------------------
//   The XL module contains all the standard XL capabilities.
//   It is pre-loaded by any XL program, so that its content can be
//   used without any `import` or `use` statement.

    // Default authors for XL modules
    module AUTHORS

    // System configuration
    module SYSTEM               // Configuration of the system

    // Error handling
    module ERROR                // Detecting and reporting errors

    // Basic machine-level data types
    module TYPE                 // Representation of types
    module BOOLEAN              // Boolean type (true / false)
    module NUMBER               // Common aspects of numbers
    module INTEGER              // Signed integer types
    module UNSIGNED             // Unsigned integer types
    module REAL                 // Real numbers (floating-point representation)
    module CHARACTER            // Representation of characters
    module TEXT                 // Text (sequence of characters)
    module RANGE                // Range of elements (and range arithmetic)
    module ITERATOR             // Iterator
    module CONVERSION           // Implicit and explicit conversions
    module CONVERSIONS          // Standard conversions

    // Structure data types
    module RECORD               // Data records
    module MODULE               // Module interface, specification, etc
    module ENUMERATION          // Enumerations
    module FLAGS                // Individual flags
    module ACCESS               // Access to other data (refs, pointers, etc)
    module POINTER              // Raw machine-level pointers
    module REFERENCE            // Reference to other items (including counted)
    module TUPLE                // Tuples of values
    module ATOMIC               // Atomic operations
    module SEQUENCE             // Sequence of items
    module ARRAY                // Fixed-size sequence of same-type items
    module STRING               // Variable-size sequence of items
    module SLICE                // Slice of contiguous items
    module LIST                 // List of items
    module MAP                  // Mapping one data type to another
    module STREAM               // Stream of items

    // Basic program structures
    module CONDITIONAL          // Conditional operations (if-then-else, etc)
    module LOOP                 // Loops (infinite, while, until, etc)
    module CASE                 // Case handling

    // Basic amenities (in alphabetical order)
    module IO                   // Input/output operations
    module FILE                 // File operations
    module MEMORY               // Memory allocation, management, mapping
    module GARBAGE              // General-purpose garbage collector
    module TIME                 // Time-related operations
    module DATE                 // Date-related operations
    module FORMAT               // Text formatting
    module TEST                 // Unit testing
    module FLIGHT_RECORDER      // Flight-recorder for program operation
    module COMMAND_LINE         // Command-line options
    module ENVIRONMENT          // Environment variables
    module THREAD               // Multiple threads of execution
    module MESSAGE              // Messages that can be passed around
    module CHANNEL              // Communication channel between components
    module NETWORK              // Network access
    module REGULAR_EXPRESSION   // Regular expressions
    module SEMANTIC_VERSIONING  // Semantic versioning

    // Basic operations shared across many types
    module ASSIGN               // Assignments
    module COPY                 // Copy operations
    module MOVE                 // Move operations
    module CLONE                // Clone operations
    module DELETE               // Delete operation
    module ARITHMETIC           // Arithmetic operations, e.g. +, -, *, /
    module BITWISE              // Bitwise operations, e.g. and, or, xor
    module COMPARISON           // Comparisons, e.g. <, >, =

    // Advanced arithmetic and data types
    module MATH                 // Basic math functions

    // Character sets
    module ASCII                // ASCII code
    module UNICODE              // Unicode (wide characters)

    // Language processing
    module SYNTAX               // Syntax description
    module SCANNER              // Scan XL tokens
    module PARSER               // Parse XL trees
    module EVALUATOR            // Evaluate XL trees
    module COMPILER             // Compile to machine code
    module SYMBOL               // Symbol table
    module RUNTIME              // Runtime environment
    module LIFETIME             // Lifetime of entities

    // Modules that are visible by default
    use TYPE, BOOLEAN, INTEGER, UNSIGNED, REAL, CHARACTER, TEXT
    use RANGE, ITERATOR, CONVERSION
    use RECORD, MODULE, ENUMERATION, ACCESS, TUPLE
    use CONDITIONAL, LOOP, CASE
