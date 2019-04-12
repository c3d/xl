# XL - An extensible language

XL is a very simple programming language designed to make it very easy
for programmers to extend the language to suit their needs. In XL,
extending the language is a routine operation, much like adding a
function or creating a class in more traditional programming
languages.

For more information, please consult the [reference manual](doc/XL.md).


## A few simple examples

A program computing the factorial of numbers between 1 and 5 would be
written as follows:
```xl
0! is 1
N! is N * (N-1)!

for I in 1..5 loop
    writeln "The factorial of ", N, " is ", N!
```


As a testament to its extensible nature, fundamental operations in XL
are defined in the standard library, including operations that would
be implemented using keywords in more traditional languages. For
example, the `if` statement in XL is defined by the following code:

```xl
if true  then TrueClause else FalseClause   is TrueClause
if false then TrueClause else FalseClause   is FalseClause
if true  then TrueClause                    is TrueClause
if false then TrueClause                    is false
```

Similarly, the `while` loop is defined as follows:

```xl
while Condition loop Body is
    if Condition then
        Body
        while Condition loop Body
```

The standard library also provides implementations for usual
operations. For example, if you evaluate `1+3`, this is done through a
definition for `+` on `integer` values that looks like the following
(where `...` denotes some implementation-dependent code):

```xl
X:integer + Y:integer is ...
```


## More complicated examples

Two dialects of XL further demonstrate the extensibility of the language

