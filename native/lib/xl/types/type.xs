// ****************************************************************************
//  type.xs                                         XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for types
//
//     This module provides
//     - the `type` type itself, including key type attributes
//     - common operations on types
//     - a number of type constructors, notably to create subtypes
//
//
//
//
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************
/*
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

    A subtype is a type whose values all belong to its supertype.
    A subtype can therefore be used wherever the supertype can.
    Several type constructors create subtypes with various restrictions.
    For example, `constant integer` is a subtype of `integer` where
    values cannot be mutated, whereas `integer range 1..5` is a subtype
    of integer where values have to be between 1 and 5.

    A derived type is a type built by adding more capabilities to a base type.
    Therefore, the derived type is a subtype of the base type. For example,
    `integer` is a derived type of `number`, adding a specific representation
    of values, which implies that `integer` values can be used for any
    operation that accepts the `number` type.

    The TYPE module offers a number of type constructors, notably the
    most basic one, `type Pattern`, which returns a type matching the
    pattern. For example, `type complex(Re:real, Im:real)` would match
    the vaue `complex(2.0, 3.5)`. This is XL's equivalent of struct in C.

    Types are first-class citizen in XL: they can be stored in variables,
    passed around, and so on. The compiler will determine if a specific
    use of a type variable should be treated like "template code" to use
    C++ terminology, or if there is a better way to implement it.
    For example, consider an allocation of memory for type `T`:
        Allocate[type T] as pointer[T]
    The compiler is free to implement this as a generic function, similar
    to a C++ template, or as a function taking some pointer to type data,
    using for example `T.ByteSize` to allocate memory.
*/


use BITWISE, MEMORY, TEXT, BOOLEAN, SYMBOLS, PARSER

type type with
// ----------------------------------------------------------------------------
//   A `type` is used to identify a set of values
// ----------------------------------------------------------------------------
//   Simple types like `integer` or `array[1..5] of real` occupy a known
//   amount of space in memory. They are defined by consecutive bits and bytes.
//
//   Reference and pointer types are represented by a machine pointer.
//   For example, an `access integer` will be a simple pointer to an integer.
//   In that case, the pointer type's `Indirect` field points to the type
//   being pointed to, `integer` in the above example.
//
//   If the actual type of the object is only known at runtime, as would
//   be the case for `any integer`, then the value pointer is followed
//   by a type pointer, and `DynamicType` is a pointer to that dynamic type.
//   For example, if you pass a `M:mammal` value as `A:any animal`, then
//   `(any animal).Indirect = animal` and `(any animal).DynamicType` is
//   `any animal`. The value `A` is made of two pointers, the first one
//   being a pointer to `M`, the second one being a pointer to `mammal`
//
//   If the actual size of the object is only known at runtime, as would be
//   the case for the payload of a `string of integer`, then `Indirect`
//   would point to a type where `DynamicSize` is itself the size type.
//   In that case, the size type is what the base pointer points to,
//   For example, a small string with less than 255 byte-sized elements
//   could be represented with `DynamicSize` pointing to `unsigned8`.
//
//   The fields `Mutable` and `Constant` indicate if the type was explicitly
//   made mutable or constant.


     // A unique, compiler-generated name for the type
    Name                        as parse_tree

    // Number of bits used to represent the type, e.g. 1 for boolean
    BitSize                     as bit_count

    // Alignment in bits, e.g 8 for boolean on most byte-oriented machines
    BitAlign                    as bit_count

    // Number of bytes, e.g. 1 for boolean on most byte-oritented machines
    ByteSize                    as byte_count

    // Byte alignment, e.g. 4 for integer on a 32-bit machine with 8-bit bytes
    ByteAlign                   as byte_count

    // True if the type was explicitly marked as mutable
    Mutable                     as boolean

    // True if the type was expliclty marked as constant
    Constant                    as boolean

    // Optional type of data pointed to by values of any pointer/ref type
    Indirect                    as optional type

    // Type of dynamic type field for a type like `any integer`
    DynamicType                 as optional type

    // Type of dynamic size field for a type like string's inner data
    DynamicSize                 as optional type

    // Lifetime for values of that type
    LifeTime                    as lifetime

    // Returns actual form type if the type matches the pattern
    Matches Form:pattern        as optional type

    // Return conversion function to convert to another type, if any
    Convert ToType:type         as optional (Value:anything -> anything)

    // Return the N-th operation for that type, in declaration order
    Operation Index:unsigned    as optional (Value:anything -> anything)


// Return a type matching the pattern
type Form:pattern               as type

// Static subtype without any automatic conversion (must be explicit)
//   unix_file_descriptor is another integer
another T:type                  as type

// Dynamic reference subtype preserving the original type of the value
//   intersect (S1:any shape, S2:any shape) as any shape
any T:type                      as type

// Return a type that matches either of the children forms
//   type complex is either { rectangular(Re, Im); polar(Modulus, Argument) }
either Forms:patterns           as type

// A derived type with additional declarations
//   type circle is shape with { Radius: real; Center: point }
T:type with D:declarations      as type

// Return a type with some additional condition
//   type odd is integer where (self and 1 = 1)
T:type where Cond:boolean       as type

// A subtype that only accepts a variable of the exact type
//   address V:variable integer as pointer to integer
// will accept `address X` but not `address 1`
variable T:type                 as type

// A subtype that declares a local variable of the type
//   for X:(local integer) in R:(range of integer) loop Body
// will automatically declare a local `I` in `for I in 1..5 loop ...`
local T:type                    as type

// A subtype that declares a variable of the type in enclosing scope
//   let Variable:(declaring integer) be Value:integer
declaring T:type                as type

// Subtypes that only accept mutable or constant (non-mutable) values
//    Zero:    constant integer is 0
//    Counter: mutable integer  is 1
constant T:type                 as type
mutable T:type                  as type

// Constrain the direction for argument passing for arguments
own T:type                      as type // Owned reference (non shareable)
ref T:type                      as type // Immutable reference (shareable)
in  T:type                      as type // Input only, move value to callee
out T:type                      as type // Output only, move value from callee

// Check if a type contains a value or pattern
T:type contains F:pattern       as boolean

// Operations on types
T1:type and T2:type             as type // Must belong to both types
T1:type or  T2:type             as type // Union of two types
T1:type xor T2:type             as type // One or the other
not T:type                      as type // Does not belong to the given type
T1:type & T2:type               is T1 and T2
T1:type | T2:type               is T1 or  T2
T1:type ^ T2:type               is T1 xor T2
~T:type                         is not T
!T:type                         is not T

// A specific type holding no value
nil                             as type is type(nil)

// A specific type that can hold anything, one that never matches
anything                        as type is type(Anything)
nothing                         is not anything


type pattern with
// ----------------------------------------------------------------------------
//    A `pattern` identifies specific forms in the code
// ----------------------------------------------------------------------------
    Shape                   as parse_tree
    Tree:parse_tree         as pattern // Implicitly created from parse tree


type patterns with
// ----------------------------------------------------------------------------
//    A sequence of patterns, either new-line or semi-colon separated
// ----------------------------------------------------------------------------
    Patterns                as sequence of pattern
    Seq:sequence of pattern as patterns // Implicitly created from sequence


type lifetime with
// ----------------------------------------------------------------------------
//   The lifetime for a value
// ----------------------------------------------------------------------------

    // Compiler-generated name for the lifetime
    Name                as parse_tree
