# XL Type System

XL types are a way to organize values by restricting which
operations can be selected during evaluation. For example, knowing
that `A` is a `real` allows expression `A+A` to match declaration
pattern `X:real+Y:real`, but prevent it from matching pattern
`X:integer+Y:integer`.

In XL, types are based on the _shape_ of
[parse trees](HANDBOOK_1-syntax.md#the-xl-parse-tree).
A type identifies the tree patterns that belong to the type.
The expression `type(Pattern)` returns the type for the given type
declaration pattern. For example, the type for all additions where the
first value is a `real` is `type(A:real+B)`.

This approach to typing means in particular that a same value can
belong to _multiple_ types. For example, the expression `2+3*5`
belongs to `type(A+B*C)`, but also to `type(A:integer+B:integer)`,
or to `infix`.

Therefore, for XL, you shouldn't talk about _the_ type of a value, but
rather about _a_ type. However, in the presence of a type annotation,
it is customary to talk about _the type_ to denote the single type
indicated by the annotation. For example, for `X:integer`, we will
ordinarily refer to the type of `X` as being `integer`, although the
value of `X`, for example `2`, may also belong to other types such as
`even_integer` or `positive_integer` or `type(2)`, a type that
only contains the value `2`.


## Type annotations

A type can be associated to a name using a _type annotation_.
For example, a type annotation such as `X:integer` indicates that the
values that can be bound to the name `X` must belong to the `integer`
type.

Two infix operators can be used for type annotations, `X:T` and `X as T`.
Both are annotations indicating that `X` belongs to type `T`.
Typical usage for these two kinds of annotations is illustrated below,
indicating that the `<` operator between two `integer` values has the
`boolean` type:

```
X:integer < Y:integer as boolean
```

The first difference between the two kinds of type annotations is parsing
precedence. The infix `:` has precedence higher than most operators,
whereas infix `as` has a very low precedence. In most declarations, an
infix `:` is used to give a type to formal parameters, whereas an
infix `as` is used to give a type to the whole expression. This is
illustrated in the example above, where `X:integer` and `Y:integer`
define the types of the two formal parameters `X` and `Y` in the
pattern `X < Y`, and the `as boolean` part indicates that the result of an
operation like `3 < 5` has the `boolean` type.

Another difference is [mutability](#mutability). If type `T` is not
explicitly marked as `constant` or `variable`, `X:T` indicates that
`X` is mutable, whereas `X as T` indicates that `X` is not mutable.
For example, `seconds : integer` declares a _variable_ named seconds,
where you can store your own seconds values, whereas `seconds as integer`
declares a _function_ named seconds, possibly returning the number of
seconds in the current time from some real-time clock.


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

A declaration `type T is P` is equivalent to `T is type (P)`. This is
important to remember if you write type expressions. For example:

```xl
// This is `type(integer)`, which only accepts the name `integer`
type int is integer

// This is `type(X:integer8)`, which accepts `integer8` values
type int8 is X:integer8

// This creates an alternate name for `unsigned`
positive is unsigned
```

## Type-related concepts

A number of essential concepts are related to the type system, and
will be explained more in details below:

* the [lifetime](#lifetime) of a value is the amount of time during
  which the value exists in the program. Lifetime is, among other
  things, determined by [scoping](HANDBOOKE_2-evaluation.md#scoping).
* [creation](#creation) and [destruction](#destruction)
  defines how values of a given type are initialized and destroyed.
* [errors](#errors) are special types used to indicate failure.
* [mutability](#mutability) is the ability for an entity to change
  value over its lifetime.
* [compactness](#compactness) is the property of some types to have
  their values represented in the machine in a compact way, i.e. a
  fixed-size sequence of consecutive memory storage units
  (most generally bytes).
* [ownership](#ownership) is a properties of some types to control the
  lifetime of the associated values or possibly some other resource
  such as a network connection. Non-owning types can be used to
  [access](#access) values of an associated owning type.
* [inheritance](#inheritance) is the ability for a type to inherit
  all operations from another type, so that its values can safely be
  implicitly converted to values of that other type.
* the [interface](#interface) of a type is an optional scope that
  exposes _fields_ of the type, i.e. individually accessible values.
  The _implementation_ of the type must provide all interfaces exposed
  in the type's interface.
* [copy](#copy), [move](#move) and [binding](#binding) are operations
  used to transfer values across parts of a program.
* [atomicity](#atomicity) is the ability to perform operations in a
  way that allows consistent behavior across multiple threads of
  execution, possibly executing concurrently on different CPUs.


### Lifetime

The lifetime of a value is the amount of time during which the value
exists in the program, in other words the time between its
[creation](#creation) and its [destruction](#destruction).

An entity is said to be _live_ if it was created but not yet
destroyed. It is said to be _dead_ otherwise.

> **NOTE** Some entities may be live but not accessible from within
> the current context because they are not visible. This is the case
> for variables declared in the caller's context.

The lifetime information known by the compiler about entity `X` is
represented as compile-time constant `lifetime X`. The lifetime
values are equipped with a partial order `<`, such that the expression
`lifetime X < lifetime Y` being `true` is a compiler guarantee that
`Y` will always be live while `X` is live. It is possible for neither
`lifetime X < lifetime Y` nor `lifetime X > lifetime Y` to be
true. This `lifetime` feature is used to implement
[Rust-like](https://doc.rust-lang.org/1.8.0/book/ownership.html)
[restrictions on access types](HANDBOOK_4-compilation.md#lifetime),
i.e. a way to achieve memory safety at zero runtime cost.

The lifetime of XL values fall in one of the following categories:

* _Global_ entities are live at least as long as they are visible.
  This includes builtin-enttiies, entitites declared in the top-level
  of the modules used by the program, and most entities created by the
  compiler itself. The compiler can generally assign preallocated
  storage to such entities, at compilation time.

* _Temporary values_ hold the result of evaluation of functions.
  They are created in the called function, and [copied](#copy) or
  [moved](#move) to the function caller. The temporary value is
  destroyed before the end of the statement, and possibly as early as
  it is no longer used. In the following example, the value of `x*3`
  can be destroyed as soon as the expression `x*3+5` is computed.

  ```
  f(x) is (x*3+5)/2
  ```

  Such temporary values are typically stored in registers or on the
  stack, although some temporary values may require heap storage that
  will be freed when the value is destroyed.

* _Named constants_ have a lifetime that corresponds to their
  [scope](HANDBOOKE_2-evaluation.md#scoping).
  As long as the named constant is visible, it exists. In the
  following example, the value of `DEGREE_TO_RADIAN`, `2 * pi / 180`
  exists for the duration of the `cos_degrees` function:

  ```
  cos_degrees X is
     DEGREE_TO_RADIAN is 2 * pi / 180
     cos(X * DEGREE_TO_RADIAN)
  ```

  The compiler has a lot of freedom on how to implement named
  constants, and may use preallocated storage, functions, or immediate
  constants depending on the need.

* _Variables_ have a lifetime that generally corresponds to their
  [scope](HANDBOOKE_2-evaluation.md#scoping), but the value of their
  lifetime terminates each time the value is updated. In the following
  example, `Message` is created with value `"Hello"`, but on the
  second line, that value is destroyed to be replaced with value
  `"Hello World"`.

  ```
  Message : text := "Hello"
  Message := Message & " World"
  ```

  Except for global variables, variables are usually stored on the
  stock or in registers.

* _Dynamic values_ require dynamic storage, generally in a heap.
  The lifetime of such values is normally controlled by the
  values used to access the storage. With the exception of data types
  used to access data not owned by the XL program (e.g. data allocated
  from another language), XL ownership rules ensure that dynamic
  values are destroyed as soon as they can no longer be accessed.

  For example, the code below creates a `string of integer`, which
  uses dynamically allocated storage, to hold an arbitrary large
  sequence of `integer` values. Thus, the `string of integer` value
  extends the lifetime of all values geneated in the sequence.
  However, it also guarantees that these values are destroyed when the
  `string of integer` value itself is no longer needed.

  ```
  syracuse N:integer as string of integer is
      loop
          result := result & N
          N := if N mod 2 = 0 then N/2 else N*3+1
      until N = 1
  ```

  Dynamic data is normally stored on a standard heap, but XL
  provides hooks that make it possible to provide your own allocation
  for data storage.


### Creation

_Creation_ is the process of preparing a value for use. The XL
compiler ensures that specific rules are followed to invoke creation
code provided by the programmer before any other possible use of the
value being created.

When you define a type, you need to specify the associate shape. For
example, we defined a `complex` type as follows:

```xl
type complex is complex(Re:real, Im:real)
```

This means that a shape like `complex(2.3, 5.6)` is a `complex`. This
also means that the _only_ elementary way to create a `complex` is by
creating such a shape. It is not possible to have an uninitialized
element in a `complex`, since for example `complex(1.3)` would not
match the shape and not have the right type.

Using the shape explicitly given for the type is called the _constructor_
for the type. A constructor can never fail nor build a partial
object. If an argument returns an [error](#errors) during evaluation,
then that `error` value will not match the expected argument, except
naturally if the constructor is written to accept `error` values.

Often, developers will offer alternate ways to create values of a
given type. These alternate helpers are nothing else than regular
definitions that return a value of the type.

For example, for the `complex` type, you may create an imaginary unit,
`i`, but you need a constructor to define it. You can also recognize
common expressions such as `2+3i` and turn them into constructors.

```xl
i   is complex(0.0, 1.0)

syntax { POSTFIX 190 i }
Re:real + Im:real i                 is complex(Re, Im)      // Case 1
Re:real + Im:real * [[i]]           is complex(Re, Im)      // Case 2
Re:real + [[i]] * Im:real           is complex(Re, Im)      // Case 3
Re:real as complex                  is complex(Re, 0.0)     // Case 4
X:complex + Y:complex as complex    is ...

2 + 3i              // Calls case 1 (with explicit concersions to real)
2 + 3 * i           // Calls case 2 (with explicit conversions to real)
2 + i * 3           // Calls case 3
2 + 3i + 5.2        // Calls case 4 to convert 5.2 to complex(5.2, 0.0)
2 + 3i + 5          // Error: Two implicit conversions (exercise: fix it)
```

<a name="my_file"/>

A type implementation may be _hidden_ in a [module interface](HANDBOOK_6-modules.md),
in which case the module interface should also provide some functions
to create elements of the type. The following example illustrates this
for a `file` interface based on Unix-style file descriptors:

```xl
module MY_FILE with
    type file
    open Name:text as file
    close F:file

module MY_FILE is
    type file is file(fd:integer)
    open Name:text as file is
        fd:integer := libc.open(Name, libc.O_RDONLY)
        file(fd)
    close F:inout file is
        if fd >= 0 then
            libc.close(F.fd)
            F.fd := -2
    delete F:inout file is close F    // Destruction, see below
```

> **RATIONALE** This mechanism is similar to _elaboration_ in Ada or
> to _constructors_ in C++. It makes it possible for programmers to
> provide strong guarantees about the internal state of values before
> they can be used. This is a fundamental brick of programming
> techniques such as encapsulation, programming contracts or
> [RAII](https://en.wikipedia.org/wiki/Resource_acquisition_is_initialization).


### Destruction

When the lifetime of a value `V` terminates, the statement `delete V`
automatically evaluates. Declared entites are destroyed in the reverse
order of their declaration.  A `delete X:T` definition is called a
_destructor_ for type `T`. It often has an [inout](#inout) parameter
for the value to destroy, in order to be able to modify its argument,
i.e. a destructor often has a signature like `delete X:*T`.

There is a built-in default definition of that statement that has
no effect and matches any value:

```xl
delete Anything is nil
```

There may be multiple destructors that match a given expression. When
this happens, normal lookup rules happen. This means that, unlike
languages like C++, a programmer can deliberately override the
destruction of an object, and remains in control of the destruction
process.

> **RATIONALE** In XL, multiple patterns can match a given value.
> It might seem desirable to call all the patterns that match, but not
> only would it introduce a special-case lookup, it would also be
> extremely dangerous in a number of easily identified cases.
> As an illustration, consider the following code:
> ```xl
> delete F:*file when F.fd < 0  is ... // Case 1
> delete F:*file                is ... // Case 2
> ```
> Clearly, the intent of the programmer is to special-case the
> destruction of `file` values that have an invalid file descriptor,
> for example as a result of an error condition from the C `open`
> call (which returns `-1` on error).

It is possible to create local destructor definitions. When such a
local definition exists, it is possible for it to override a more
general definition. The general definition can be accessed using
[super lookup](HANDBOOK_2-evaluation.md#super-lookup).

```xl
show_destructors is
    delete Something is
        print "Deleted", Something
        super.delete Something
    X is 42
    Y is 57.2
    X + Y
```

This should output something similar to the following:

```xl
Deleted 42.0
Deleted 57.2
Deleted 42
```

The first value being output is the temporary value created by the
necessary implicit conversion of `X` from `integer` to `real`. Note
that additional temporary values may appear depending on the
optimizations performed by the compiler. The value returned by the
function should not be destroyed, since it's passed to the caller.

Any destruction code must be able to be called multiple times with the
same value, if only because you cannot prevent a programmer from
writing:

```xl
delete Value
```

In that case, `Value` will be destroyed twice, once by the explicit
`delete`, and a second time when `Value` goes out of scope. There is
obviously no limit on the number of destructions that an object may go
through.

```
for I in 1..LARGE_NUMBER loop
    delete Value
```

Also, remember that assigning to a value implicitly destroys the target of
the assignment.


### Errors

Errors in XL are represented by values with the `error` type (or any
type that can be implicitly converted to `error`). The error type has
a constructor that takes a simple error message:

```xl
type error is error Message:text
```

A function that may fail will often have an `error or T` return value.
There is a specific shortcut for that, `fallible T`. For example, a
logarithm returns an error for non-positive values, so that the
signature of the `log` functions is:

```xl
log X:real as fallible real     is ... // May return real or error
```

If possible, error detection should be pushed to the interface of the
function. For the `log` function, it is known to fail only for
negative or null values, so that a better interface would be:

```xl
log X:real as real  when X > 0.0    is ... // Always return a real
log X:real as error                 is ... // Always return an error
```

 A benefit of writing code this way is that the compiler can more
 easily figure out that the following code is correct and does not
 require any kind of error handling:

 ```xl
 if X > 0.0 then
     print "Log(", X, ") is ", log X
 ```

> **RATIONALE** By returning an `error` for failure conditions, XL
> forces the programmer to deal with errors. They cannot simply be
> ignored like C return values or C++ exceptions can be. Errors that
> may possibly return from a function are a fundamental part of its
> type, and error handling is not optional.

A number of types [derive](#inheritance) from the base `error` type to
feature additional properties:

* A `compile_error` helps the compiler emit better diagnostic for
  situations which would lead to an invalid program.
  ```xl
  // Emit a specific diagnostic when writing a real into an integer
  X:integer := Y:real       is compile_error "Possible truncation"
  ```

* A `range_error` indicates that a given value is out of range.
  The default message provided is supplemented with information
  comparing the value with the expected range.
  ```xl
  T:text[A:integer] as character or range_error is
      if A < 0 or A >= length T then
          range_error "Text index is out of bounds", A, T
      else
          P : memory_address[character] := memory_address(T.first)
          P += A
          *P
  ```

* A `logic_error` indicates an unexpected condition in the program,
  and can be returned by `assert`, `require` and `ensure`.
  ```xl
  if X > 0 then
      print "X is positive"
  else if X < 0 then
      print "X is negative"
  else
      logic_error "I never considered that case"
  ```


### Mutability

A value is said to be _mutable_ if it can change during its lifetime.
A value that is not mutable is said to be _constant_. A mutable named
entity is called a _variable_. An immutable named entity is called a
_named constant_.

The `X:T` type annotations indicates that `X` is a mutable value of
type `T`, unless type `T` is explicitly marked as constant. When `X`
is a name, the annotation declares that `X` is a variable.
The `X as T` type annotation indicates that `X` is a constant value of
type `T`, unless type `T` is explicitly marked as variable. When `X`
is a name, this may declare either a named constant or a function
without parameters, depending on the shape of the body.

```xl
StartupMessage : text := "Hello World"  // Variable
Answer as integer is 42                 // Named constant
```

A mutable value can be initialized or modified using the `:=`
operator, which is called an
[_assignment_](HANDBOOK_2-evaluation.md##assignments-and-moves).
There are a number of derived operators, such as `+=`, that combine a
frequent arithmetic operation and an assignment.

```xl
X : integer := 42       // Initialize with value 42
X := X or 1             // Binary or, X is now 43
X -= 1                  // Subtract 1 from X, now 42
```

Some entities may give [access](#access) to individual inner
values. For example, a `text` value is conceptually made of a number
of individual `character` values that can be accessed individually.
This is true irrespective of how `text` is represented. In addition,
a slice of a `text` value is itself a `text` value. The mutability of
a `text` value obviously has an effect on the mutability of accessed
elements in the `text`.

The following example shows how `text` values can be mutated directly (1),
using a computed assignment (2), by changing a slice (3) or by
changing an individual element (4).

```xl
Greeting : text := "Hello"              // Variable text
Person as text is "John"                // Constant text
Greeting := Greeting & " " & Person     // (1) Greeting now "Hello John"
Greeting &= "!"                         // (2) Greeting now "Hello John!"
Greeting[0..4] := "Good m0rning"        // (3) Greeting now "Good m0rning John!"
Greeting[6] := 'o'                      // (4) Greeting now "Good morning John!"
```

None of these operations would be valid on a constant text such as
`Person` in the code above. For example, `Person[3]:='a'` is
invalid, since `Person` is a constant value.

> **NOTE** In the case (3) above, modifying a `text` value through an
> access type can change its length. This is possible because
> `Greeting[0..4]` is not an independent value, but an access type,
> specifically a `slice`, which keeps track of both the `text`
> (`Greeting` here) and the index range (`0..4` in that case),
> with a `:=` operator that modifies the accessed `text` value.

A constant value does not change over its lifetime, but it may change
over the lifetime of the program. More precisely, the lifetime of a
constant is at most as long as the lifetime of the values it is
computed from. For example, in the following code, the constant `K`
has a different value for every interation of the loop, but the
constant `L` has the same value for all iterations of `I`

```xl
for J in 1..5 loop
    for I in 1..5 loop
        K is 2*I + 1
        L is 2*J + 1
        print "I=", I, " K=", K, " L=", L
```

> **RATIONALE** There is no syntactic difference between a constant
> and a function without parameters. An implementation should be free
> to implement a constant as a function if this is more effective, or
> to use smarter strategies when appropriate.

### Compactness

Some data types can be represented by a fixed number of contiguous
memory locations. This is the case for example of `integer` or `real`:
all `integer` values take the same number of bytes. Such data types
are called _compact_.

On the other hand, a `text` value can be of any length, and may
therefore require a variable number of bytes to represent values such
as `"Hi"` and `"There once was a time where text was represented in
languages such as Pascal by fixed-size character array with a byte
representing the length. This meant that you could not process text
that was longer than, say, 255 characters. More modern languages have
lifted this restriction."`. These values are said to be _scattered_.

Scattered types are always built by _interpreting_ compact types. For
example, a representation for text could be made of two values, the
memory address of the first character, and the size of the text. This
is not the only possible representation, of course, but any
representation require interpreting fixed-size memory locations and
giving them a logical structure.

Although this is not always the case, the assignment for compact types
generally does a [copy](#copy), while the assignment for scattered
types typically does a [move](#move).


### Ownership

Computers offer a number of resources: memory, files, locks, network
connexions, devices, sensors, actuators, and so on. A common problem
with such resources is to control their _ownership_. In other words,
who is responsible for a given resource at any given time.

In XL, like in languages like Rust or C++, ownership is largely
determined by the type system, and relies heavily on the guarantees
it provides, in particular with respect to [creation](#creation) and
[destruction](#destruction). In C++, the mechanism is called
[RAII](https://en.wikipedia.org/wiki/RAII), which stands for _Resource
Acquisition is Initialization_. The central idea is that ownership of
a resource is an invariant during the lifetime of a value. In other
words, the value gets ownership of the resource during construction,
and releases this ownership during destruction. This was illustrated
in the `file` type of the module `MY_FILE` [given earlier](#my_file).

Types designed to own the associated value are called _owner types_.
There is normally at most one live owner at any given time for each
controlled resource, that acquired the resource at construction time,
and will release it at destruction time. It may be possible to release
the owned resource early using `delete Value`.

The [standard library](HANDBOOK_7-standard-library.md) provides a
number of types intended to own common classes of resources, including:

* An `array`, a `buffer` and a `string` all own a contiguous sequence
  of items of the same type.
  * An `array` has a fixed size during its lifetime and allocates
    items directly, e.g. on the execution stack.
  * A `buffer` has a fixed size during its lifteime, and allocates
    items dynamically, typically from a heap.
  * A `string` has a variable size during its lifetime, and
    consequently may move items around in memory as a result of
    specific operations.
* A `text` owns a variable number of `character` items, being
  equivalent to `string of character`.
* A `file` owns an open file.
* A `mutex` owns execution by a single thread while it's live.
* A `timer` owns a resource that can be used to measure time and
  schedule execution.
* A `thread` owns an execution thread and the associated call stack.
* A `task` owns an operation to perform that can be dispatched to one
  of the available threads of execution.
* A `process` owns an operating system process, including its threads
  and address space.
* A `context` captures an execution context.
* An `own` value owns a single item allocated in dynamic storage, or
  the value `nil`.


### Access

Not all types are intended to be owner types. Many types delegate
ownership to another type. Such types are called _access types_.
When an access type is destroyed, the resources that it accesses are
_not_ disposed of, since the access type does not own the value.
A value of the acces type merely provides _access_ to a particular
value of the associated owner type.

For example, if `T` is a `text` value and if `A` and `B` are `integer`
values, then `T[A..B]` is a particular kind of access value called a
_slice_, which denotes the fragment of text between `0`-based positions
 `A` and `B`. By construction, slice `T[A..B]` can only access `T`,
 not any other `text` value. Similarly, it is easy to implement bound
 checks on `A` and `B` to make sure that no operation ever accesses
 any `character` value outside of `T`. As a result, this access value
 is perfectly safe to use.

Access types generalize _pointers_ or _references_ found in other
languages, because they can describe a much wider class of access
patterns. A pointer can only access a single element, whereas access
types have no such restriction, as the `T[A..B]` example demonstrates.
Access types can also enforce much stricter ownership rules than mere
pointers.

> **NOTE** The C language worked around the limitation that pointers
> access a single element by abusing so-called "pointer arithmetic",
> in particular to implement arrays. In C, `A[I]` is merely a shortcut
> for `*(A+I)`. This means that `3[buffer]` is a valid way in C to
> access the third element of `buffer`, and that there are scenarios
> where `ptr[-1]` also makes sense as a way to access the element that
> precedes `ptr`. Unfortunately, this hack, which may have been cute
> when machines had 32K of memory, is now the root cause of a whole
> class of programming errors known as _buffer overflows_, which
> contribute in no small part to the well-deserved reputation of C as
> being a language that offers no memory safety whatsoever.

The [standard library](HANDBOOK_7-standard-library.md) provides a
number of types intended to access common owner types, including:

* A `slice` can be used to access range of items in contiguous
  sequences, including `array`, `buffer` or `string` (and therefore
  `text`, which is `string of character`).
* A `reader` or a `writer` can be used to access a `file` either for
  reading or writing.
* A `lock` takes a `mutex` to prevent multiple threads from executing
  a given piece of code.
* Several types such as `timing`, `dispatch`, `timeout` or
  `rendezvous` will combine `timer`, `thread`, `task` and `context`
  values.
* A `ref` is a reference to a live `own` value.
* The `in`, `out` and `inout` type expressions can sometimes be
  equivalent to an access types if that is the most efficient way to pass
  an argument around. However, this is mostly invisible to the programmer.
* A `memory_address` references a specific address in memory, and is
  the closest there is in XL to a raw C pointer. It is purposely
  verbose and cumbersome to use, so as to discourage its use when not
  absolutely necessary.


### Inheritance

### Interface

The [interface](HANDBOOK_2-evaluation.md#interface-and-implementation)
of a type can specifiy a [scope](HANDBOOK_2-evaluation.md#scoping)
for values that match the type, using the syntax `type T with I`,
where `I` is a scope containing the publicly available declarations.
These declarations are called _fields_ of the type when they denote
[mutable](#mutability) values, and _members_ of the type if they are
constant.

The code below defines a `picture` type that exposes `width`, `height`
and `data` fields, as well as a `size` member that is used to compute
the size of the `data` buffer.

```xl
type picture with
    width  : unsigned
    height : unsigned
    data   : buffer[size] of unsigned8
    size as unsigned
```

Note that only knowing the interface of a type does not allow values
of the type to be created. Typically, the interface of a function
making it possible to create values will also be provided. In the rest
of the discussion for the `picture` type, we will also assume that
there is a `create_picture` function with the following interface:

```xl`
picture(width:unsigned, height:unsigned) as picture
```

#### Information hiding

The interface does not reveal any information on the actual shape of
the parse tree for `picture` values. In other words, it does not
specify how the `picture` type is actually implemented. A type that
has a name but no implementation, like `picture` above, is called a
_tag type_. A tag type can only match values that were _tagged_ with
the same type using some explicit type annotation.

The type interface above remains sufficient to validate code like the
following definition of `is_square`:

```xl
is_square P:picture is P.width = P.height
```

In that code, `P` is properly tagged as having the `picture` type, and
even if we have no idea how that type is implemented, we can still use
`P.width` and deduce that it's an `integer` value based on the
type interface alone.

#### Anonymous scope implementation

The simplest way to implement fields is to create a type that has
a structure exposing declarations that directly match the
interface. For the `picture` type, this could be the following code:

```xl
type picture is
    width   : unsigned
    height  : unsigned
    data    : buffer[size] of unsigned8
    size is width * height
```

Remember that this is equivalent to:

```xl
picture is type
    width   : unsigned
    height  : unsigned
    data    : buffer[size] of unsigned8
    size is width * height
```

This implementation of the `picture` type is a pattern that matches
values that directly match the pattern, such as:

```xl
my_picture is
    1024
    768
    my_buffer
```

For better readability, the pattern can also
[match a scope](HANDBOOK_2-evaluation.md#pattern-matching-scope-values)
```xl
another_picture is
    width  is 1024
    height is 768
    buffer is another_buffer
```

#### Named scope implementation

In general, you want the pattern to be more specific, so it is
customary to add a prefix that matches the type name and add infix `,`
operators separating the values, therefore creating a [constructor](#creation).

```xl
type picture is picture
    width   : unsigned,         // Notice the comma
    height  : unsigned,         // Here too
    data    : buffer[size] of unsigned8
    size is width * height

my_picture is picture(1024, 768, my_buffer)

another_picture is picture
    width  is 256
    height is 256
    data   is another_buffer
```

#### Indirect implementation

However, the implementation is often entirely different, and merely
needs to _expose_ the interface in some way. This is called an
_indirect implementation_ of the interface.

For example, the `picture` type can be implemented by _delegating_
the implementation to another value that provides the required
information. For the sake of illustration, we will imagine that we use
a `bitmap` type defined as follows:

```xl
type bitmap with
    width  : unsigned16
    height : unsigned16
    buf    : array[width, height] of unsigned8
```

This means that the implementation of the `picture` type must perform
some adjustments in order to delegate the work to the underlying
`bitmap` value.

```xl
type picture is picture
    Bitmap:bitmap
    buffer:optional[buffer[size] of unsigned8]

(P:picture).width   is P.Image.width
(P:picture).height  is P.Image.height
(P:picture).buffer  is P.Image.buffer

```


### Copy

The [assignment operator](HANDBOOK_2-evaluation.md#assignments-and-moves)
is written `A := B` in XL. For compact types, this is normally
equivalent to `A :+ B`, which is guaranteed to be a _copy_.

### Move


### Binding

#### `in` arguments

#### `out` arguments

#### `inout` arguments



### Atomicity


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

* `T1 or T2` is a type for values that belong to `T1` or to `T2`.
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

* `T1 and T2` is a type for values that belong to both `T1` and
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
  variants.
  > **NOTE** By default, formal parameters are mutable, since
  > they are generally specified with something like `X:integer`, but
  > modifications apply to the binding in the current evaluation
  > context, therefore not modifying the corresponding argument.

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



### Copy or Move


### Variant types

```xl
type picture with
    width  : unsigned
    height : unsigned
    format : either { RGB; GRAY }
    data   : buffer
    size is width * height
    type grayscale is fixed_point range 0.0..1.0 bits 8
    type buffer is buffer[1..size] of pixel

    type pixel is pixel[format]
    type pixel[RGB] is rgb(red   : grayscale,
                           green : grayscale,
                           blue  : grayscale)
    type pixel[GRAY] is gray(gray: grayscale)
```



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



-------------------------------------------------------------

### MOSTLY JUNK BELOW, IGNORE (IDEAS SCRATCHPAD)

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