* [Tao3D](http://tao3d.sf.net) focuses on real-time 3D animations
  and can be used as a scriptable presentation software, or as someone
  once described it, a sort of real-time 3D LaTeX Lisp. In Tao3D, you
  describe a slide with a program that looks like the following code:

  ```xl
  import Slides

  slide "A simple slide example",
      * "This looks like some kind of markdown language"
      * "But code makes it powerful: your mouse is on the " & position
      position is
          if mouse_x < 0 then "left" else "right"
  ```

   >The examples above use the new syntax in XL, with `is` as its definition
   >operator. Older variants of the language used `->` instead. If
   >you downloaded a pre-built binary of Tao3D, chances are that you need
   >to replace `is` with `->` for the code above to work as intended.

* [ELFE](http://github.com/c3d/elfe), formerly ELIOT (Extensible
  Language for the Internet of things) was an experimetn on how to
  write distributed software that looks like a single program, for
  exampel to control swarms of devices in the context of the Internet
  of Things. An example of a simple ELFE program would be:

  ```xl
  WORKER is "worker.mycorp.com"
  MIN_TEMP is 25
  MAX_TEMP is 55

  invoke WORKER,
      every 2s,
          reply
              display temperature

  display Temp:real is
      writeln "The temperature of ", WORKER, " is ", Temp
  ```

> The present branch, `bigmerge`, is an ongoing effort to _reconverge_
> the various dialects of XL. At the moment, it should pass most of
> the ELFE-level tests, although this is not actively tested. Getting
> it to support Tao3D is somewhat more difficult and may take some time.


## If you come from another language

If you are familiar with other programming languages, here are a few
things that may surprise you about XL.

* There are no keywords.
* The language is designed primarily to be readable and writable by humans
* The language is _homoiconic_, i.e. programs are data.
* Evaluation is defined entirely in terms of rewrites of an abstract.
  syntax tree that represents the program being evaluated.
* The precedence of all operators is dynamic.
* The language is primarily defined by its own standard library.


### Semantics: One operator to rule them all

XL has one fundamental operator, `is`, the _definition operator_.
This operator can be read as *transforms into*, i.e. it transforms the
code that is on the left into the code that is on the right.

It can define _variables_ with possibly complex structures:

```xl
pi              is      3.1415926
words           is      "xylophage", "zygomatic", "barfitude"
```

It can define _functions_:

```xl
abs X           is      if X < 0 then -X else X
```

It can define _operators_:

```xl
X ≠ Y           is      not X = Y
```


It can define _specializations_ for particular inputs:

```xl
0!              is      1
N!  when N > 0  is      N * (N-1)!
```

It can define _notations_ using arbitrary combinations of operators:

```xl
A in B..C       is      A >= B and A <= C
```

It can define _optimizations_ using specializations:

```xl
X * 1           is      X
X + 0           is      X
```

It can define _program structures_:

```xl
loop Body       is      Body; loop Body
```


It can define _types_:

```xl
complex         is      type(complex(re:real, im:real))
```

It can define _higher-order functions_:

```xl
adder N         is      (X is N + X)
add3            is      adder 3

 // This will compute 8
 add3 5
```

It can define _maps_, which are actually just regular functions:

```xl
my_map is
    0 is 4
    1 is 0
    8 is "World"
    27 is 32
    N when N < 45 is N + 1

// The following is "World"
my_map 8

// The following is 32
my_map[27]

// The following is 45
my_map (44)
```

It can define _templates_ (C++ terminology), or _generics_ (Ada terminology):_

```xl
// An (inefficient) implementation of a generic array type
array [1] of T is
    Value : T
    0 is Value
array [N] of T when N > 1 is
    Head  : array[N-1] of T
    Tail  : T
    I when I<N-1 is Head[I]
    I when I=N-1 is Tail

A : array[5] of integer
A[3]
```

It can define _variadic functions_:

```xl
min X       is X
min X, Y    is { Z is min Y; if X < Z then X else Z }

// Computes 4
min 7, 42, 20, 8, 4, 5, 30
```


### Syntax: Look, Ma, no keywords!

XL has no keywords. Instead, the syntax relies on a rather simple
[recursive descent](https://en.wikipedia.org/wiki/Recursive_descent_parser)
[parser](src/parser.cpp).

THe parser generates a parse tree made of 8 node types. The first four
node types are leaf nodes:

 * `Integer` is for integer numbers such as `2` or `16#FFFF_FFFF`.
 * `Real` is for real numbers such as `2.5` or `2#1.001_001_001#e-3`
 * `Text` is for text values such as `"Hello"` or `'World'`. Text can
   be encoded using UTF-8
 * `Name` is for names and symbols such as `ABC` or `**`

The last four node types are inner nodes:

 * `Infix` are nodes where a named operator separates the operands,
   e.g. `A+B` or `A and B`.
 * `Prefix` are nodes where the operator precedes the operand, e.g.
   `+X` or `sin X`. By default, functions are prefix.
 * `Postfix` are nodes where the operator follows the operand, e.g.
   `3%` or `5km`.
 * `Block` are nodes with only one child surrounded by delimiters,
   such as `(A)`, `[A]` or `{A}`.

Of note, the line separator is an infix that separates statements,
much like the semi-colon `;`. The comma `,` infix is traditionally
used to build lists or to separate the argument of
functions. Indentation forms a special kind of block.

For example, the following code:

```xl
    tell "foo",
        if A < B+C then
            hello
        world
```

parses as a prefix `tell`, with an infix `,` as its right argument. On
the left of the `,` there is the text `"foo"`. On the right, there is
an indentation block with a child that is an infix line separator. On
the left of the line separator is the `if` statement. On the right is
the name `world`.

This parser is dynamically configurable, with the default priorities
being defined by the [xl.syntax](src/xl.syntax) file.

Parse trees are the fundamendal data structure in XL. Any data or
program can be represented as a parse tree. Program evaluation is
defined as transformation of parse trees.


### XL as a functional language

XL can be seen as a functional language, where functions are
first-class entities, i.e. you can manipulate them, pass them around,
etc:

```xl
adder X:integer is (Y is Y + X)

add3 is adder 3
add5 is adder 5

writeln "3+2=", add3 2
writeln "5+17=", add5 17
writeln "8+2=", (adder 8) 2
```

However, it is a bit different in the sense that the core data
structure is the parse tree. Some specific parse trees, for example
`A+B`, are not naturally reduced to a function call, although they are
subject to the same evaluation rules based on tree rewrites.


### Subtlety #1: expression vs. statement

The XL parse tree is designed to represent programs in a way that
is relatively natural for human beings. In that sense, it departs from
languages such as Lisp or SmallTalk.

However, being readable for humans requires a few special rules to
match the way we read expressions. Consider for example the following:

```xl
write sin X, cos Y
```

Most human being parse this as meaning `write (sin(X),cos(Y))`,
i.e. we call `write` with two values resulting from evaluating `sin X`
and `cos Y`. This is not entirely logical. If `write` takes
comma-separated arguments, why wouldn't `sin` also take
comma-separated arguments? In other words, why doesn't this parse as
`write(sin(X, cos(Y))`?

This shows that humans have a notion of *expressions*
vs. *statements*. Expressions such as `sin X` have higher priority
than commas and require parentheses if you want multiple arguments. By
contrast, statements such as `write` have lower priority, and will
take comma-separated argument lists. An indent or `{ }` block begins a
statement, whereas parentheses `()` or square brackets `[]` begin an
expression.

There are rare cases where the default rule will not achieve the
desired objective, and you will need additional parentheses.

### Subtlety #2: infix vs. prefix

Another special rule is that XL will use the presence of space on
only one side of an operator to disambiguate between an infix or a
prefix. For example:

```xl
write -A    // write (-A)
B - A       // (B - A)
```

### Subtlety #3: Delayed evaluation

When you pass an argument to a function, evaluation happens only when
necessary. Deferred evaluation may happen multiple times, which is
necessary in many cases, but awful for performance if you do it by
mistake.

Consider the following definition of `every`:

```xl
every Duration, Body is
    loop
        Body
        sleep Duration
```

In that case, we want the `Body` to be evaluated every iteration,
since this is typically an operation that we want to execute at each
loop. Is the same true for `Duration`? Most likely, no.

One way to force evaluation is to give a type to the argument. If you
want to force early evaluation of the argument, and to check that it
is a real value, you can do it as follows:

```xl
every Duration:real, Body is
    loop
        Body
        sleep Duration
```

### Subtlelty #4: Closures

Like many functional languages, XL ensures that the value of
variables is preserved for the evaluation of a given body. Consider
for example:

```xl
adder X:integer is (Y is Y + X)
add3 := adder 3
```

In that case, `adder 3` will bind `X` to value `3`, but then the
returned value outlives the scope where `X` was declared. However, `X`
is referred to in the code. So the returned value is a *closure* which
integrates the binding `X is 3`.
