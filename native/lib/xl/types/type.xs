// ****************************************************************************
//  type.xs                                         XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for types
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

use BITWISE, MEMORY, TEXT, BOOLEAN, SYMBOLS, PARSER

module TYPE with
// ----------------------------------------------------------------------------
//  Specificatoin of the XL type system
// ----------------------------------------------------------------------------
/*  The TYPE module describes the basic type system in XL.
    It provides the `type` type itself, operations on types, as well as
    a number of type constructors, for example to create constant types.

    Types are annotations given to the compiler that indicate the kind
    of operations that can be performed on a value. Types also implicitly
    determine the size and binary representation of values.

    Two operators are used for type annotation, `X:T` and `X as T`.
    Both indicate that `X` belongs to type `T`.

    The first difference between the two type annotations is parsing
    precedence. This makes it convenient to write a declaration like
        X:integer + Y:integer as integer

    The second difference is that `X as T` denotes that X is not mutable,
    whereas `X : T` denotes that X is mutable.

    A given piece of code can belong to multiple types. For example,
    code like `2 + 3` could belong to an `addition` type defined as
    `addition is type A+B`, but also be considered an `infix` type before
    evaluation, or an `integer` after evaluation using the declarations
    in the `ARITHMETIC` module.

    A subtype is a type whose values all belong to a same parent type.
    Several type constructors create subtypes with various restrictions.

    A derived type is a type built by adding more capabilities to a base type.
    Therefore, the base type is a subtype of the derived type.

    The TYPE module offers a number of type constructors, notably the
    most basic one, `type Pattern`, which returns a type matching the
    pattern. For example, `type complex(Re:real, Im:real)` would match
    the vaue `complex(2.0, 3.5)`. This is XL's equivalent of struct in C.

    Types are first-class citizen in XL: they can be stored in variables,
    passed around, and so on. The compiler will determine if a specific
    use of a type variable should be treated like "template code" to use
    C++ terminology, or if there is a better way to implement it.
    For example, consider an allocation of memory for type `T`:
        allocate T:type as pointer[T]
    The compiler is free to implement this as a generic function, similar
    to a C++ template, or as a function taking some pointer to type data.
*/

    type type with
    // ------------------------------------------------------------------------
    //   A `type` is used to identify a set of values
    // ------------------------------------------------------------------------
        Name                    as PARSER.tree
        BitSize                 as bit_count
        BitAlign                as bit_count
        ByteSize                as byte_count
        ByteAlign               as byte_count
        Mutable                 as boolean
        Indirect                as type
        DynamicType             as type
        DynamicSize             as type


    type pattern with
    // ------------------------------------------------------------------------
    //    A `pattern` identifies specific forms in the code
    // ------------------------------------------------------------------------
        Shape                   as PARSER.tree
    Tree:PARSER.tree            as pattern // Implicitly created from a tree


    type patterns with
    // ------------------------------------------------------------------------
    //    A sequence of patterns
    // ------------------------------------------------------------------------
        Patterns                as sequence of pattern
    Seq:sequence of pattern     as patterns // Implicitly created from sequence


    // Return a type matching the pattern
    type Form:pattern           as type

    // Static subtype with automatic conversion (may truncate input value)
    like T:type                 as type

    // Static subtype without any automatic conversion (must be explicit)
    another T:type              as type

    // Dynamic subtype preserving the original type of the value
    any T:type                  as type

    // Return a type that matches either of the children forms
    either Forms:patterns       as type

    // A derived type
    T:type with D:declarations  as type

    // Return a type with some additional condition
    T:type where Cond:boolean   as type

    // A subtype that only accepts a variable of the exact type
    variable T:type             as type

    // A subtype that declares a local variable of the type
    local T:type                as type

    // A subtype that declares a variable of the type in enclosing scope
    defining T:type             as type

    // Subtypes that only accept mutable or constant (non-mutable) values
    constant T:type             as type
    mutable T:type              as type

    // For arguments, constrain the direction for argument passing
    my  T:type                  as type // Take ownership to modify ('mine')
    in  T:type                  as type // Input only, destroyed in caller
    out T:type                  as type // Output only, create in caller

    // Check if a type contains a value or pattern
    T:type contains F:pattern   as boolean

    // Operations on types
    T1:type or  T2:type         as type // Union of two types
    T1:type xor T2:type         as type // One or the other
    T1:type and T2:type         as type // Must belong to both types
    not T:type                  as type // Does not belong to the given type

    // A specific type holding no value
    nil                         as type

    // A specific type that can hold anything, one that never matches
    anything                    as type
    nothing                     is not anything
