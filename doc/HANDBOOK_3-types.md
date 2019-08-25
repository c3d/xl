# XL Type System

A type annotation like `X : integer` indicates that `X` has the
`integer` type. Types identify which operations that can be performed
on a value. For example, knowing that `X` is an `integer` allows
expression `X+X` to match declaration pattern `X:integer+Y:integer`.

In XL, types are defined by the _shape_ of trees that belong to the
type. The expression `type(Pattern)` returns the type for the given
type declaration pattern. For example, the type for all additions
where the first value is a `real` is `type(A:real+B)`.

This approach to typing means in particular that, unlike many other
languages, a same value can belong to _multiple_ types. For example,
the expression `2+3*5` belongs to `type(A+B*C)`, but also to
`type(A:integer+B:integer)`, or to `infix`.


## Type annotations

Two infix operators can be used for type annotations, `X:T` and `X as T`.
Both are annotations indicating that `X` belongs to type `T`.

The first difference between the two type annotations is parsing
precedence. The infix `:` has precedence higher than most operators,
whereas infix `as` has a very low precedence. In most declaratoins, an
infix `:` is used to give a type to formal parameters, whereas an
infix `as` is used to give a type to the whole expression. This is
illustrated in the following pattern, which states that the addition
of two `integer` values is itself an `integer`:
```xl
X:integer + Y:integer as integer
```

Another difference is that `X as T`, by default, denotes that `X` is
not mutable, whereas `X : T` denotes that `X` is mutable, unless the
type explicitly specifies otherwise. For example, `seconds : integer`
declares a _variable_ named seconds, where you can store your own
seconds values, whereas `seconds as integer` declares a _function_
named seconds, computing the number of seconds in the current time, or
some other value that you cannot modify.


## Type declarations

Like other XL values, a type can be given a name. For example, a
`complex` type made of two `real` numbers representing the real and
imaginary parts can be described using the following code:

```xl
complex is type(complex(Re:real, Im:real))
```

This declaration means that any parse tree like `complex(1.3,2.5)`
will match the `complex` type.

There is a shortcut notation for declaring types, where the `type`
word can be placed in the pattern instead of in the body of the
definition. This is nothing more than syntactic sugar for readability.
The previous example should be written as follows:

```xl
type complex is complex(Re:real, Im:real)
```


## Type expressions

A type declaration is like any other XL declaration. It can have parameters,
including parameters with the `type` type, and such declarations can
then be used to build _tpye expressions_.

For example, the following code extends our previous `complex` type to
take an argument that indicates the representation for `real` numbers,
and uses that first declaration to declare two types, `complex` and
`copmlex32`, the latter using `real32` as a representation type for
real numbers:

```xl
type complex[real:type] is complex(Re:real, Im:real)
type complex is complex[real]
type complex32 is complex[real32]
```

> **NOTE** Type expressions play for XL the role that "class templates"
> play in C++, or "generic types" in Ada.


## Standard type expressions

A number of type expressions are provided by the standard library.
The most important ones are:

* `nil` is a type that contains a single value, `nil`, used to
  represent an absence of value: `type nil is nil`.

* `T1` or `T2` is a type for values that belong to `T1` or to `T2`.
  It is similar to what other languages may call union types.
  For example, `integer or real` will match both `integer` and `real`
  values.

* `T1` and `T2` is a type for values that belong to both `T1` and `T2`.
  For example, `number and ordered` will match ordered numbers.

* `another T` is a new type that is identical to `T`, allowing overloading.
  For example, `type distance is another real` will create another `real`
  type, allowing you to forbid multiplication, and preventing errors
  such as adding a `distance` to a `real`.

* `optional T` is a shortcut for `T or nil`. This is useful for
  functions like `find` that return an optional value, and where not
  finding something is not an error but an expected result.

* `fallible T` is a shortcut for `T or error`, and should be used for
  [functions that may fail](#error-handling).

* `either Patterns` is a type that matches one of the patterns given.
  It can be used in particular for what would be called "enumerations"
  in a language like C, but is richer, much like "enumerations" in Rust.

* `variable T` or `var T` is a mutable version of type `T`.

* `constant T` is a non-mutable version of type `T`.




## Type hierarchy




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
