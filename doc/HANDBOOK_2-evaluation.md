# XL program evaluation

XL defines _program execution_ primarily in terms of operations on
the parse tree combined with operations on an implicit _context_ that
stores the program state. The context itself is also described in XL
in order to define the expected result of evaluation.

For efficiency, actual implementations are unlikely to store
everything as an actual parse tree, although there is an _interpreter_
implementation that does exactly that. A compiler is more likely to
[optimize representations](#compiled-representations) of both
code and data, as long as that optimized representation ultimately
respect the semantics described using the normal form for the parse tree.


## Execution phases

Executing an XL program is the result of three phases,

1. A [parsing phase](#parsing-phase) where program source text is
   converted to a parse tree,
2. A [declaration phase](#declaration-phase), where all declarations
   are stored in the context,
3. An [evaluation phase](#evaluation-phase), where statements other
   than declarations are processed in order.

The execution phases are designed so that in a very large number of
cases, it is at least conceptually possible to do both the parsing and
declaration phases ahead of time, and to generate machine code that can
perform the evaluation phase using only representations of code and
data [optimized](#compiled-representations) for the specific machine
running the program. It should be possible to create an efficient
ahead-of-time compiler for XL. Work is currently in progress to build
one.

> **NOTE** Reasonably efficient compilers were produced for earlier
> generations of the language, notably as part of the Tao3D project.
> However, this earlier iteration of the language had a very weak type
> system that made advanced optimizations hard to achieve. This was
> actually a feature for Tao3D, which purposely disabled some
> optimizations in order to improve compilation speed, notably when
> the program structure did not change. The version of XL described in
> this document, however, has markedly evolved relative to what was
> implemented in Tao3D, with the hope that much better code quality
> can be achieved. This part has not been demonstrated yet.


### Execution context

The execution of XL programs is defined by describing the evolution of
a particular data structure called the _execution context_, or simply
_context_, which stores all values accessible to the program at any
given time.

That data structure is only intended to explain the effect of
evaluating the program. It is not intended to be a model of how things
are actually implemented. As a matter of fact, care was taken in the
design of XL to allow standard compilation and optimization techniques
to remain applicable, and to leave a lot of freedom regarding actual
evaluation techniques.

In the examples below, `CONTEXT0`, `CONTEXT1`, ... will denote
pseudo-variables that describe the various currently visible execution
contexts, following the language [scoping](#scoping) rules. The most
recent contexts will have higher numbers. In addition, `HIDDEN0`,
`HIDDEN1`, ... will represent pending execution contexts that are
invisible to the currently executing code. These are also known as
[_activation records_](https://en.wikipedia.org/wiki/Activation_record).
Entries in `HIDDEN` contexts are [live](HANDBOOK_3-types.md#lifetime),
but invisible to the current code. By convention, `CONTEXT0` and
`HIDDEN0` are not defined in the examples and are assumed to be
inherited from earlier execution.

> **NOTE** By default, the context of the caller is not visible to the
> callee. It can be made visible if necessary using a feature called
> [_caller lookup_](#caller-lookup).


### Parsing phase

The parsing phase reads source text and turns it into a parse tree
using operator spelling and precedence information given in the
[syntax file](../src/xl.syntax). This results either in a parse-time
error, or in a faithful representation of the source code as a parse
tree data structure that can be used for program evaluation.

Since there is almost a complete equivalence between the parse tree
and the source code, the rest of the document will, for convenience,
represent a parse tree using a source code form. In the rare cases
where additional information is necessary for understanding, it will
be provided in the form of XL comments.

Beyond the creation of the parse tree, very little actual processing
happens during parsing. There are, however, a few tasks that can only
be performed during parsing:

1. Filtering out comments: Comments should not have an effect on the
   program, so they are simply eliminated during parsing.

2. Processing `syntax` statements: This must be done during parsing,
   because `syntax` is designed to modify the
   [spelling and precedence](HANDBOOK_1-syntax.md#extending-the-syntax)
   of operators, and that information is used during the parsing
   phase.

3. Processing `use` statements: Since imported modules can contain
   `syntax` statements, they must at least partially be processed
   during parsing. Details about `use` statements are covered in
   the [chapter about modules](HANDBOOK_6-modules.md).

4. Identifying words that switch to a
   [child syntax](HANDBOOK_1-syntax.md#child-syntax):
   symbols that activate a child syntax are recognized during parsing.
   This is the case for example with the `extern` name in the
   [default syntax](../src/xl.syntax#L62).

5. Identifying binary data: words such as `bits` marked as introducing
   `BINARY` data in the syntax file are treated specially during
   parsing, to generate parse tree nodes representing binary data.
   >  **NOTE** this is not currently implemented.

The need to process `use` statements during parsing means that it's
not possible in XL to have computed `use` statements. The name of
the module must always be evaluated at compile-time.

> **RATIONALE** An alternative would have been to allow computed
> `use` statement, but disallow `syntax` in them. However, for
> convenience, `use` names look like `XL.CONSOLE.TEXT_IO` and not,
> say, `"xl/console/text_io.xs"`, so there is no obvious way to
> compute them anyway. If computed `use` statement ever become
> necessary, it will be easy enough to use the syntax `use "path"`
> for them.

Once parsing completes successfully, the parse tree can be handed to
the declaration and evaluation phases. Parsing occurs for the
_entire program_, including imported modules, before the other phases
begin.


### Sequences

Both declaration and evaluation phases will process _sequences_, which
are one of:

* A block, in which case processing the sequence means processing the
  block's child
  ```xl
  loop { print "Hello World" }
  ```
* An infix `NEWLINE` or semi-colon `;`, in which case the left and right
  operands of the infix are processed in that order.
  ```xl
  print "One"; print "Two"
  print "Three"
  ```
* An `use` statement, which is the only statement that requires
  processing in all three executation phases.
  ```xl
  use XL.MATH.COMPLEX
  ```
* A `syntax` definition, which only plays a role during parsing is
  ignored during the declaration and evaluation phases.
  ```xl
  syntax { INFIX 290 <=> }
  ```
* An infix `is`, which is called a _definition_, an infix `:` or
  `as`, which are called
  [_type annotations_](HANDBOOK_3-types.md##type-annotations),
  or an infix assignment operator `:=` with a type annotation
  on the left, called a _variable initialization_. Definitions, type
  annotations and variable initializations are collectively called
  _declarations_, and are  processed during the
  [declaration phase](#declaration-phase).
  ```xl
  pi is 3.1415                  // Definition of 'pi'
  Count : integer               // Declaration of 'Count'
  byte_size X as integer        // Declaration of 'byte_size X'
  Remaining : integer := 100    // Declaration of 'Remaining'
  ```
* Anything else, which is called a _statement_ and is processed during the
  [evaluation phase](#evaluation-phase).
  ```xl
  print "This is a statement"
  ```

For example, consider the following code:

```xl
pi is 3.14
circumference 5.3
circumference Radius:real is 2 * pi * Radius
```

The first and last line are representing a definition of `pi` and
`circumference Radius:real` respectively. The second line is made of one
statement that computes `circumference 5.3`. There are two definitions,
one statement and no type annotation in this code.

Note that there is a type annotation for `Radius` in the definition on
the last line, but that annotation is _local_ to the definition, and
consequently not part of the declarations in the top-level sequence.

In that specific case, that type annotation is a declaration of a
_parameter_ called `Radius`, which only accepts `real` values.
Sometimes, such parameters are called _formal parameters_.
A parameter will receive its value from an _argument_ during the
evaluation. For example the `Radius` parameter will be _bound_ to
argument`5.3` while evaluating the statement on the second line.

The _result_ of a sequence is the value of its last statement. In our
example, the result of executing the code will be the value computed
by `circumference 5.3`.


### Declaration phase

The declaration phase of the program begins as soon as the parsing
phase finishes.

During the declaration phase, all declarations are stored in order in
the context, so that they appear before any declaration that was
already in the context. As a result, the new declarations may _shadow_
existing declarations that match.

In the example above, the declaration phase would result in a context
that looks something like:

```xl
CONTEXT1 is
    pi is 3.14
    circumference Radius:real is 2 * pi * Radius
    CONTEXT0
    HIDDEN0
```

An actual implementation is likely to store declarations is a more
efficient manner. For example, an interpreter might use some hashing or
some form of balanced tree. Such optimizations must preserve the order
of declarations, since correct behavior during the evaluation phase
depends on it.

In the case of a [compiled implementation](HANDBOOK_4-compilation.md),
the compiler will most likely assign machine locations to each of the
declarations.  When the program runs, a constant like `pi` or the
definition of `circumference` may end up being represented as a
machine address, and a variable such as `Radius` may be represented as
a "stack location", i.e. a preallocated offset from the current stack
pointer, the corresponding memory location only containing the value,
i.e. the right-hand side of `:=`.
Most of the [type analysis](HANDBOOK_3-types.md) can be performed at
compile time, meaning that most type information is unnecessary at
program run time and can be eliminated from the compiled program.

Note that since the declaration phase occurs before the execution
phase, all declarations in the program will be visible during the
evaluation phase. In our example, it is possible to use `circumference`
before it has been declared. Definitions may therefore refer to one
another in a circular way. Some other languages such as C require
"forward declarations" in such cases, XL does not.

The parse tree on the left of `is`, `as` or `:` is called the
_pattern_ of the declaration. The pattern will be checked against the
_form_ of parse trees to be evaluated. The right operand of `:` or
`as` is the type of the type annotation. The parse tree on the right
of `is` is called the _body_ of the definition.


### Evaluation phase

The evaluation phase processes each statement in the order they appear
in the program. For each statement, the context is looked up for
matching declarations in order. There is a match if the shape of the
tree being evaluated matches the pattern of the declaration. Precise
pattern matching rules will be [detailed below](#pattern-matching).
In our example, `circumference 5.3` will not match the declaration of
`pi`, but it will match the declaration of `circumference Radius:real`
since the value `5.3` is indeed a real number.

When a match happens, a new context is created with _bindings_ for the
formal parameters to the value passed as an argument in the
statement. This new context is called a _local context_ and will be
used to evaluate the body of the definition. For example, the local
context to evaluate the body of the definition of
`circumference Radius:real` would be:

```xl
CONTEXT2 is
    Radius:real := 5.3
    CONTEXT1
    HIDDEN1
HIDDEN1 is CONTEXT1
```

As a reminder, `Radius` is a _formal parameter_, or simply _parameter_
that receives the _argument_ 5.3 as a result of _binding_. The binding
remains active for the duration of the evaluation of of the body of
the definition. The binding, at least conceptually, contains the type
annotation for the formal parameter, ensuring that all required
[type constraints](HANDBOOK_3-types.md) are known and respected.
For example, the context contains the `Redius:real` annotation, so that
attempting `Radius := "Hello"` in the body of `circumference` would
fail, because the type of `"Hello"` does not match the `real` type.

Bindings can be marked as [mutable](HANDBOOK_3-types.md#mutability)
or constant. In this document, bindings made with `:=` are mutable, while
binding made with `is` are constant. Since by default, an `X : T`
annotation creates a mutable binding, the binding for `Radius` is made
with `:=`.

Once the new context has been created, execution of the program
continues with the body of the definition. In that case, that means
evaluating expression `2 * pi * Radius` in the newly created local
context.

After execution of the body completes, the result of that execution
replaces the statement that matched the definition's pattern. In our
example, `circumference 5.3` behaves like `2 * pi * Radius` in a
context containing `Radius is 5.3`.

The process can then resume with the next statement if there is
one. In our example, there isn't one, so the execution is complete.


## Expression evaluation

Executing the body for the definition of `circumference Radius:real`
involves the evaluation of expression `2 * pi * Radius`. This follows
almost exactly the same process as for `circumference 5.3`, but in
that case, that process needs to be repeated multiple times to
complete the evaluation.

If we apply the evaluation process with `2 * pi * Radius`, assuming
the declarations in the [standard library](HANDBOOK_7-standard-library.md),
no declaration has a larger pattern like `X * Y * Z` that could match
the whole expression. However, there is a definition for a
multiplication between `real` numbers, with a pattern that looks like
`X:real * Y:real as real`, as well as another for `integer`
multiplication, with a pattern that looks like `X:integer * Y:integer`.
There may be more, but we will ignore them for the rest of this
discussion. The code below shows what the relevant declaration might
look like:

```xl
X:integer * Y:integer   as integer  is builtin IntegerAdd
X:real * Y:real         as real     is builtin FloatingPointAdd
```

The `*` operator is left-associative, so `2 * pi * Radius` parses as
`(2 * pi) * Radius`. Therefore, we will be looking for a match with
`X` corresponding to `2 * pi` and `Y` corresponding to `Radius`.
However, that information alone is insufficient to determine if either
sub-expression is `integer` or `real`. In order to be able to make
that determination, [immediate evaluation](#immediate-evaluation) of
the arguments is required. The evaluation process therefore repeats
with sub-expression `2 * pi`, and like before, it is necessary to
evaluate `pi`. This in turns gives the result `3.14` given the current
context. That result replaces `pi`, so that we now must evaluate
`2 * 3.14`.

The `2 * 3.14` tree does not match `X:real * Y:real` because `2` is an
`integer` and not a `real`. It does not match `X:integer * Y:integer`
either because `3.14` is a `real` and not an `integer`. However, the
standard library provides a definition of an _implicit conversion_
that looks something like this:

```xl
X:integer as real     is builtin IntegerToReal
```

This implicit conversion tells the compiler how to transform an
`integer` value like `2` into a `real`. Implicit conversions are only
considered if there is no exact match, and only one of them can be
used to match a given parameter. In our case, there isn't an exact
match, so the evaluation will consider the implicit conversion to get a
`real` from `integer` value `2`.

The body of the implicit conversion above is therefore evaluated in a
context where `X` is set to `2`:

```xl
CONTEXT3 is
    X:integer := 2
    CONTEXT2
    HIDDEN2
HIDDEN2 is CONTEXT2
```

The result of that implicit conversion is `2.0`. Evaluation can
then resume with the `X:real * Y:real as real` definition, this time
called with an argument of the correct `real` type for `X`:

```xl
CONTEXT4 is
    X:real := 2.0
    Y:real := 3.14
    CONTEXT2
    HIDDEN2
```

The result of the multiplication is a `real` with value `6.28`, and
after evaluating `Radius`, evaluation of the second multiplication
will then happen with the following context:

```xl
CONTEXT5 is
    X:real := 6.28 // from 2 * pi
    Y:real :=5.3  // from Radius
    CONTEXT2
    HIDDEN2
```

The result of the last multiplication is a `real` with value
`33.284`. This is the result of evaluating `circumference 5.3`, and
consequently the result of executing the entire program.

> **NOTE** The [standard XL library](HANDBOOK_7-standard-library.md)
> only provides implicit conversions that do not cause data loss. On
> most implementation, `real` has a 53-bit mantissa, which means that
> the implicit conversion from `integer` to `real` is actually closer
> to the following:
> ```xl
> X:integer as real when X >= -2^53 and X < 2^53 is ...
> ```


## Pattern matching

As we have seen above, the key to execution in XL is _pattern matching_,
which is the process of finding the declarations patterns that match a
given parse tree. Pattern matching is recursive, the _top-level pattern_
matching only if all _sub-patterns_ also match.

For example, consider the following declaration:

```xl
log X:real when X > 0.0 is /* ... implementation of log ... */
```

This will match an expression like `log 1.25` because:

1. `log 1.25` is a prefix with the name `log` on the left, just like
   the prefix in the pattern.

2. `1.25` matches the formal parameter `X` and has the expected `real`
   type, meaning that `1.25` matches the sub-pattern `X:real`.

3. The condition `X > 0.0` is true with binding `X is 1.25`

More generally, a pattern `P` matches an expression `E` if one of the
following conditions is true:

* <details>
  <summary>
  The top-level pattern `P` is a name, and expression `E` is the same name.
  </summary>

  | Declaration           | Matched by         | Not matched by      |
  | --------------------- | ------------------ | ------------------- |
  | `pi is 3.14`          | `pi`               | `ip`, `3.14`        |
  </details>

* The top-level pattern `P` is a prefix, like `sin X` or a postfix, like
  `X%`, and the expression `E` is a prefix or  postfix respectively
  with the same operator as the pattern, and operands match.

  | Pattern               | Matched by         | Not matched by       |
  | --------------------- | ------------------ | -------------------- |
  | `sin X`               | `sin (2.27 + A)`   | `cos 3.27`           |
  | `+X:real`             | `+2.27`            | `+"A"`, `-3.1`, `1+1`|
  | `X%`                  | `2.27%`, `"A"%`    | `%3`, `3%2`          |
  | `X km`                | `2.27 km`          | `km 3`, `1 km 3`     |

* The top-level pattern `P` is an infix `when` like `Pattern when Condition`,
  called a _conditional pattern_, and `Pattern` matches `E`, and the
  value of `Condition` is `true` with the bindings declared while
  matching `Pattern`.

  | Pattern               | Matched by         | Not matched by       |
  | --------------------- | ------------------ | -------------------- |
  | `log X when X > 0`    | `log 3.5`          | `log(-3.5)`          |

* The pattern `P` is an infix, and the operator is the same in the pattern
  and in the expression, and both operands in the pattern match the
  respective operands in the expression.

  | Pattern               | Matched by         | Not matched by       |
  | --------------------- | ------------------ | -------------------- |
  | `X:real+Y:real`       | `3.5+2.9`          | `3+2`, `3.5-2.9`     |
  | `X and Y`             | `N and 3`          | `N or 3`             |

  For sub-patterns, if `E` is a name and its' bound to an infix with
  the same operator as in the pattern, then its value can be _split_.

  | Pattern               | Matched by         | Not matched by       |
  | --------------------- | ------------------ | -------------------- |
  | `write X,Y` | `write Items` when `Items is "A","B"` | `wrote 0`, `write Items` when `Items is "A"+B"`   |

  > **NOTE** A very common idiom  is to use comma `,` infix to separate
  > multiple parameters, as in the following definition:
  > ```xl
  > write Head, Tail is write Head; write Tail
  > ```
  > This declaration will match `write 1, 2, 3` with bindings `Head is 1` and
  > `Tail is 2,3`. In the evaluation of the body with these bindings,
  > `write Tail` will then match the same declaration again with
  > `Tail` being split, resulting in bindings `Head is 2` and `Tail is 3`.

* The pattern `P` is an `integer`, `real` or `text` constant, or a
  [metabox](#metabox), for example `0`, `1.25`, `"Hello"` or `[[sqrt 2]]`
  respectively, and the expression `E` matches the value given by the
  constant or computed by the expression in the metabox.
  In that case, expression `E` needs to be evaluated to check if it matches.

  | Pattern               | Matched by         | Not matched by       |
  | --------------------- | ------------------ | -------------------- |
  | `if [[true]] then Op` | `if X<3 then run` when `X<3` is `true` | `if 0 then run` |
  | `0!`                  | `N!` when `N=0`    | `N!` when `N<>0`     |

  This case applies to sub-patterns, as was the case for `0! is 1` in
  the [definition of factorial](HANDBOOK_0-introduction.md#factorial),
  as well as to top-level patterns, as in `0 is "Zero"`.
  This last case is useful in [maps](#scoping).

  | Definition            | Matched by          | Not matched by       |
  | --------------------- | ------------------- | -------------------- |
  | `0 is "Zero"`         | `0`, `N` when `N=0` | `1`, `'A'`, `N` unless `N=0`|
  | `[[false]] is self`   | `false`             | `true`               |

* The sub-pattern `P` is a name, such as `N` in `N! is N * (N-1)!`. In
  that case, a binding `N is A` is added at the top of the local context
  used to evaluate the body, where `A` is the (possibly evaluated)
  sub-expression from the argument.

  | Sub-pattern           | Matched by          | Not matched by       |
  | --------------------- | ------------------- | -------------------- |
  | `N`                   | Any expression      |                      |

* The sub-pattern `P` is a type annotation, such as `X:real`.
  This case is similar to the previous one, except that `E` may need
  to be evaluated to check its type. For example, expression `A+B`
  must be evaluated if the pattern is `X:real`, but not for `X:infix`
  since `A+B` is an infix.

  | Pattern               | Matched by          | Not matched by       |
  | --------------------- | ------------------- | -------------------- |
  | `N:integer`           | `0`, `A+B` when `integer` | `A+B` when `real` |
  | `N:infix`             | `A+B` always        | `A*B`                |

* The sub-pattern `P` is a name that was already bound in the same
  top-level pattern, and  expression `P = E` is `true`.

  | Pattern               | Matched by          | Not matched by       |
  | --------------------- | ------------------- | -------------------- |
  | `N+N`                 | `3+3`, `A+B` when `A=B` | `3-3`, `3+4`     |

* The pattern is a block with a child `C`, and `C` matches `E`.
  In other words, blocks in a pattern only change the precedence, but play no
  other role in pattern matching.

  | Definition            | Matched by          | Not matched by       |
  | --------------------- | ------------------- | -------------------- |
  | `(X+Y)*(X-Y) is X^2-Y^2` | `[A+3]*[A-3]`    | `(A+3)*(A-4)`        |

  The delimiters of a block cannot be tested that way, you should use
  a conditional pattern like `B:block when B.opening = "("`.


In some cases, pattern matching requires evaluation of an expression
or sub-expression. This is called [immediate evaluation](#immediate-evaluation).
Otherwise, [evaluation will be lazy](#lazy-evaluation).


## Overloading

There may be multiple declarations where the pattern matches a given
parse tree. This is called _overloading_. For example, as we have seen
above, for the multiplication expression `X*Y` we have at least
`integer` and `real` candidates. This looks like:

```xl
X:integer * Y:integer as integer        is ...
X:real    * Y:real    as real           is ...
```

The first declaration above would be used for an expression like `2+3`
and the second one for an expression like `5.5*6.4`. It is important
for the compiler to be able to distinguish them, since they may result
in very different machine-level operations.

In XL, the various declarations in the context are considered in
order, and the first declaration that matches is selected. A candidate
declaration matches if it matches the whole shape of the tree.

> **NOTE** The XL2 implementation does not select the first that
> matches, but the _largest and most specialized_ match. This is a
> slightly more complicated implementation, but not by far, and it has
> some benefits, notably with respect to making the code more robust
> to reorganizations. For this reason, this remains an open option.
> However, it is likely to be more complicated with the more dynamic
> semantics of XL, notably for [dynamic dispatch](#dynamic-dispatch),
> where the runtime cost of finding the proper candidate might be a
> bit too high to be practical.

For example, `X+1` can match any of the declarations patterns below:

```xl
X:integer + Y:integer
X:integer + 1
X:integer + Y:integer when Y > 0
X + Y
Infix:infix
```

The same `X+1` expression will not match any of the following
patterns:

```xl
foo X
+1
X * Y
```

Knowing which candidate matches may be possible at compile-time, for
example if the selection of the declaration can be done solely based
on the type of the arguments and parameters. This would be the case if
matching an`integer` argument against an `integer` parameter, since
any value of that argument would match. In other cases, it may
require run-time tests against the values in the declaration. This
would be the case if matching an `integer` argument against `0`, or
against `N:integer when N mod 2 = 0`.

For example, a definition of the
[Fibonacci sequence](https://en.wikipedia.org/wiki/Fibonacci_number)
in XL is given below:

```xl
fib 0   is 0
fib 1   is 1
fib N   is (fib(N-1) + fib(N-2))
```

> **NOTE** Parentheses are required around the
> [expressions statements](HANDBOOK_1-syntax.md#tweak-1-expression-vs-statement)
> in the last declaration in order to parse this as the addition of
> `fib(N-1)` and `fib(N-2)` and not as the `fib` of `(N-1)+fib(N-2)`.

When evaluating a sub-expression like `fib(N-1)`, three candidates for
`fib` are available, and type information is not sufficient to
eliminate any of them. The generated code will therefore have to evaluate
`N-1`. [Immediate evaluation](#immediate-evaluation) is needed
in order to compare the value against the candidates. If the value is
`0`, the first definition will be selected. If the value is `1`, the
second definition will be used. Otherwise, the third definition will
be used.

A binding may contain a value that may itself need to be split in
order to be tested against the formal parameters. This is used in the
implementation of `print`:

```xl
print Items             is write Items; print
write Head, Rest        is write Head; write Rest
write Item:integer      is /* implementation for integer */...
write Item:real         is /* implementation for real */...
```

In that case, finding the declaration matching `print "Hello", "World"`
involves creating a binding like this:

```xl
CONTEXT1 is
    Items is "Hello", "World"
    CONTEXT0
```

When evaluating `write Items`, the various candidates for `write`
include `write Head, Rest`, and this will be the one selected after
splitting `Items`, causing the context to become:

```xl
CONTEXT2 is
    Head is "Hello"
    Rest is "World"
    CONTEXT0
    HIDDEN1 is CONTEXT1
```


## Dynamic dispatch

As shown above, the declaration that is actually selected to evaluate
a given parse tree may depend on the dynamic value of the
arguments. In the Fibonacci example above, `fib(N-1)` may select any
of the three declarations of `fib` depending on the actual value of
`N`. This runtime selection of declarations based on the value of
arguments is called _dynamic dispatch_.

In the case of `fib`, the selection of the correct definition is a
function of an `integer` argument. This is not the only kind of test
that can be made. In particular, dynamic dispatch based on the
_type_ of the argument is an important feature to support well-known
techniques such as object-oriented programming.

Let's consider an archetypal example for object-oriented programming,
the `shape` class, with derived classes such as `rectangle`, `circle`,
`polygon`, and so on. Textbooks typically illustrate dynamic dispatch
using a `Draw` method that features different implementations
depending on the class. Dynamic dispatch selects the appropriate
implementation based on the class of the `shape` object.

In XL, this can be written as follows:

```xl
draw R:rectangle    is /* ... implementation for rectangle class ... */
draw C:circle       is /* ... implementation for circle class ... */
draw P:polygon      is /* ... implementation for polygon class ... */
draw S:shape        is /* ... implementation for shape class ... */

draw Something      // Calls the right implementation based type of Something
```

A single dynamic dispatch may require multiple tests on different
arguments. For example, the `and` binary operator can be defined
(somewhat inefficiently) as follows:

```xl
[[false]] and [[false]]     is false
[[false]] and [[true]]      is false
[[true]]  and [[false]]     is false
[[true]]  and [[true]]      is true
```

When applied to types, this capability is sometimes called _multi-methods_
in the object-oriented world. This makes the XL version of dynamic
dispatch somewhat harder to optimize, but has interesting use
cases. Consider for example an operator that checks if two shapes
intersect. In XL, this can be written as follows:

```xl
X:rectangle intersects Y:rectangle  as boolean  is ... // two rectangles
X:circle    intersects Y:circle     as boolean  is ... // two circles
X:circle    intersects Y:rectangle  as boolean  is ... // rectangle & circle
X:polygon   intersects Y:polygon    as boolean  is ... // two polygons
X:shape     intersects Y:shape      as boolean  is ... // general case

if shape1 intersects shape2 then    // selects the right combination
    print "The two shapes touch"
```

> **NOTE** Type-based dynamic dispatch is relatively similar to the
> notion of _virtual function_ in C++, although the XL implementation is
> likely to be quite different. The C++ approach only allows dynamic
> dispatch along a single axis, based on the type of the object
> argument. C++ also features a special syntax, `shape.Draw()`, for calls
> with dynamic dispatch, which differs from the C-style syntax for
> function calls, `Draw(shape)`. The syntax alone makes the `intersects`
> example difficult to write in C++.

As another illustration of a complex dynamic dispatch not based on
types, Tao3D uses theme functions that depend on the names of the
slide theme, master and element, as in:

```xl
theme_font "Christmas", "main",       "title"   is font "Times"
theme_font "Christmas", SlideMaster,  "code"    is font "Menlo"
theme_font "Christmas", SlideMaster,  SlideItem is font "Palatino"
theme_font SlideTheme,  SlideMaster,  SlideItem is font "Arial"
```

As the example above illustrates, the XL approach to dynamic dispatch
takes advantage of pattern matching to allow complex combinations of
argument tests.


## Immediate evaluation

In the `circumference` examples, matching `2 * pi * Radius` against the
possible candidates for `X * Y` expressions required an evaluation of
`2 * pi` in order to check whether it was a `real` or `integer` value.

This is called _immediate evaluation_ of arguments, and is required
in XL for statements, but also in the following cases:

1. When the formal parameter being checked has a type annotation, like
   `Radius` in our example, and when the annotation type does not
   match the type associated to the argument parse tree. Immediate
   evaluation is required in such cases in order to check if the argument
   type is of the expected type after evaluation.  Evaluation is *not*
   required if the argument and the declared type for the formal
   parameter match, as in the following example:
   ```xl
   write X:infix   is  write X.left, " ", X.name, " ", X.right
   write A+3
   ```
   In that case, since `A+3` is already an `infix`, it is possible
   to bind it to `X` directly without evaluating it. So we will
   evaluate the body with binding `X:infix is A+3`.

2. When the part of the pattern being checked is a constant or a
   [metabox](#metabox). For example, this is the case in the definition of the
   factorial below, where the expression `(N-1)` must be evaluated in order
   to check if it matches the value `0` in pattern `0!`:
   ```xl
   0! is 1
   N! is N * (N-1)!
   ```
   This is also the case for the condition in `if-then-else` statements,
   to check if that condition matches either `true` or `false`:
   ```xl
   if [[true]]  then TrueBody else FalseBody    is TrueBody
   if [[false]] then TrueBody else FalseBody    is FalseBody
   ```

3. When the same name is used more than once for a formal parameter,
   as in the following optimization:
   ```xl
   A - A    is 0
   ```
   Such a definition would require the evaluation of `X` and `2 * Y`
   in expression `X - 2 * Y` in order to check if they are equal.

4. When a conditional clause requires the evaluation of the corresponding
   binding, as in the following example:
   ```xl
   syracuse N when N mod 2 = 0  is N/2
   syracuse N when N mod 2 = 1  is N * 3 + 1
   syracuse X+5 // Must evaluate "X+5" for the conditional clause
   ```

Evaluation of sub-expressions is performed in the order required to
test pattern matching, and from left to right, depth first. Patterns
are tested in the order of declarations. Computed values for sub-expressions
are [memoized](#memoization), meaning that they are computed at most once
in a given statement.


## Lazy evaluation

In the cases where immediate evaluation is not required, an argument
will be bound to a formal parameter in such a way that an evaluation
of the formal argument in the body of the declaration will evaluate
the original expression in the original context. This is called
_lazy evaluation_. The original expression will be evaluated every
time the parameter is evaluated.

To understand these rules, consider the canonical definition of `while` loops:

```xl
while Condition loop Body is
    if Condition then
        Body
        while Condition loop Body
```

Let's use that definition of `while` in a context where we test the
[Syracuse conjecture](https://en.wikipedia.org/wiki/Collatz_conjecture):

```xl
while N <> 1 loop
    if N mod 2 = 0 then
        N /= 2
    else
        N := N * 3 + 1
    print N
```

The definition of `while` given above only works because `Condition`
and `Body` are evaluated multiple times. The context when evaluating
the body of the definition is somewhat equivalent to the following:

```
CONTEXT1 is
    Condition is N <> 1
    Body is
        if N mod 2 = 0 then
            N /= 2
        else
            N := N * 3 + 1
        print N
    CONTEXT0
```

In the body of the `while` definition, `Condition` must be evaluated
because it is tested against metabox `[[true]]` and `[[false]]` in the
definition of `if-then-else`. In that same definition for `while`,
`Body` must be evaluated because it is a statement.

The value of `Body` or `Condition` is not changed by them being evaluated.
In our example, the `Body` and `Condition` passed in the recursive
statement at the end of the `while Condition loop Body` are the same
arguments that were passed to the original invokation. For the same
reason, each test of `N <> 1` in our example is with the latest value
of `N`.

Lazy evaluation can also be used to implement "short circuit"
boolean operators. The following code for the `and` operator will not
evaluate `Condition` if its left operand is `false`, making this
implementation of `and` more efficient than the one given earlier:

```xl
[[true]]  and Condition is Condition
[[false]] and Condition is false
```


## Closures

The bindings given above for `Condition` and `Body` are somewhat simplistic.
Consider what would happen if you wrote the following `while` loop:

```xl
Condition is N > 1
while Condition loop N -= 1
```

Evaluating this would lead to a "naive" binding that looks like this:

```xl
CONTEXT2 is
    Condition is Condition
    Body is N -= 1
    CONTEXT0
```

That would not work well, since evaluating `Condition` would require
evaluating `Condition`, and indefinitely so. Something needs to be
done to address this.

In reality, the bindings must look more like this:

```xl
CONTEXT2 is
    Condition is CONTEXT1 { Condition }
    Body is CONTEXT1 { N-= 1 }
    CONTEXT0
```

The notation `CONTEXT1 { Condition }` means that we evaluate `Condition` in
context `CONTEXT1`. This one of the [scoping operators](#scoping),
which is explained in more details below. A prefix with a context on
the left and a block on the right is called a _closure_.

In the above example, we gave an arbitrary name to the closure, `CONTEXT1`,
which is the same for both `Condition` and `Body`. This name is
intended to underline that the _same_ context is used to evaluate both.
In particular, if `Body` contains a context-modifying operation like
`N -= 1`, that will modify the same  `N` in the same `CONTEXT1` that
will later be used to evaluate `N > 1` while evaluating `Condition`.

A closure may be returned as a result of evaluation, in which case all
or part of a context may need to be captured in the returned value,
even after that context would otherwise normally be discarded.

For example, consider the following code defining an anonymous function:

```xl
adder N is { lambda X is X + N }
add3 is adder 3     // Creates a function that adds 3 to its input
add3 5              // Computes 8
```

When we evaluate `add3`, a binding `N is 3` is created in a new context
that contains declaration `N is 3`. That context can simply be written
as `{ N is 3 }`. A context with an additional binding for `M is "Hello"`
could be written something like `{ N is 3; M is "Hello" }`.

The value returned by `adder N` is not simply `{ lambda X is X + N }`,
but something like `{N is 3} { lambda X is X + N }`, i.e. a closure that
captures the bindings necessary for evaluation of the body `X + N` at
a later time.

This closure can correctly be evaluated even in a context where there
is no longer any binding for `N`, like the global context after the
finishing the evaluation of `add3`. This ensures that `add3 5` correctly
evaluates as `8`, because the value `N is 3` is _captured_ in the closure.

A closure looks like a prefix `CONTEXT EXPR`, where `CONTEXT` and `EXPR`
are blocks, and where `CONTEXT` is a sequence of declarations.
Evaluating such a closure is equivalent to evaluating `EXPR` in the
current context with `CONTEXT` as a local context, i.e. with the
declarations in `CONTEXT` possibly shadowing declarations in the
current context.

In particular, if argument splitting is required to evaluate the expression,
each of the split arguments shares the same context. Consider the `write`
and `print` implementation, with the following declarations:

```xl
write Head, Tail        is write Head; write Tail
print Items             is write Items; print
```

When evaluating `{ X is 42 } { print "X=", X }`, `Items` will be
bound with a closure that captures the `{ X is 42 }` context:

```xl
CONTEXT1 is
    Items is { X is 42 } { "X=", X }
```

In turn, this will lead to the evaluation of `write Items`, where
`Items` is evaluated using the `{ X is 42 }` context. As a result, the
bindings while evaluating `write` will be:

```xl
CONTEXT2 is
    Head is CONTEXT1 { "X=" }
    Tail is CONTEXT1 { X }
    CONTEXT1 is { X is 42 }
```

The whole processus ensures that, when `write` evaluates `write Tail`,
it computes `X` in a context where the correct value of `X` is
available, and `write Tail` will correctly write `42`.


## Memoization

A sub-expression will only be computed once irrespective of the number
of overload candidates considered or of the number of tests performed
on the value. Once a sub-expression has been computed, the computed
value is always used for testing or binding that specific
sub-expression, and only that sub-expression.

For example, consider the following declarations:

```xl
X + 0               is Case1(X)
X + Y when Y > 25   is Case2(X, Y)
X + Y * Z           is Case3(X,Y,Z)
```

If you evaluate an expression like `A + foo B`, then `foo B` will be
evaluated in order to test the first candidate, and the result will be
compared against `0`. The test `Y > 25` will then be performed with
the result of that evaluation, because the test concerns a sub-expression,
`foo B`, which has already been evaluated.

On the other hand, if you evaluate `A + B * foo C`, then `B * foo C`
will be evaluated to match against `0`.  Like previously, the evaluated
result will also be used to test `Y > 25`. If that test fails, the third
declaration remains a candidate, because having evaluated `B * foo C`
does not preclude the consideration of different sub-expressions such
as `B` and `foo C`. However, if the evaluation of `B * foo C` required
the evaluation of `foo C`, then that evaluated version will be used as
a binding for `Z`.

> **RATIONALE** These rules are not just optimizations. They are necessary
> to preserve the semantics of the language during dynamic dispatch for
>  expressions that are not constant. For example, consider a call like
> `fib(random(3..10))`, which evaluates the `fib` function with a random
>  value between `3` and `10`. Every time `random` is evaluated, it returns
> a different, pseudo-random value. The rules above guarantee that the
> _same_ value will be used when testing against `0`, `1` or as a
> binding with `N`. Witout these rules, it would be possible for the body
> of the general case to be called with a value that is `0` or `1`.


## Self

In a definition body, `self` refers to the input tree. A special idiom
is a definition where the body is `self`, called a _self
definition_. Such definitions indicates that the item being defined
needs no further evaluation. For example, `true` and `false` can be
defined as:

```xl
true    is self
false   is self
```

This means that evaluating `true` will return `true`, and evaluating
`false` will return `false`, without any further evaluation. Note that
you cannot write for example `true is true`, as `true` in the body is
a statement, which would require further evaluation, hence an infinite
recursion.

It is possible to use `self` for data structures. For example, in
order to ensure that comma-separated lists are not evaluated, you can write :

```xl
X, Y    is self
```

Note that the following values also evaluate as themselves:

1. `integer`, `real` or `text` constants

2. Sequences of declarations, like `{ Zero is 0; One is 1 }`, in particular
   the contexts captured for [closures](#closures).


## Nested declarations

A definition body may itself contain declarations, which are called
_nested declarations_.

When the body is evaluated, a _local declaration phase_ will run,
followed by a _local evaluation phase_. The local declaration phase
will add the local declarations at the beginning of a new context,
which will be destroyed when the body evaluation terminates. The local
declarations therefore shadow declarations from the enclosing context.

For example, a function that returns the number of vowels in some text
can be written as follows:

```xl
count_vowels InputText is
    is_vowel C is
        Item in Head, Tail is Item in Head or Item in Tail
        Item in RefItem is Item = RefItem
        C in 'a', 'e', 'i', 'o', 'u', 'y', 'A', 'E', 'I', 'O', 'U', Y'

    Count : integer := 0
    for C in InputText loop
        if is_vowel C then
            Count += 1
    Count
count_vowels "Hello World" // Should return 3
```

This code example defines a local helper `is_vowel C` that checks if
`C` is a vowel by comparing it against a list of vowels. That local
helper is not visible to the outer program. You cannot use `is_vowel X`
in the outer program, since it is not present in the outer context. It
is, however, visible while evaluating the body of `count_vowels T`.

Similarly, the local helper itself defines an even more local helper
infix `in` in order ot evaluate the expression `C in 'a', 'e', ...`.

While evaluating `count_vowels "Hello World"`, the context will look
something like:

```xl
CONTEXT1 is
    is_vowel C is [...]
    Count:integer := 0
    InputText is "Hello World"
    CONTEXT0
```

In turn, while evaluating `is_vowel Char`, the context will look
somethign like:

```xl
CONTEXT2 is
    Item in Head, Tail is [...]
    Item in RefItem is [...]
    C is 'l'
    CONTEXT1
```

The context is sorted so that the innermost definitions are visible
first. Also, outer declarations are visible from the body of inner
ones. In the example above, the body of `is_vowel Char` could validly
refer to `Count` or to `InputText`.


## Scoping

A list of declarations, similar to the kind that is used in
[closures](#closures), is called a _map_ and evaluates as itself.
One of the primary uses for maps is _scoping_, in other words defining
a common _scope_ for the declarations that it contains. Since the
[declaration phase](#declaration-phase) operates on entire blocks, all
declarations within a scope are visible at the same time.

There are two primary operations that apply to a map:

1. _Applying_ a map as a prefix to an operand, as we saw with closures,
   evaluates the operand in the context defined by overlaying the map
   definitions on top of the current context.

2. _Scoping_ an expression within a map uses the infix `.` operator,
   where the expression on the right is evaluated in a context that
   consists _exclusively_ of the declarations in the map on the left.


Evaluating a closure is a prime example of map application. The context
is captured by the closure in a map, and the closure itself is a
prefix that corresponds to the map application. Such an expression can
also be created explicitly. For example, `{ X is 40; Y is 2 } { X + Y }`
will evaluate as `42`, taking `X` and `Y` from the map, and taking the
declaration used to evaluate `X + Y` from the current context.

Another common usage for maps is to store declarations where the
patterns are constant values. For example, you can use a map called
`digit_spelling` to convert a digit to its English spelling:

```xl
digit_spelling is
    0 is "zero"
    1 is "one"
    2 is "two"
    3 is "three"
    4 is "four"
    5 is "five"
    6 is "six"
    7 is "seven"
    8 is "eight"
    9 is "nine"
```

With this declaration, the expression `digit_spelling 3` evaluates to
`"three"`. This kind of map application is called _indexing_. A suggested
style choice is to make the intent more explicit using square brackets,
as in `digit_spelling[4]`. This is a nod to the syntax of programming
languages such as C or C++.

When the index is an expression, for example `digit_spelling[A+3]` in
a context where `A is 2`, we must evaluate `A+3` in current context
augmented with the declarations in `digit_spelling`. The first candidate
has pattern `0`.  This requires the evaluation of expression `A+3` to
check if it matches the value. As indicated [earlier](#pattern-matching),
this evaluation will not consider constants, since it is performed to match
a constant.  In other words, it will match the pattern `X+Y` for
`A+2`, and therefore compute the value `5`. That computed value will fail
the check against pattern `0`, but because of [memoization](#memoization),
it will then be used against the various constants in the map. As a result,
`digit_spelling[A+2]` evaluates as `"five"`.

A map is not restricted to constant patterns. For example, the
following map performs a more complete spelling conversion for numbers
below 1000 (the notation `\N` being a shortcut for `lambda N`):

```xl
number_spelling is
    \N when N<10    is digit_spelling[N]
    11              is "eleven"
    12              is "twelve"
    13              is "thirteen"
    14              is "fourteen"
    15              is "fifteen"
    16              is "sixteen"
    17              is "seventeen"
    18              is "eighteen"
    19              is "nineteen"
    20              is "twenty"
    30              is "thirty"
    40              is "forty"
    50              is "fifty"
    60              is "sixty"
    70              is "seventy"
    80              is "eighty"
    90              is "ninety"
    \N when N<100   is (number_spelling[N/10*10] & " " &
                        digit_spelling[N mod 10])
    \N when N<1000  is (digit_spelling[N/100] & " hundred and " &
                        digit_spelling[N mod 100])
```

Another common idiom is to use a named map to group related declarations.
This is the basis for the XL module system. For example, consider the
following declaration:

```xl
byte_magic_constants is
    num_bits    is 8
    min_value   is 0
    max_value   is 255
```

With that declaration, `byte_magic_constants.num_bits` evaluates to `8`.
A declaration like this can of course be more than a simple name:

```xl
magic_constants Bits is
    num_bits    is Bits
    min_value   is 0
    max_value   is 2^Bits - 1
```

In that case, `magic_constants(4).max_values` will evaluate to `15`.

This is also exactly what happens when you `use` a module. For example,
with `use IO = XL.CONSOLE.TEXT_IO`, a local name `IO` is created in
the current context that contains the declarations in the module. As a
result, `IO.write` will refer to the declaration in the module.


## Caller lookup

Any lookup


## Assignments

## Moves


## Functions as values

Unlike in several functional languages, when you declare a "function",
you do not automatically declare a named entity or value with the
function's name.

For example, the first definition in the following code does not
create any declaration for `my_function` in the context, which means
that the last statement in that code will cause an error.

```xl
my_function X is X + 1
apply Function, Value is Function(Value)
apply my_function, 1
```

> **RATIONALE** One reason for that choice is that [overloading](#overloading)
> means a multiplicity of declarations often need to be considered for
> a single expression. Another reason is that declarations can have
> arbitrarily complex patterns. It is not obvious what name should be
> given to a declaration of a pattern like `A in B..C`: a "name" like
> `in..` does not even "work" syntactically.
>
> It is not clear how such a name would be called as a function either,
> since some of the arguments may themselves contain arbitrary parse
> trees, as we have seen for the definition of `print`, where the
> single `Items` parameter may actually be a comma-separated list of
> arguments that will be split when calling `write Items` and matching
> it to `write Head, Tail`.

If you need to perform the operation above, it is however quite easy
to create a map that performs the operation. That map may be given a
name or be anonymous. The following code example shows two correct
ways to write such an `apply` call for a factorial definition:

```xl
0!                      is 1
N!                      is N * (N-1)!
apply Function, Value   is Function(Value)

// Using an anonymous map to compute 3!
apply { \N is N! }, 3

// Using a named map to compute 5!
factorial   is { \N is N! }
apply factorial, 5
```

Passing definitions like this might be seen as related to what other
languages call _anonymous functions_, or sometimes _lambda function_
in reference to Church's lambda calculus. The way this works, however,
is markedly different internally, and is detailed in the section on
[scoping](#scoping) above.

-------------------------------------------------------------------------------

Previous: [Syntax](HANDBOOK_1-syntax.md)
Next: [Type System](HANDBOOK_3-types.md)
