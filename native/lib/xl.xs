// ****************************************************************************
//  xl.xs                                           XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//    The specification of the top-level XL modules
//
//   The XL module contains all the standard XL capabilities.
//   It is pre-loaded by any XL program, so that its content can be
//   used without any `import` or `use` statement.
//
//
//
//
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************


// Modules that are visible by default without an `import`
use TYPE
use BOOLEAN, INTEGER, UNSIGNED, REAL, CHARACTER
use TEXT
use RANGE, ITERATOR, CONVERSIONS
use RECORD, MODULE, ENUMERATION, ACCESS, TUPLE
use CONDITIONAL, LOOP, CASE


// System configuration
module SYSTEM               // Configuration of the system

// Error handling
module ERROR                // Detecting and reporting errors

// Basic machine-level data types
module TYPE                 // Representation of types
module NUMBER               // Common aspects of numbers
module ENUMERATED           // Enumerated values
module BOOLEAN              // Boolean type (true / false)
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
module OWN                  // Owning pointer
module BOX                  // Boxing items
module REFERENCE            // Reference to other items (including counted)
module TUPLE                // Tuples of values
module ATOMIC               // Atomic operations
module SEQUENCE             // Sequence of items
module CONTAINER            // Container of items
module ARRAY                // Fixed-size sequence of same-type items
module STRING               // Variable-size sequence of items
module SLICE                // Slice of contiguous items
module LIST                 // List of items
module SET                  // Set of unique values
module STREAM               // Stream of items
module MAP                  // Mapping values
module REDUCE               // Reducing
module FILTER               // Filtering

// Operations shared across many types
module ASSIGN               // Assignments
module COPY                 // Copy operations
module MOVE                 // Move operations
module CLONE                // Clone operations
module DELETE               // Delete operation
module ARITHMETIC           // Arithmetic operations, e.g. +, -, *, /
module BITWISE              // Bitwise operations, e.g. and, or, xor
module COMPARISON           // Comparisons, e.g. <, >, =

// Advanced arithmetic and data types
module MATH                 // Math functions and types
// Program structures
module CONDITIONAL          // Conditional operations (if-then-else, etc)
module LOOP                 // Loops (infinite, while, until, etc)
module CASE                 // Case handling
module CONTRACT             // Contract specification

// System-level utilities
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
module TASK                 // Operation to execute, possibly in a thread
module MESSAGE              // Messages that can be passed around
module CHANNEL              // Communication channel between components
module NETWORK              // Network access
module URL                  // URL representations
module REGULAR_EXPRESSION   // Regular expressions
module SEMANTIC_VERSIONING  // Semantic versioning


// Language processing
module LITTERAL             // Litteral values
module SYNTAX               // Syntax description
module SCANNER              // Scan XL tokens
module PARSER               // Parse XL trees
module EVALUATOR            // Evaluate XL trees
module COMPILER             // Compile to machine code
module SOURCE               // Program source
module CODE                 // Program code
module CLOSURE              // Closure
module SYMBOL               // Symbol table
module RUNTIME              // Runtime environment
module LIFETIME             // Lifetime of entities


MODULE_INFO is
// ----------------------------------------------------------------------------
//   Information about the top-level XL module
// ----------------------------------------------------------------------------
    use SEMANTIC_VERSIONING, DATE, TEXT

    Version             is 0.0.1
    Date                is February 10, 2019
    URL                 is "http://github.com/c3d/xl"
    License             is "GPL v3"
    Creator             is "Christophe de Dinechin <christophe@dinechin.org>"
    Authors             is
        Creator
    Email               is "xl-devel@lists.sourceforge.net"
    Copyright           is "(C) " & Year & ", " & Join(Authors, ", ")
