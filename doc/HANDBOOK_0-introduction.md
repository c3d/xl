# Introduction to XL

Extensible? What does that mean for a programming language?
For XL, it really means three things:

1. XL has a method to extend the language with any kind of feature, not
   just functions or data types, but also programming constructs, optimizations,
   domain-specific notations, and more. Actually, all this is done
   with a [single operator](#one-operator-to-rule-them-all), `is`,
   called the _definition operator_.

2. As a validation of the concept, most features that are built-in in
   other programming languages, like the `while` loop, or integer
   arithmetic, are _constructed_ in XL. Specifically, they are
   provided by the [standard library](#the-standard-library), using
   techniques that you, yourself, can use in your program. This,
   obviously, means that you can add your own loops, or your own
   machine-level data types, and even extend existing ones.

3. XL gives you [complete control](#efficient-translation) over the
   program translation process.  This means that libraries exist or
   can be written to make XL at least as good as C for low-level
   bit-twiddling, at least as good as C++ for generic algorithms, at
   least as good as Ada for tasking, at least as good as Fortran for
   numerical algorithms, at least as good as Java for distributed
   programming, and so on.

This may all seem too good to be true. This document explains how the
magic happens. But first of all, one thing that really matters: XL is
supposed to be _simple_. Let's start with a few well-known examples to
prove this.


## Two basic examples

It is practically mandatory to begin the presentation of any
programming language with a ["Hello World"](https://en.wikipedia.org/wiki/%22Hello,_World!%22_program) example,
immediately followed by a a recursive definition of the
[factorial function](https://en.wikipedia.org/wiki/Factorial). Let's
follow this long honored tradition.

### Hello World

In XL, a program that prints `Hello World` on the terminal console
output will look like this:

```xl
import XL.CONSOLE.TEXT_IO
write_line "Hello World"
```

The first line _imports_ the `XL.CONSOLE.TEXT_IO` module. The program
can then use the `write_line` function from that module to write the
text on the terminal console.

Why do we need the `import` statement? There is a general rule in XL
that you only pay for things that you use. Not all programs will use a
terminal console, so the corresponding functions must be explicitly
imported into your program. It is possible that some systems, like
embedded systems, don't even have a terminal console. On such a
system, the corresponding module would not be available, and the
program would properly fail to compile.

What is more interesting, though, is the definition of `write_line`.
That definition is [discussed below](#the-case-of-text-input-output-operations),
and you will see that it is quite simple.


### Factorial

A program computing the factorial of numbers between 1 and 5, and then
showing them on the console, would be written as follows:

```xl
import IO = XL.CONSOLE.TEXT_IO

0! is 1
N! is N * (N-1)!

for I in 1..5 loop
    IO.write_line "The factorial of ", I, " is ", I!
```

We have used an alternative form of the `import` statement, where the
imported module is given a local nick-name, `IO`. This form is useful
when it's important to avoid the risk of name collisions between
modules. In that case, you need to refer to the `write_line` function
of the module as `IO.write_line`.

The definition of the factorial function shows how expressive XL is,
making it possible to use the well-known notation for the factorial
function. The definition consists in two parts:

* the special case of the factorial of `0` is defined as follows:
  ```xl
  0! is 1
  ```
* the general case is defined as follows, and involves a recursion
  in the form of the `(N-1)!` expression:
  ```xl
  N! is N * (N-1)!
  ```

That definition would not detect a problem if you wrote `-3!`.
The second form would match, and presumably enter a recursion that
would exhaust available stack space. It is possible to fix that
problem by indicating that the definition only works for positive numbers:

```xl
0!              is 1
N!  when N > 0  is N * (N-1)!
```

Writing the code that way will ensure that there is a compile-time
error if you write `-3!`, because there is no definition that
matches.


## One operator to rule them all

XL has a single fundamental operator, `is`, called the _definition operator_.
You use it as follows: `Pattern is Implementation`, where `Pattern` is a
program pattern, like `X+Y`, and `Implementation` is an implementation for
that pattern, for example `Add X, Y`. This operator can be read as
_transforms into_, i.e. it transforms the code that is on the left
into the code that is on the right.

<details>
<summary>It can define simple variables or constants</summary>

```xl
pi              is      3.1415926
```
</details>
<details>
<summary>It can define lists or data structures</summary>

```xl
funny_words     is      "xylophage", "zygomatic", "barfitude"
identity_matrix is
    [ [1, 0, 0],
      [0, 1, 0],
      [0, 0, 1] ]
```
</details>
<details>
<summary>It can define functions</summary>

```xl
abs X:number    is      if X < 0 then -X else X
```

</details>
<details>
<summary>It can define operators</summary>

```xl
X ≠ Y           is      not X = Y
```


</details>
<details>
<summary>It can define specializations for particular inputs</summary>

```xl
0!              is      1
N!  when N > 0  is      N * (N-1)!
```

</details>
<details>
<summary>It can define notations using arbitrary combinations of operators</summary>

```xl
A in B..C       is      A >= B and A <= C
```

</details>
<details>
<summary>It can define optimizations using specializations</summary>

```xl
X * 1           is      X
X + 0           is      X
```

</details>
<details>
<summary>It can define program structures</summary>

```xl
loop Body       is      Body; loop Body
```

</details>
<details>
<summary>It can define types</summary>

```xl
type complex    is      polar or cartesian
type cartesian  is      cartesian(re:number, im:number)
type polar      is      polar(mod:number, arg:number)
```

Note that [types](HANDBOOK_4-types.md) in XL indicate the shape of
parse trees. In other words, the `cartesian` type above will match any
parse tree that takes the shape of the word `cartesian` followed by
two numbers, like for example `cartesian(1,5)`.
</details>
<details>
<summary>It can define higher-order functions, i.e. functions that return functions</summary>

```xl
adder N         is      (lambda X is N + X)
add3            is      adder 3

 // This will compute 8
 add3 5
```
The notation `lambda X`, inspired by
[lambda calculus](https://en.wikipedia.org/wiki/Lambda_calculus),
means that we match the catch-all pattern `X`. This makes XL a
functional language in the traditional sense.

> **NOTE** The current implementations of XL special-case single-defintion
> contexts, and `lambda` can be omitted in that case. In a normal
> context, `X is Y` defines a name `X`, but it did not seem very
> useful to have single-definition contexts defining only a name.
> The above example could have been written as:
> ```
> adder N is (X is N + X)
> ```
> However, this is not consistent with the rest of the language.
> It is likely that `lambda` will be required in all future implementations.

</details>
<details>
<summary>It can define maps associating a key to a value</summary>

```xl
my_map is
    0 is 4
    1 is 0
    8 is "World"
    27 is 32
    lambda N when N < 45 is N + 1

// The following is "World"
my_map 8

// The following is 32
my_map[27]

// The following is 45
my_map (44)
```

This provides a functionality roughly equivalent to `std::map` in
C++. However, it's really nothing more than a regular function with a
number of special cases. The compiler can optimize special kinds
of mapping to provide an efficient implementation, for example if all
the indexes are contiguous integers.

</details>
<details>
<summary>It can define templates (C++ terminology) or
         generics (Ada terminology)</summary>

```xl
// An (inefficient) implementation of a generic 1-based array type
type array [1] of T is
    Value : T
    1 is Value
type array [N] of T when N > 1 is
    Head  : array[N-1] of T
    Tail  : T
    I when I<N is Head[I]
    I when I=N is Tail

A : array[5] of integer
for I in 1..5 loop
    A[I] := I * I
```

</details>
<details>
<summary>It can define variadic functions</summary>

```xl
min X, Y    is { Z is min Y; if X < Z then X else Z }
min X       is X

// Computes 4
min 7, 42, 20, 8, 4, 5, 30
```
</details>

In short, the single `is` operator covers all the kinds of declarations
that are found in other languages, using a single, easy to read syntax.


## The standard library

Each programming languages offers a specific set of features, which
are characteristic of that language. Most languages offer integer
arithmetic, floating-point arithmetic, comparisons, boolean logic,
loops, text manipulation (often called "strings"), but also
programming constructs such as loops, tests, and so on.

XL provides most features you are used to, but they are defined in the
XL _standard library_, not by the compiler. The standard library is
guaranteed to be present in all implementations and behave
identically. However, it is written using only tools that are
available to you as a developer.


### Definitions of control flow operations

For example, the _if statement_ in XL is defined in the standard
library as follows:

```xl
if [[true]]  then TrueClause else FalseClause   is TrueClause
if [[false]] then TrueClause else FalseClause   is FalseClause
if [[true]]  then TrueClause                    is TrueClause
if [[false]] then TrueClause                    is false
```

Similarly, the `while` loop is defined as follows:

```xl
while Condition loop Body is
    if Condition then
        Body
        while Condition loop Body
```

With the definitions above, you can then use `if` and `while` in your
programs much like you would in any other programming language, as in
the following code that verifies the Syracuse conjecture:

```xl
while N <> 1 loop
    if N mod 2 = 0 then
        N /= 2
    else
        N := N * 3 + 1
    write_line N
```

> **NOTE** A value between two square brackets, as in `[[true]]` and `[[false]]`,
> is called a [metabox](HANDBOOK_2-evaluation.md#metabox). It indicates
> that the pattern must match the actual values in the metabox. In
> other words, `foo true` defines a pattern with a formal paramter name `true`,
> whereas `foo [[true]]` defines a pattern which only matches when the
> argument is equal to constant `true`.


### The next natural evolutionary step

Moving features to a library is a natural evolution for programming
languages. Consider for example the case of text I/O operations. They
used to be built-in the language in early languages such as BASIC's
`PRINT` or Pascal's `WriteLn`, but they moved to the library in later
languages such as C with `printf`. As a result, C has a much wider
variety of I/O functions.  The same observation can be made on text
manipulation and math functions, which were all built-in in BASIC, but
all implemented as library functions in C. For tasking, Ada has
built-in construct, C has the `pthread` library. And so on.

Yet, while C moved a very large number of things to libraries, it
still did not go all the way. The meaning of `x+1` in C is defined
strictly by the compiler. So is the meaning of `x/3`, even if some
implementations have to make a call to a library function to actually
implement that code.

C++ went one step further than C, allowing you to _overload_
operators, i.e. redefine the meaning of an operation like `X+1`,
but only for custom data types, and only for already existing
operators.

In C++, you cannot create the _spaceship operator_ `<=>` yourself.
It has to be [added to the language](http://open-std.org/JTC1/SC22/WG21/docs/papers/2017/p0515r0.pdf),
and that takes a 35-pages article to discuss the implications.
By contrast, all it takes in XL to implement `<=>` in a variant that
always returns `-1`, `0` or `1` is the following:

```xl
syntax { INFIX 290 <=> }
X <=> Y     when X < Y  is -1
X <=> Y     when X = Y  is  0
X <=> Y     when X > Y  is  1
```

Similarly, C++ makes it extremely difficult to optimize away an
expression like `X*0`, `X*1` or `X+0`, whereas XL makes it extremely
easy:

```xl
X*0     is 0
X*1     is X
X+0     is X
```

Finally, C++ also makes it very difficult to deal with expressions
containing multiple operators. For example, many modern CPUs feature a
form of [fused multiply-add](https://en.wikipedia.org/wiki/Multiply–accumulate_operation#Fused_multiply–add),
which has benefits that include performance and precision. Yet C++
will not allow you to overload `X*Y+Z` to use this kind of
operations.

In XL, this is not a problem at all:

```xl
X*Y+Z   is FusedMultiplyAdd(X,Y,Z)
```

In other words, the XL approach represents the next evolutionary step
for programming languages along a line already followed by
highly-successful ancestors.


### Benefits of moving features to a library

Putting basic features in the standard library, as opposed to putting
them in the compiler, has several benefits:

1. Flexibility: It is much easier to offer a large number of behaviors
   and to address special cases.

2. Clarity: The definition given in the library gives a very clear
   and machine-verifiable description of the operation.

3. Extensibility: If the library definition is not sufficient, it is
   possible to add what you need. It will behave exactly as what is in
   the library. If it proves useful enough, it may even make it to the
   standard library in a later iteration of the language.

4. Fixability: Built-in mechanisms, such as library versioning, make
   it possible to address bugs without breaking existing code, which
   can still use an earlier version of the library.


The XL standard library consists of a [wide variety of modules](../native/lib).
The top-level module is called `XL`, and sub-modules are categorized
in a hierarchy. For example, if you need to perform computations on
complex numbers, you would use `import XL.MATH.COMPLEX` to load the
[complex numbers module](../native/lib/xl/math/complex.xs)

The [library builtins](../src/builtins.xl) is a list of definitions
that are accessible to any XL program without any explicit `import`
statement. This includes most features that you find in languages
such as C, for example integer arithmetic or loops. Compiler options
make it possible to load another file instead, or even to load no file
at all, in which case you need to build everything from scratch.


### The case of text input / output operations

Input/output operations (often abbreviated as I/O) are a fundamental
brick in most programming languages. In general, I/O operations are
somewhat complex. If you are curious, the source code for the
venerable `printf` function in C is
[available online](https://github.com/lattera/glibc/blob/master/stdio-common/vfprintf.c).

The implementation of text I/O in XL is comparatively very simple. The
definition of `write_line` looks something like, where irrelevant
implementation details were elided as `...`:

```xl
write X:text            as boolean      is ...
write X:integer         as boolean      is ...
write X:real            as boolean      is ...
write X:character       as boolean      is ...
write true                              is write "true"
write false                             is write "false"
write Head, Rest                        is write Head; write Rest

write_line              as boolean      is write SOME_NEWLINE_CHARACTER
write_line Items                        is write Items; write_line
```

This is an example of _variadic function definition_ in XL. In other
words, `write_line` can take a variable number of argument, much like
`printf` in C. You can write multiple comma-separated items in a
`write_line`. For example, consider the following code:

```xl
write_line "The value of X is ", X, " and the value of Y is ", Y
```

That would first call the last definition of `write_line` with the
following _binding_ for the variable `Items`:

```xl
Items   is "The value of X is ", X, " and the value of Y is ", Y`
```

This in turn is passed to `write`, and the definition that matches is
`write Head, Rest` with the following bindings:

```xl
Head    is "The value of X is "
Rest    is X, " and the value of Y is ", Y
```

In that case, `write Head` will directly match `write X:text` and
write some text on the console. On the other hand, `write Rest` will
need to iterate once more through the `write Head, Rest` definition,
this time with the following bindings:

```xl
Head    is X
Rest    is " and the value of Y is ", Y
```

The call to `write Head` will then match one of the implementations of
`write`, depending on the actual type of `X`. For example, if `X` is
an integer, then it will match with `write X:integer`. Then the last split
occurs for `write Rest` with the following bindings:

```xl
Head    is " and the value of Y is "
Rest    is Y
```

For that last iteration, `write Head` will use the `write X:text`
definition, and `write Rest` will use whatever definition of `write`
matches the type of `Y`.

All this can be done at compile-time. The generated code can then be
reused whenever the combination of argument types is the same. For
example, if `X` and `Y` are `integer` values, the generated code could
be used for

```xl
write_line "The sum is ", X+Y, " and the difference is ", X-Y
```

This is because the sequence of types is the same. Everything happens
as if the above mechanism had created a series of additional
definition that looks like:

```xl
write_line A:text, B:integer, C:text, D:integer is
    write A, B, C, D
    write_line

write A:text, B:integer, C:text, D:integer is
    write A
    write B, C, D

write B:integer, C:text, D:integer is
    write B
    write C, D

write C:text, D:integer is
    write C
    write  D
```

All these definitions are then available as shortcuts whenever the
compiler evaluates future function calls.


### Comparing XL I/O operations with C and C++

Unlike `printf`, `write_line` as defined above is both type-safe and
extensible.

It is type-safe because the compiler knows the type of each argument
at every step, and can check that there is a matching `write`
function.

It is extensible, because additional definitions of `write` will be
considered when evaluating `write Items`. For example, if you add a
`complex` type similar to the one defined by the standard library, all
you need for that type to become "writable" is to add a definition of
`write` that looks like:

```xl
write Z:complex     is write "(", Z.re, ";", Z.im, ")"
```

Unlike the C++ `iostream` facility, the XL compiler will naturally
emit less code. In particular, it will need only one function call for
every call to `write_line`, calling the generated function for the
given combination of arguments.

Additionally, the approach used in XL makes it possible to offer
specific features for output lines, for example to ensure that a
single line is always printed contiguously even in a multi-threaded
scenario. Assuming a `single_thread` facility ensuring that the code
is executed by at most one thread, creating a locked `write_line` is
nothing more than:

```xl
locked_write_line Items is
    single_thread
         write_line Items
```

It is extremely difficult, if not impossible, to achieve a similar
effect with C++ `iostream` or, more generally, with I/O facilities
that perform one call per I/O item. That's because there is no way for
the compiler to identify where the "line breaks" are in your code.


## Efficient translation

Despite being very high-level, XL was designed so that efficient
translation to machine code was possible, if sometimes challenging. In
other words, XL is designed to be able to work as a _system language_,
in the same vein as C, Ada or Rust, i.e. a language that can be used
to program operating systems, system libraries, compilers or other
low-level applications.

For that reason, nothing in the semantics of XL mandates complex
behind-the-scene activites, like garbage collection, thread safety, or
even memory management. As for other aspects of the language, any such
activity has to be provided by the library. You only pay for it if you
actually use it. In other words, the only reason you'd ever get
garbage collection in an XL program is if you explicitly need it for
your own application.

This philosophy sometimes requires the XL compiler to work extra hard
in order to be more than minimally efficient. Consider for example the
definition of the `while` loop given above:

```xl
while Condition loop Body is
    if Condition then
        Body
        while Condition loop Body
```

That definition can be used in your own code as follows:

```xl
while N <> 1 loop
    if N mod 2 = 0 then N /= 2 else N := N * 3 + 1
```

What happens is that the compiler looks at the code, and matches
against the definitions at its disposal. The `while` loop in the code
matches the form `while Condition loop Body`, provided you do the
following _bindings_:

```xl
Conditions is N <> 1
Body is
   if N mod 2 = 0 then N /= 2 else N := N * 3 + 1
```

The definition for the `while Condition loop Body` form is then
evaluated with the above bindings, in other words, the code below then
needs to be evaluated:

```xl
    if Condition then
        Body
        while Condition loop Body
```

Conceptually, that is extremely simple. Getting this to work well is
of course a little bit complicated. In particular, the definition ends
with another reference to `while`. If the compiler naively generates a
_function call_ to implement a form like that, it would rapidely run
out of stack space. A special optimization called _tail call elimination_
is required to ensure the expected behavior, namely the generation of a
machine branch instruction instead of a machine call instruction.

Furthermore, the reference implementation is just that, a reference.
The compiler is perfectly allowed, even encouraged, to "cheat",
i.e. to recognize common idioms, and efficiently translate them. One
name, `builtin`, is reserved for that purpose. For example, the
definition of integer addition may look like this:

```xl
X:integer + Y:integer as integer    is builtin Add
```

The left part of `is` here is perfectly standard XL. It tells the
compiler that an expression like `X+Y` where both `X` and `Y` have the
`integer` type will result in an `integer` value (that is the meaning
of `as integer`). The implementation, however, is not given. Instead,
the `builtin Add` tells the compiler that it has a cheat sheet for
that operations, called `Add`. How this cheat sheet is actually
implemented is not specified, and depends on the compiler.


## Adding complex features

Features can be added to the language that go beyond a simple
notation. This can also be done in XL, although this may require a
little bit of additional work. This topic cannot be covered
extensively here. Instead, examples from existing implementations will
provide hints of how this can happen.

### Reactive programming in Tao3D

[Reactive programming](https://en.wikipedia.org/wiki/Reactive_programming)
is a form of programming designed to facilitate the propagation of
changes in a program. It is particularly useful to react to changes in
a user interface.

[Tao3D](https://tao3d.sf.net) added reactive programming to XL to deal
with user-interface events, like mouse movements or keyboard input.
This is achieved in Tao3D using a combination of _partial re-evaluation_
of programs in response to _events_ sent by functions that depend on
user-interface state.

For example, consider the following Tao3D program to draw the hands of
a clock (see complete [YouTube tutorial](https://youtu.be/apy5csu0DkE)
for more details):

```xl
locally
    rotate_z -6 * minutes
    rectangle 0, 100, 15, 250

locally
    rotate_z -30 * hours
    rectangle 0, 50, 15, 150

locally
    color "red"
    rotate_z -6 * seconds
    rectangle 0, 80, 10, 200
```

The `locally` function controls the scope of partial re-evaluation.
Time-based functions like `minutes`, `hours` or `seconds` return the
minutes, hours and seconds of the current time, respectively, but also
trigger a time event each time they change. For example, the `hours`
function will trigger a time event every hour.

The `locally` function controls partial re-evaluation of the code within
it, and caches all drawing-related information within it in a
structure called a _layout_. There is also a top-level layout for
anything created outside of a `locally`.

The first time the program is evaluated, three layouts are created by
the three `locally` calls, and populated with three rectangles (one of
them colored in red), which were rotated along the Z axis
(perpendicular to the screen) by an amount depending on time. When,
say, the `seconds` value changes, a time event is sent by `seconds`,
which is intercepted by the enclosing `locally`, which then
re-evaluated its contents, and then sends a redraw event to the
enclosing layout. The two other layouts will use the cached graphics,
without re-evaluating the code under `locally`.

All this can be implemented entirely within the constraints of the
normal XL evaluation rules. In other words, the language did not have
to be changed in order to implement Tao3D.


### Declarative programming in Tao3D

Tao3D also demonstrates how a single language can be used to define
documents in a way that feels declarative like HTML, but still offers
the power of imperative programming like JavaScript, as well as style
sheets reminiscent of CSS. In other words, Tao3D does with a single
language, XL, what HTML5 does with three.

For example, an interactive slide in Tao3D would be written using code
like this:

```xl
import Slides

slide "The XL programming language",
    * "Extensible"
    * "Powerful"
    * "Simple"
```

This can easily be mis-interpreted as being a mere markup language,
something similar to [markdown](https://en.wikipedia.org/wiki/Markdown),
which is one reason why I sometimes call XL an _XML without the M_.

However, the true power of XL can more easily be shown by adding the
clock defined previously, naming it `clock`, and then using it in the
slide. This introduces the dynamic aspect that Javascript brings to
HTML5.

```xl
import Slides

clock is
    locally
        line_color "blue"
        color "lightgray"
        circle 0, 0, 300

    locally
        rotate_z -6 * minutes
        rectangle 0, 100, 15, 250

    locally
        rotate_z -30 * hours
        rectangle 0, 50, 15, 150

    locally
        color "red"
        rotate_z -6 * seconds
        rectangle 0, 80, 10, 200

slide "The XL programming language",
    * "Extensible"
    * "Powerful"
    * "Simple"
    anchor
        translate_x 600
        clock
```

In order to illustrate how pattern matching provides a powerful method
to define styles, one can add the following definition to the program
in order to change the font for the titles (more specifically, to
change the font for the "title" layouts of all themes and all slide masters):

```xl
theme_font Theme, Master, "title" is font "Palatino", 80, italic
```

The result of this program is an animated slide that looks like the
following:

![Animated clock](images/Tao3D-clock.png)


### Distributed programming with ELFE

[ELFE](https://github.com/c3d/elfe) is another XL-based experiment
targeting distributed programming, notably for the Internet of
things. The idea was to use the homoiconic aspect of XL to evaluate
parts of the program on different machines, by sending the relevant
program fragments and the associated data over the wire for remote
evaluation.

> ***NOTE*** ELFE is now integrated as part of XL, and the ELFE demos are
> stored in the [demo](../demo) directory of XL.

This was achieved by adding only four relatively simple XL functions:

* `tell` sends a program to another node in a "fire and forget" way,
  not expecting any response.

* `ask` evaluates a remote program that returns a value, and returns
  that value to the calling program.

* `invoke` evaluates a remote program, establishing a two-way
  communication with the remote that the remote can use with `reply`

* `reply` allows remote code within an `invoke` to evaluate code in
  its original caller's context, but with access to all the local
  variables declared by the remote.

Consider the [following program](../demo/7-two-hops.xl):

```xl
WORKER_1 is "pi2.local"
WORKER_2 is "pi.local"

invoke WORKER_1,
   every 1.1s,
        rasp1_temp is
            ask WORKER_2,
                temperature
        send_temps rasp1_temp, temperature

   send_temps T1:real, T2:real is
       if abs(T1-T2) > 2.0 then
           reply
               show_temps T1, T2

show_temps T1:real, T2:real is
    write_line "Temperature on pi is ", T1, " and on pi2 ", T2, ". "
    if T1>T2 then
        write_line "Pi is hotter by ", T1-T2, " degrees"
    else
        write_line "Pi2 is hotter by ", T2-T1, " degrees"
```

This small program looks like a relatively simple control script.
However, the way it runs is extremely interesting.

1. This single program actually runs on three different machines, the
   original controller, as well as two machines called `WORKER_1` and
   `WORKER_2`.

2. It still looks and feels like a single program. In particular,
   variables, values and function calls are passed around machines
   almost transparently. For example
   * the computation `T1-T2` in `send_temps` is performed on `WORKER_1`...
   * ... using a value of `T1` that actually came from `WORKER_2`
     through the `ask` statement in `rasp1_temp`.
   * Whenever the `reply` code is executed, variable `T1` and `T2` live
     on `WORKER_1`...
   * ... but within the `reply`, they are passed transparently as
     arguments in order to call `show_temps` on the controller.

3. Communication occurs primarily between `WORKER_1` and `WORKER_2`,
   which exchange a message every 1.1s. Communication with the
   controller only occurs if and when necessary. If the controller
   resides in Canada and the workers in Australia, this can save
   substantial networking costs.

4. A single `temperature` function, with an extremely simple
   implementation, provides an extremely rich set of remotely-accessible
   features that might require a very complex API in other languages.

This last point is worth insisting on. The following program uses the
same function to compute the min, max and average temperature on the
remote node. Nothing was changed to the temperature API. The
computations are performed efficiently by the remote node.

```xl
invoke "pi.local",
    min   is 100.0
    max   is 0.0
    sum   is 0.0
    count is 0

    compute_stats T:real is
        min   := min(T, min)
        max   := max(T, max)
        sum   := sum + T
        count := count + 1
        reply
            report_stats count, T, min, max, sum/count

    every 2.5s,
        compute_stats temperature

report_stats Count, T, Min, Max, Avg is
    write_line "Sample ", Count, " T=", T, " ",
               "Min=", Min, " Max=", Max, " Avg=", Avg
```

To run the ELFE demos, you need to start an XL server on the machines
called `pi.local` and `pi2.local`, using the `-remote` command-line
option of XL:

```xl
% xl -remote
```

You can then run the program on a third machine with:

```xl
% xl 7-two-hops.xl
```

Like for Tao3D, the implementation of these functions is not very
complicated, and more importantly, it did not require any kind of
change to the basic XL evaluation rules. In other words, adding
something as sophisticated as transparently distributed progrmming to
XL can be done by practically any programmer, without changing the
compiler.

-------------------------------------------------------------------------------

Previous: [Top](HANDBOOK.md)
Next: [Syntax](HANDBOOK_1-syntax.md)
