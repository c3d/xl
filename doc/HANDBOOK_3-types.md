# XL Type System

A type annotation like `X : integer` indicates that `X` has the
`integer` type. Types identify which operations that can be performed
on a value. For example, knowing that `X` is an `integer` allows
expression `X+X` to match declaration pattern `X:integer+Y:integer`.

In XL, types are defined by the _shape_ of trees that belong to the
type. The expression `type(Pattern)` returns the type for the given
type declaration pattern. For example, the type for all additions
where the first value is a `real` is `type(A:real+B)`.

This approach to typing means in particular that a same value can
belong to _multiple_ types. For example, the expression `2+3*5`
belongs to `type(A+B*C)`, but also to `type(A:integer+B:integer)`,
or to `infix`.

In XL, you shouldn't talk about _the_ type of a value, but rather
about _a_ type. However, in the presence of a type annotation, it is
customary to talk about _the type_ to denote the type indicated by
the annotation. For example, in the presence of an annotation like
`X:integer`, we will frequently refer to the type of `X` as being
`integer`, although the value of `X`, for example `2`, may belong to
other types such as `even` or `positive` or `type(2)` (i.e. a type
that only contains the value `2`).

## Type annotations

Two infix operators can be used for type annotations, `X:T` and `X as T`.
Both are annotations indicating that `X` belongs to type `T`.

The first difference between the two type annotations is parsing
precedence. The infix `:` has precedence higher than most operators,
whereas infix `as` has a very low precedence. In most declarations, an
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
imaginary parts can be described as follows:

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
then be used to build _type expressions_.

For example, the following code extends our previous `complex` type to
take an argument that indicates the representation for `real` numbers,
and uses that first declaration to declare two types, `complex` and
`complex32`, the latter using `real32` as a representation type for
real numbers:

```xl
type complex[real:type] is complex(Re:real, Im:real)
type complex is complex[real]
type complex32 is complex[real32]
```

> **NOTE** Type expressions play for XL the role that "class templates"
> play in C++, or "generic types" in Ada. By convention, the formal
> parameters or arguments of type expressions are placed between
> square brackets, as in `complex[real]`, although there is no
> requirement for this. In practice, exceptions are frequent, notably
> for types using operator-like notations, like `pointer to T`.


## Standard type expressions

A number of type expressions are provided by the standard library.
The most common and useful ones are:

* `nil` is a type that contains a single value, `nil`, which evaluates
  to itself. That is generally used to represent an absence of value.

* `T1` or `T2` is a type for values that belong to `T1` or to `T2`.
  It is similar to what other languages may call union types.
  For example, `integer or real` will match both `integer` and `real`
  values. Operations on `T1 or T2` will cause dynamic dispatch
  depending on the actual value being considered. For example,
  consider:
  ```
  double X:(integer or real) is X + X
  double 1      // returns 2 as an integer
  double 3.5    // returns 7.0 as a real
  ```

* `T1` and `T2` is a type for values that belong to both `T1` and
  `T2`.  For example, `number and totally_ordered` will match totally
  ordered numbers, i.e. it will not match `"ABC"` (`totally_ordered`,
  but not a `number`) nor will it match `ieee754(2.5)` (`number`, but
  not `totally_ordered`).

* `another T` is a new type that is identical to `T`, allowing overloading.
  For example, `type distance is another real` will create another `real`
  type, allowing you to forbid multiplication, and preventing errors
  such as adding a `distance` to a `real`.

  ```
  type distance is another real
  X:distance * Y:distance is compile_error "Cannot multiply distances"
  X:real as distance is compile_error "Implicit distance from real"
  syntax { POSTFIX 400 m cm mm km }
  X:real m  is distance(X)
  X:real cm is distance(X * 0.01)
  X:real mm is distance(X * 0.001)
  X:real km is distance(X * 1000.0)

  D:distance is 3.2km
  D + D     // OK: inherit X:distance+Y:distance from X:real+Y:real
  D + 1.0   // Error: Implicit distance from real
  D * D     // Error: Cannot multiply distances
  ```
  > **NOTE** The code above is incomplete, since `distance` would
  > inherit `X:integer as real`, so that `D+1` would be accepted.

* `optional T` is a shortcut for `T or nil`. This is useful for
  functions like `find` that return an optional value, and where not
  finding something is not an error but an expected result.
  > **NOTE** Compilers should perform specific optimizations such as
  > representing the value with a pointer and reserving the null
  > pointer for value `nil`.

