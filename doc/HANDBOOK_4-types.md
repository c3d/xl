# XL Type System

Types are annotations given to the compiler that restrict the kind
of operations that can be performed on a value. Types also implicitly
determine the size and binary representation of values. In XL, types
are defined by the shape of trees that belong to the type.

Two operators are used for type annotation, `X:T` and `X as T`.
Both are annotations indicating that `X` belongs to type `T`.

The first difference between the two type annotations is parsing
precedence. This makes it convenient to write a declaration like
   X:integer + Y:integer as integer
which means that the notation X+Y, when X and Y are integer, is
itself an integer.

The second difference is that `X as T` denotes that X is not mutable,
whereas `X : T` denotes that X is mutable.

The difference matters in particular in the interface of a type, as
declared by `with`. The non-mutable declarations using `as` are
considered as belonging only to the type, whereas mutable declarations
using `:` are considered as belonging to values of the type. As a result,
much like C++ class member declarations, "functions" or "methods"
are interpreted as belonging to the type, whereas "values" or
"members" belong to type instances.

For example, consider a `person` type declaration like the following:

   type person with
       Name : text
       Greeting as text

This means that the `Name` belongs to each value of the `person` type,
but that the `Greeting` belongs to the `person` type, not to individual
`person` instances. If `P` is a `person`, then `P.Name` depends on the
individual person, but `P.Greeting` is the same as `person.Citizenship`.

Inversely, if a declaration takes a value of the type as its first
argument (usually called `Self`), then the value can be passed
using the dot field notation. For example, consider:

   type person with
       FirstName : text
       LastName : text
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
