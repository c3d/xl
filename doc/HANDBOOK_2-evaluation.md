# XL program execution

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

In the examples below, `CONTEXT` will be a pseudo-variable that
describes the execution context. Where necessary, `CONTEXT0` will
represent the previous context.


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
   because `syntax` is designed to
   [modify the spelling and precedence](HANDBOOK_1-syntax.md#extending-the-syntax-in-your-program)
   of operators, and that information is used during the parsing
   phase.

3. Processing `import` statements: Since imported modules can contain
   `syntax` statements, they must at least partially be processed
   during parsing. Details about `import` statements are covered in
   the [chapter about modules](HANDBOOK_6-modules.md).

4. Identifying words that switch to a
   [child syntax](HANDBOOK_1-syntax.md#comment-block-text-and-syntax-separators):
   symbols that activate a child syntax are recognized during parsing.
   This is the case for example with the `extern` name in the
   [default syntax](../src/xl.syntax#L62).

5. Identifying binary data: words such as `bits` marked as introducing
   `BINARY` data in the syntax file are treated specially during
   parsing, to generate parse tree nodes representing binary data.
   >  **NOTE** this is not currently implemented.

The need to process `import` statements during parsing means that it's
not possible in XL to have computed `import` statements. The name of
the module must always be evaluated at compile-time.

> **RATIONALE** An alternative would have been to allow computed
> `import` statement, but disallow `syntax` in them. However, for
> convenience, `import` names look like `XL.CONSOLE.TEXT_IO` and not,
> say, `"xl/console/text_io.xs"`, so there is no obvious way to
> compute them anyway.

Once parsing completes successfully, the parse tree can be handed to
the declaration and evaluation phases. Parsing occurs for the
_entire program_, including imported modules, before the other phases
begin.


### Sequences

Both declaration and evaluation phases will process _sequences_, which
are one of:

* A block, in which case processing the sequence means processing the
  block's child
* An infix `NEWLINE` or semi-colon `;`, in which case the left and right
  operands of the infix are processed in that order.
* An `import` statement, which is the only statement that requires
  processing in all three executation phases.
* A `syntax` definition, which only plays a role during parsing is
  ignored during the declaration and evaluation phases.
* An infix `is`, which is called a _definition_, or an infix `:` or
  `as`, which are called _type annotations_. Definitions and type
  annotations are collectively called _declarations_, and are
  processed during the [declaration phase](#declaration-phase).
* Anything else, which is called a _statement_ and is processed during the
  [evaluation phase](#evaluation-phase).

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

A name like `Radius` is called a _formal parameter_, or simply
_parameter_.  Each parameter will receive its value from an _argument_
during the evaluation. For example the `Radius` parameter will be
_bound_ to argument`5.3` while evaluating the statement on the second line.

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
CONTEXT is
    pi is 3.14
    circumference Radius:real is 2 * pi * Radius
    CONTEXT0 // Previous context
```

The actual implementation is likely to store declarations is a more
efficient way, for example using some hashing or some form of balanced
tree. Such optimizations must preserve the order of declarations,
since correct behavior during the evaluation phase depends on it.

Note that since the declaration phase occurs before the execution
phase, all declarations in the program will be visible during the
evaluation phase. In our example, it is possible to use `circumference`
before it has been declared. Definitions may therefore refer to one
another in a circular way. Some other languages require "forward
declarations" in such cases, XL does not.

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
used to evaluate the body of the definition.

For example, the local context to evaluate the body of the definition
of `circumference Radius:real` would be:

```xl
CONTEXT is
    Radius is 5.3
    // Earlier context begins here
    pi is 3.14
    circumference Radius:real is 2 * pi * Radius
    CONTEXT0 // Previous context
```

As a reminder, `Radius` is a _formal parameter_, or simply _parameter_
that receives the _argument_ 5.3 as a result of _binding_. The binding
remains active for the duration of the evaluation of of the body of
the definition.

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
the declarations in the [standard library](../src/builtins.xl),
no declaration has a larger pattern like `X * Y * Z` that could match
the whole expression. However, there is a definition for a
multiplication between `real` numbers, with a pattern that looks like
`X:real * Y:real as real`, as well as another for `integer`
multiplication, with a pattern that looks like `X:integer * Y:integer`.
There may be more, but we will ignore them for the rest of the discussion.

The `*` operator is left-associative, so `2 * pi * Radius` parses as
`(2 * pi) * Radius`. Therefore, we will be looking for a match with
`X` corresponding to `2 * pi` and `Y` corresponding to `Radius`.
However, that information alone is insufficient to determine if either
sub-expression is `integer` or `real`. In order to be able to make
that determination, [immediate evaluation](#immediate-evaluation) of
the arguments is required.

The evaluation process therefore repeats with sub-expression `2 * pi`,
and like before, it is necessary to evaluate `pi`. This in turns gives
the result `3.14` given the current context. That result replaces
`pi`, so that we now must evaluate `2 * 3.14`.

This tree does not match `X:real * Y:real` because `2` is an `integer`
and not a `real`. It does not match `X:integer * Y:integer` either
because `3.14` is a `real` and not an `integer`. However, the standard
library provides a definition of an _implicit conversion_ that looks
something like this:

```xl
X:integer as real is /* some implementation here */
```

This implicit conversion tells the compiler how to transform an
`integer` value like `2` into a `real`. Implicit conversions are only
considered if there is no exact match, and only one of them can be
used to match a given parameter. In our case, there isn't an exact
match, so the compiler will consider the implicit conversion to get a
`real` from `integer` value `2`.

The body of the implicit conversion above is therefore evaluated in a
context where `X` is set to `2`:

```xl
CONTEXT is
    X is 2
    // Earlier context begins here
    Radius is 5.3
    // Earlier context begins here
    pi is 3.14
    circumference Radius:real is 2 * pi * Radius
    CONTEXT0 // Previous context
```

The result of that implicit conversion is `2.0`. Evaluation can
then resume with the `X:real * Y:real as real` definition, this time
called with an argument of the correct `real` type for `X`:

```xl
CONTEXT is
    X is 2.0
    Y is 3.14
    // Earlier context here
    Radius is 5.3
    // Earlier context begins here
    pi is 3.14
    circumference Radius:real is 2 * pi * Radius
    CONTEXT0 // Previous context
```

The result of the multiplication is a `real` with value `6.28`, and
after evaluating `Radius`, evaluation of the second multiplication
will then happen with the following context:

```xl
CONTEXT is
    X is 6.28 // from 2 * pi
    Y is 5.3  // from Radius
    // Earlier context here
    Radius is 5.3
    // Earlier context begins here
    pi is 3.14
    circumference Radius:real is 2 * pi * Radius
    CONTEXT0 // Previous context
```

The result of the last multiplication is a `real` with value
`33.284`. This is the result of evaluating `circumference 5.3`, and
consequently the result of executing the entire program.

> **NOTE** The standard XL library only provides implicit conversions
> that do not cause data loss. On most implementation, `real` has a
> 53-bit mantissa, which means that the implicit conversion from
> `integer` to `real` is actually closer to the following:
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

* The top-level pattern `P` is a name, and expression `E` is the same name.
  For example, `pi is 3.14` defines a name `pi`, and the only way to match
  this pattern is with the expression `pi`.

* The top-level pattern `P` is a prefix, like `sin X` or a postfix, like
  `X%`, and the expression `E` is a prefix or  postfix respectively
  with the same operator as the pattern, and operands match.
  For example, pattern `sin X:real` will match expression `sin (A+B)`
  if `(A+B)` has the `real` type, but it will not match a prefix with
  any other name, like `cos C`. Similarly, pattern `X%` will match
  `3%` but not `3!`, because the operator is not the right one.

* The top-level pattern `P` is an infix `when` like `Pattern when Condition`,
  called a _conditional pattern_, and `Pattern` matches `E`, and the
  value of `Condition` is `true` with the bindings declared while
  matching `Pattern`. For example, `log X when X > 0 is ...` will
  match `log 42` but not `log(-1)`.

* The pattern `P` is an infix, and the operator is the same in the pattern
  and in the expression, and both operands in the pattern match the
  respective operands in the expression. For example, pattern
  `X:integer+Y:integer` will match `2+3` but not `A-B` nor `"A" + "B"`.
  This works both for patterns and sub-patterns. For sub-patterns, if
  `E` is a name, then its value can be _decomposed_. For example,
  `write Items` can match `write Head, Tail` if there is a binding
  like `Items is "Hello", "World"` in the current context.

  > **NOTE** A very common idiom  is to use comma `,` infix to separate
  > multiple parameters, as in the following definition:
  > ```xl
  > write Head, Tail is write Head; write Tail
  > ```
  > This declaration will match `write 1, 2, 3` with bindings `Head is 1` and
  > `Tail is 2,3`. In the evaluation of the body with these bindings,
  > `write Tail` will then match the same declaration again with
  > `Tail` being decomposed, resulting in bindings `Head is 2` and `Tail is 3`.

* The pattern `P` is an `integer`, `real` or `text` constant, or a
  [metabox](#metabox), for example `0`, `1.25`, `"Hello"` or `[[sqrt 2]]`
  respectively, and the expression `E` matches the value given by the
  constant or computed by the expression in the metabox. In that case,
  expression `E` needs to be evaluated to check if it matches. This
  case applies to sub-patterns, as in `0! is 1`, as well as to
  top-level patterns, as in `0 is "Zero"`. This last case is useful in
  [maps](#scoping).

* The sub-pattern `P` is a name possibly with a type annotation, such
  as `N` in `N! is N * (N-1)!` or `X:real` in `sin X:real is cos(x -
  pi/2)`. If a type is given, expression `E` may be evaluated as
  necessary to check the type. For example, expression `A+B` will have
  to be evaluated to match `X:real`, but that would not be necessary
  for `X:infix` because `A+B` is already an infix. In that case, a
  binding `P is E2` is added at the top of the current context, where
  `E2` is the evaluated value of `E` if `E` was evaluated, and `E`
  otherwise.

* The sub-pattern `P` is a name that was already bound in the same
  top-level pattern, and  expression `P = E` is `true`. For example,
  `A - A is 0` will match expression `X - Y` if `X = Y`, or more
  precisely, if `Y = A` is `true` in a context where `A is X`.

* The pattern is a block with a child `C`, and `C` matches `E`.
  In other words, blocks in a pattern only change the precedence, but play no
  other role in pattern matching. This would be the case for
  `(X+Y) * (X-Y) is X^2 - Y^2`, which would match `[A+3] * [A-3]`.
  Note that the delimiters of a block cannot be tested that way, you
  should use a conditional pattern like `B:block when B.opening = "("`.

In some cases, pattern matching requires evaluation of an expression
or sub-expression. This is called [immediate evaluation](#immediate-evaluation).
Otherwise, [evaluation will be deferred](#deferred-evaluation).


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
> semantics of XL, notably for dynamic dispatch, where the
> runtime cost of finding the proper candidate might be a bit too high
> to be practical.

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

A binding may contain a value that may itself need to be _decomposed_
in order to be tested against the formal parameters. This is used in
the implementation of `write_line`:

```xl
write_line Items        is write Items; write_line
write Head, Rest        is write Head; write Rest
write Item:integer      is /* implementation for integer */...
write Item:real         is /* implementation for real */...
```

In that case, finding the declaration matching `write_line "Hello",
"World"` involves creating a binding like this:

```xl
CONTEXT is
    Items is "Hello", "World"
    CONTEXT0
```

When evaluating `write Items`, the various candidates for `write`
include `write Head, Rest`, and this will be the one selected after
decomposition of `Items`, causing the context to become:

```xl
CONTEXT is
    Head is "Hello"
    Rest is "World"
    CONTEXT0
```


## Dynamic dispatch

As shown above, the declaration that is actually selected to evaluate
a given parse tree may depend on the dynamic value of the
arguments. In the Fibonacci example above, `fib(N-1)` may select any
of the three declarations of `fib` depending on the actual value of
`N`. This runtime selection of declarations based on the value of
arguments is called _dynamic dispatch_. Dynamic dispatch is an
important feature to support well-known techniques such as
object-oriented programming.

Testing can happen for the value of multiple arguments. Consequently,
dynamic dispatch in XL can apply to multiple arguments, an approach
that is sometimes called _multi-methods_ in the object-oriented
world. This is very different from C++, for instance, where built-in
dynamic dispatch ("virtual functions") works along a single axis at a
time, and only based on the type of the value.

This makes the XL version of dynamic dispatch sometimes harder to
optimize, but has interesting use cases. Consider for example the
archetypal object-oriented `shape` class, with derived classes such as
`rectangle`, `circle`, `polygon`, and so on. In C++, it is easy to
impement a `Shape::Draw` method, because selecting the correct
implementation depends on a single argument, and this is the kind of
example that is often given in C++ books to illustrate the benefits of
dynamic dispatch.

On the other hand, implementing a test detecting if two shapes
intersect is not as easy in C++, since in that case, the correct
implementation depends the type of two arguments, not just one. With XL,
it is straightforward to implement, even if somewhat verbose:

```xl
X:shape     intersects Y:shape      as boolean  is ... // general case
X:rectangle intersects Y:rectangle  as boolean  is ... // two rectangles
X:circle    intersects Y:circle     as boolean  is ... // two circles
X:circle    intersects Y:rectangle  as boolean  is ... // two circles
X:rectangle intersects Y:circle     as boolean  is ... // two circles
...
```

As another illustration, Tao3D uses theme functions that depend on the
slide "theme", the slide "master" and the slide "element", as in:

```xl
theme_font "WhiteChristmas", "main", "title"    is font "Alex Brush"
theme_font SlideTheme, SlideMaster, SlideItem   is font "Arial"
```


## Immediate evaluation

In the `circumference` examples, matching `2 * pi * Radius` against the
possible candidates for `X * Y` expressions required an evaluation of
`2 * pi` in order to check whether it was a `real` or `integer` value.

This is called _immediate evaluation_ of arguments, and is required
in XL for statements, but also in the following cases:

1. When the formal parameter being checked has a type annotation, like
   `Radius` in our example, and when the annotation type does not
   match the type associated to the argument parse tree. Immediate
   evaluation is required in such cases order to check if the argument
   type is of the expected type after evaluation.  Evaluation is *not*
   required if the argument and the declared type for the formal
   parameter match, as in the following example:
   ```xl
   write X:infix   is  write X.left, " ", X.name, " ", X.right
   write A+3
   ```
   In that case, since `A+3` is already an `infix`, it is possible
   to bind it to `X` directly without evaluating it.

2. When the part of the pattern being checked is a constant or a
   [metabox](#metabox). For example, this is the case in the definition of the
   factorial below, where the expression `(N-1)` must be evaluated in order
   to check if it matches the value `0`, in order to verify if
   `(N-1)!` matches `0!`, or in the definition of `if-then-else`,
   where the condition must be evaluated to be checked against `true`
   or `false`:
   ```xl
   0! is 1
   N! is N * (N-1)!
   if [[true]]  then TrueBody else FalseBody    is TrueBody
   if [[false]] then TrueBody else FalseBody    is FalseBody
   ```

3. When the same name is used more than once for a formal parameter,
   as in the following optimmization:
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

Immediate evaluation for expressions is performed in the order required
to test pattern matching, and from left to right, depth first, while testing
a given pattern.

In the case of overloading, a sub-expression will only be computed
once irrespective of the number of overload candidates. Once a
sub-expression has been computed, the computed value is always used
when testing against following candidates.

For example, in the following code, the second declaration will not be
selected, because the sub-expression `B*C` has been evaluated to test
against `0` in the first declaration:

```xl
X + 0       is Case1(X)
X + Y * Z   is Case2(X,Y,Z)
A + B * C
```

If the intent is to allow `Case2` to be called, then the declaration
of `X + Y * Z` must be placed before the declaration of `X + 0`.


## Deferred evaluation

In the cases where immediate evaluation is not required, an argument
will be bound to a formal parameter as is, without evaluation. This is
called _deferred evaluation_. In that case, the argument will be
evaluated every time this is required to evaluate the formal argument.

Consider the canonical definition of `while` loops:

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
    write_line N
```

The definition of `while` given earlier only works because `Condition`
and `Body` are evaluated multiple times. The context when evaluating
the body of the definition is somewhat equivalent to:

```
CONTEXT is
    Condition is N <> 1
    Body is
        if N mod 2 = 0 then
            N /= 2
        else
            N := N * 3 + 1
        write_line N
    CONTEXT0
```

In the body of the `while` definition, `Condition` must be evaluated
because it is tested against known value `true` in the definition of
`if-then-else`:

```xl
if [[true]]  then TrueBody      is TrueBody
if [[false]] then TrueBody      is false
```

In that same definition for `while`, `Body` must be evaluated because
it is a statement.

The value of `Body` or `Condition` is not changed by them being evaluated.
In our example, the `Body` and `Condition` passed in the recursive
statement at the end of the `while Condition loop Body` are the same
arguments that were passed to the original invokation. For the same
reason, each test of `N <> 1` in our example is with the latest value
of `N`.

Deferred evaluation can also be used to implement "short circuit"
boolean operators. The following code for the `and` operator will not
evaluate `Condition` if its left operand is `false`.

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
CONTEXT is
    Condition is Condition
    Body is N -= 1
    CONTEXT0
```

That would not work well, since evaluating `Condition` would require
evaluating `Condition`, and indefinitely so. Something needs to be
done to address this.

In reality, the bindings must look more like this, which is called a _closure_:

```xl
CONTEXT is
    Condition is CONTEXT0.Condition
    Body is CONTEXT0.{ N-= 1 }
    CONTEXT0
```

The notation `CONTEXT0.Condition` means that we evaluate `Condition` using
`CONTEXT0` as the context. This is called the [scoping operator](#scoping).
A context that refers to an earlier context is called a _closure_.

A closure may be returned as a result of evaluation, in which case all
or part of a context may have to be captured in the closure even after
it would otherwise be normally discarded.

For example, consider the following code:

```xl
adder N is  (lambda X is X + N)
add3 is adder 3
add3 5
```

When we evaluate `add3`, a binding `N is 3` is created. Then `adder N`
returns `(lambda X is X + N)` with a closure that captures this binding.
The returned value for `add3` will therefore be a closure similar to
the following:

```xl
(N is 3).(lambda X is X + N)
```

 This closure can correctly be evaluated even in a context where there
 is no binding for `N`, like the global context after the evaluation
 of `add3`.



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

    Count is 0
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
CONTEXT is
    is_vowel C is [...]
    Count is 0
    InputText is "Hello World"
    CONTEXT0
```

In turn, while evaluating `is_vowel Char`, the context will look
somethign like:

```xl
CONTEXT is
    Item in Head, Tail is [...]
    Item in RefItem is [...]
    C is 'l'
    is_vowel C is [...]
    Count is 1
    InputText is "Hello World"
    CONTEXT0
```

The context is sorted so that the innermost definitions are visible
first. Also, outer declarations are visible from the body of inner
ones. In the example above, the body of `is_vowel Char` could validly
refer to `Count` or to `InputText`.


## Scoping

A common idiom in XL is to have a body that consists only of declarations,
without any statement. Definitions that consist of only declarations
are called _maps_. When evaluating a map, the result is the map,
i.e. the list of declarations in the body.

For example, evaluating `digit_spelling` in the code below returns the
sequence of declarations in the body:

```xl
digit_spelling is
    0 is "Zero"
    1 is "One"
    2 is "Two"
    3 is "Three"
    ...
```

There are two primary use cases for maps.

1. _Indexing_ uses the map as a prefix, for example `digit_spelling 0`
   which evaluates the operand in the current context, and then
   evaluates the result as if with the scoping operator. Indexing is
   often written using square brackets, as in `digit_spelling[0]`, in
   order to better express the intent. This notation is a nod to
   existing practice in other programming languages such as C.

2. The _scoping operator_ is an infix `.` with the map on the left,
   such as `digit_spelling.0`. This evaluates the right operand in a
   context that consists _only_ of the declarations of the value on
   the left, excluding the current context.

In short, the difference between indexing and scoping is that indexing
evaluates in the current context, then evaluates the result within the
map, whereas scoping only evaluates within the map. In both cases, it
is an error if there is no match within the map.

The following code illustrates the difference between the two:

```xl
A is 0
my_map is
    0 is "Zero"
    1 is "One"
    Name is "My map"
my_map[0]       // Valid, returns "Zero"
my_map.0        // Valid, returns "Zero"
my_map[A]       // Valid, returns "Zero"
my_map.A        // Invalid, there is no 'A' in my_map
my_map[Name]    // Invalid, no "Name" visible here
my_map.Name     // Valid, returns "My map"
```

There is no real need for the map to be given a name. In particular,
"anonymous functions" can be used for indexing and scoping, as in the
following example:

```xl
(X is X + 1) (3 + 4)        // Computes 8
(X is X + 1).(3 + 4)        // Returns
```


A primary use for maps is to be used as a prefix, which is called
_indexing_ the map, or alternatively, using the _scoping operator_,
which is an infix `.`. In the case of `digit_spelling`, the first case
would be an expression like `digit_spelling 0`, or often, in order to
indicate the intent

Using a map as a prefix evaluates the right-hand side as if it was a
single statement at the end of the


```xl
fib is
    0 is 1
    1 is 1
    N is (fib(N-1) + fib(N-2))
```

This definition can be used almost like the definition




The dot notation `X.Y` in XL is called the _scoping operator_. It
indicates that



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

One reason for that choice is that [overloading](#overloading) means a
multiplicity of declarations must often be considered for a single
expression. Another reason is that declarations can have arbitrarily
complex patterns. It is not obvious what name should be given to a
declaration of a pattern like `A in B..C`. A name like `in..` does not
even "work" syntactically.

It is not clear how such a name would be called as a function either,
since some of the arguments may themselves contain arbitrary parse
trees, as we have seen for the definition of `write_line`, where the
single `Items` parameter may actually be a comma-separated list of
arguments that will be decomposed when calling `write`.

XL offers a neat solution to these problems, however. Consider the
standard definition of `factorial`:

```xl
0! is 1
N! is N * (N-1)!
apply Function, Value is Function(Value)
apply (!), 6
```

In that case, there are two declarations that constitute the
definition of the factorial notation `N!`, so it does not really make
sense to pass `!` around. This example illustrates another obvious
problem because `N!` is an operator notation, and it's not completely
self-evident that its name should be something like `!`. How would you name
the pattern `A*B+C*D`? You can't name it `*+*` since that could be an
operator in its own right.

If you really want to expose the notation `N!` as a "function object",
you need to explicitly do so. This is illustrated below with the
definition of factorial below that is correct XL.


```xl
0! is 1
N! is N * (N-1)!
factorial is (N is N!)
apply Function, Value is Function(Value)
apply factorial, 6
```

You don't even need to give an explicit name to your function. Instead, you
can pass a definition as an argument, as in the following code:

```xl
0! is 1
N! is N * (N-1)!
apply Function, Value is Function(Value)
apply (N is N!), 6
```

Passing definitions like this might be seen as related to what other
languages call _anonymous functions_, or sometimes _lambda function_
in reference to Church's lambda calculus. The way this works, however,
is markedly different internally, and is detailed in the section on
[scoping](#scoping) below.




### Tail call optimization
