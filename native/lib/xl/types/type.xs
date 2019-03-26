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

    The difference matters in particular in the interface of a type, as
    declared by `with`. The non-mutable declarations using `as` are
    considered as belonging only to the type, whereas mutable declarations
    using `:` are considered as belonging to values of the type. As a result,
    much like C++ class member declarations, "functions" belong to the type,
    whereas "values" belong to type instances.

    For example, consider a `person` type declaration like the following:

        type person with
            Name : text
            Citizenship as text

    This means that the `Name` belongs to each value of the `person` type,
    but that the `Citizenship` belongs to the type, not to values. If `P`
    is a `person`, then `P.Name` depends on the person, but `P.Citizenship`
    is really a shortcut for `person.Citizenship`.

    However, note that for declarations that take a value of the type as
    their first argument, that value can be passed when using the dot
    field notation above. It is often called `Self` to indicate that
    intent. For example, if you have

        type person with
            First : text
            Last  : text
            FullName Self:person as text

    In that case, it is possible to write `P.FullName` which will be a
    shortcut for `person.FullName P`.

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

    A type has an *interface* and an *implementation*. An interface is
    described using the `with` operator, whereas an implementation is given
    using the `is` operator. The compiler checks that the implementation
    matches the interface, but there are many ways to implement the interface.

    Consider for example the following interface:
        type complex with
            Re          : real
            Im          : real
            Modulus     : real
            Argument    : real

    This does not imply anything about the actual representation of
    complex numbers. It only implies that if `Z` is a complex number,
    it is possible to read and write all four fields.

    A valid implementation of this type could be storing data in cartesian
    form and performing computation when reading or writing Modulus and
    Arguments. It could also switch back and forth between polar and cartesian
    form based on actual field accesses, and have an implementation that
    looks like:
        type complex is either
            cartesian   Re:real, Im:real
            polar       Modulus:real, Argument:real
    The latter is closer to how the type is actually implemented in the
    standard library
*/


use BITWISE, MEMORY, TEXT, BOOLEAN

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
    Matches Value:anything      as boolean

    // Return conversion function to convert to another type, if any
    type conversion             is optional (Value:anything -> target)
    Convert target:type         as conversion

    // Return the N-th operation for that type, in declaration order
    type operation              is optional code
    Operation Index:unsigned    as operation


// Return a type matching the pattern
type Form:pattern               as type


// Static subtype without any automatic conversion (must be explicit)
//   unix_file_descriptor is another integer
another T:type                  as type

// Dynamic reference subtype preserving the original type of the value
//   intersect (S1:any shape, S2:any shape) as any shape
any T:type                      as type

// A type that is either a value of the type of a given value, or nil
optional T:type                 as type         is T or nil

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

// A specific type holding no value
nil                             as type is type(nil)

// A specific type that can hold anything, one that never matches
anything                        as type is type(Anything)
nothing                         is not anything



type lifetime with
// ----------------------------------------------------------------------------
//   The lifetime for a value
// ----------------------------------------------------------------------------

    // Compiler-generated name for the lifetime
    Name                as parse_tree