* `fallible T` is a shortcut for `T or error`, and should be used for
  [functions that may fail](#error-handling). Unlike `nil`, an `error`
  carries a payload that gives information about the error, and can be
  used to generate an error message.

* `array[N] of T` defines a 0-based array containing `N` elements of
  type `T`. The value of `N` need not be a constant. Another variant,
  `array[A..B] of T`, allows arrays where the index is between values
  `A` and `B`, which can be any enumerated type. For example,
  `array['A'..'Z'] of boolean` provides 26 `boolean` values, indexed by
  an alphabetic letter.

* `string of T` is a variable size sequence of values with the same
  type `T`. The size of a `string` can change over its lifetime. A
  `text` may be represented as a `string of character`.

* `either Patterns` is a type that matches one of the patterns given.
  It can be used in particular for what would be called "enumerations"
  in a language like C, but is richer, much like
  [Rust enumerations](https://doc.rust-lang.org/reference/items/enumerations.html)
  ```
  type complex is either
      cartesian(Re:real, Im:real)
      polar(Mod:real, Arg:real)
  ```

* `variable T` or `var T` is a mutable version of type `T`, whereas
  `constant T` is a non-mutable version of type `T`. Only mutable
  values can be changed using the `:=` operator or their
  variants. Note that by default, formal parameters are mutable (they
  are generally specified with something like `X:integer`), but
  modifications apply to the binding in the current evaluation
  context, therefore not modifying the corresponding argument.

* `in T`, `out T` and `inout T` are types design to optimize parameter
  passing in a safe way. They indicate how you intend data to flow
  between the caller and the callee. These types also may have uses in
  data structures.

* `T in ValueList` is a subtype of `T` that only accepts values in the
  given comma-separated `ValueList`. For types that have a total
  order, `ValueList` elements can also include ranges written as
  `A..B`. For example, `integer in 1..5,9,12..20` is a type that only
  accept integer values 1 through 5, or 9, or 12 through 20.
  Similarly, `text in "One", "Two", "Three", "Four"` is a type that only
  accepts the given text strings.

These are only some common examples of type expressions. There is
nothing that prevents you from adding many others.

The case of `in T`, `out T` and `inout T`  are examples of what will
be called _ownership controlling types_, i.e. types that are dedicated
to controlling who owns what data. More details are provided in the
section on [ownership](#ownership) below.


## Ownership

The [XL execution model](HANDBOOK_2-evaluation.md) is based on a
source-like description of the evaluation context. This execution
model was described using relatively simple types for the bindings,
for example `X is 0`.

We described two
cases that correspond to immediate and lazy evaluation. This leads to
slightly different behaviour, which is exposed by the following program:

```
immediate Arg:integer is Arg + 1
lazy Arg is Arg + 1

X is 42
immediate X
lazy X
```

When evaluating `immediate X`, we need to evaluate `X` to check its
type. Therefore, the context while evaluating `immediate X` will be
`Arg is 42`, where `42` is the evaluated value of `X`.

When evaluating `lazy X` on the other hand, `X` is not evaluated, so
that the bindings will look more like:

```
Arg is CONTEXT { X }
CONTEXT is { X is 42 }
```


## Ownership

So far, we have
only described simple bindings such as `X is 0` in the context, where
copying the binding value around is inexpensive. If you call a pattern
like `foo Y:integer`, you end up with a binding like `Y is X`, and
we have not really discussed if and how that could behave differently
from a binding like `Y is 0`.

There are, however, many data types where the difference will
matter. A binding like `Y is X` is called a _reference binding_: the
value _refers_ to another value. By contrast, a binding like `Y is 0`
is called a _value binding_. The difference matters in particular if
you do something like `Y := 42`. Do you end up in a binding that is
`Y is 42`, or do you end up with a `Y is X; X is 42` binding?

Some data types, such as `string of T`, require relatively complex and
expensive memory management operations. For example, as you add
elements to a `string of integer`, the implementation of that type may
need to allocate memory to store the new values. Some other data
types, such as `array[1000] of integer`, may simply be big and
expensive to copy around. Finally, some types such as `file` may
acquire and release



For that reason, it may be necessary to optimize the generated code in
order to avoid copies and memory allocations while binding values. In
C, for example, you would pass a _pointer_ to a function when the data
being worked on is too large. The problem with this approach is well
known: ownership of the data being pointed to is unclear in
C. C++ has not improved this class of problems much. This class of
problems, collectively called "memory safety", has plagued C and C++
programs for years.

To address this problem, the Rust programming language has
[defined ownership](https://doc.rust-lang.org/1.8.0/book/ownership.html)
so that there is only one owner of any value at any time. This is done
in Rust in a way that can be fully enforced at compile-time, but
requires a somewhat complex mental gymanistics from the programmer.


rather
has a precise definition of the ownership of data. However, the XL
approach is intended to be slightly easier to use, while delivering
substantially the same benefits.



this is
done in a rather different way in XL. The rules in XL are not as
strict and constraining as in Rust, although they allow programmers to
achieve substantially the same effect, i.e. memory and thread safety
without a need for garbage collection.

### Copy or Move





copy-controlling types, which
  cause a copy when the value is initialized, when it goes out of
  scope, or in both cases. They are mostly used for function
  parameters, although they can also be used in data structures.
  ```
  increment X:inout integer is X := X+1; print_A
  print_A is print "A=", A
  A:integer := 45
  increment A   // Can print either "A=45" or "A=46" depending on copy or ref
  ```
  > **NOTE** The language makes no guarantee that the copies happen
  > _only_ when the value is created or destroyed. Typically, `inout T`
  > will perform copies only for small objects, and use references for
  > larger ones if the lifetime of the bound value allows it. The
  > compilers determines which approach is more efficient in an
  > architecture-dependent way.

  The `copy_in T`, `copy_out T` and `copy_inout T` are types that
  guarantee that copy will occur.

* `ref T` is a reference to the entity being bound, meaning that any
  change to the `ref T` value will actually modify the bound
  value. The lifetime of the bound value must dominate the lifetime of
  the `ref T` value. Mutability for the reference is the same as
  mutability for the
  ```
  // Increment in place
  increment X:ref integer is X := X+1; print_A
  print_A is print "A=", A
  A:integer := 45
  increment A   // Guaranteed to print "A=46", X is the same as A
  ```



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
