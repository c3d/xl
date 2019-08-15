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
   because `syntax` is designed to modify the spelling and precedence
   of operators, and that information is used during the parsing
   phase.

3. Processing `import` statements: Since imported modules can contain
   `syntax` statements, they must at least partially be processed
   during parsing.

4. Identifying words that switch to a
   [child syntax](HANDBOOK_1-syntax.md#comment-block-text-and-syntax-separators):
   symbols that activate a child syntax are recognized during parsing.
   This is the case for example with the `extern` name in the
   [default syntax](../src/xl.syntax#L62).

5. Identifying binary data: words such as `bits` marked as introducing
   `BINARY` data in the syntax file are treated specially during
   parsing, to generate parse tree nodes representing binary data.
   *Note:* this is not currently implemented.

The need to process `import` statements during parsing means that it's
not possible in XL to have computed `import` statements. The name of
the module must always be evaluated at compile-time.

> *RATIONALE* An alternative would have been to allow computed
> `import` statement, but disallow `syntax` in them. However, for
> convenience, `import` names look like `XL.CONSOLE.TEXT_IO` and not,
> say, `"xl/console/text_io.xs"`, so there is no obvious way to
> compute them anyway.

Once parsing completes successfully, the parse tree can be handed to
the declaration and execution phases.


### Sequences

Both declaration and evaluation phases will process _sequences_, which
are one of:

* A block, in which case processing the sequence means processing the
  block's child
* An infix "newline" or semi-colon, in which case the left and right
  operands of the infix are processed in that order
* An infix `is` is called a _definition_. An infix `:` or `as` is
  called a _type declaration_. Definitions and type declarations are
  collectively called _declarations_, and are processed during the
  [declaration phase](#declaration-phase).
* Anything else is called a _statement_ and is processed during the
  [evaluation phase](#evaluation-phase).

For example, consider the following code:

```xl
pi is 3.14
circumference 5.3
circumference Radius:real is 2 * pi * Radius
```

THe first and last line are each representing a definition of `pi` and
`circumference Radius:real` respectively. The second line is made of one
statement that computes `circumference 5.3`. There are two
definitions, one statement and no type declaration in this code.

Note that there is a type declaration of `Radius` in the definition on
the last line, but that type declaration it is _local_ to the
definition, and consequently not part of the declarations in the
top-level sequence.

A name like `Radius` is called a _formal parameter_, or simply
_parameter_.  Each parameter will receive its value from an _argument_
during the evaluation, for example `5.3` while evaluating the
statement on the second line.

The _result_ of executing any XL code is the value of its last
statement. In that case the result of executing the code will be the
value computed by `circumference 5.3`.


### Declaration phase

During the declaration phase, all declarations are stored in order in
the context, so that they appear before any declaration that was
already in the context.

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

Note that since the declaration phase occurs first, all declarations
in the program will be visible during the evaluation phase. In our
example, it is possible to use `circumference` before it has been
declared. Definitions may therefore refer to one another in a circular
way. Some other languages require "forward declarations" in such
cases, XL does not.

The parse tree on the left of `is`, `as` or `:` is called the
_pattern_ of the declaration. The pattern will be checked against the
_form_ of parse trees to be evaluated. The right operand of `:` or
`as` is called the _type_ of the type declaration. The parse tree on
the right of `is` is called the _body_ of the definition.


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
the declarations in the [standard library](../src/builtins.xl), we
will find a definition for `X:real * Y:real as real` as well as a
definition for `X:integer * Y:integer`. There is nothing that directly
matches a larger pattern like `X * Y * Z`.

The `*` operator is left-associative, so `2 * pi * Radius` parses as
`(2 * pi) * Radius`. In order to check which of the `X * Y` candidates
could apply, `X` cannot be simply bound to `2 * pi` and `Y` to
`Radius`, because we don't know if they are `real` or `integer` or
something else. In that case, the program must evaluate `2 * pi` to
compare it against the possible `X`.

The processus therefore repeats with sub-expression `2 * pi`, and like
before, it is necessary to evaluate `pi`. This in turns gives the
result `3.14` given the current context. That result replaces `pi`, so
that we now evaluate `2 * 3.14`.

This tree does not match `X:real * Y:real` because `2` is an `integer`
and not a `real`. It does not match `X:integer * Y:integer` either
because `3.14` is a `real` and not an `integer`. However, there is a
definition of an _implicit conversion_ that looks like this:

```xl
X:integer   as real is ...
```

An implicit conversion like this tells the compiler how to implicitly
transform an `integer` value like `2` into a `real`. Such an implicit
conversion is only considered if there is no exact match. In our case,
there isn't an exact match, so the compiler will consider the implicit
conversion to get a `real` from `integer` value `2`.

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


## Pattern matching

As we have seen above, the key to execution in XL is _pattern matching_,
which is the process of finding the declarations patterns that match a
given parse tree.

A pattern `P` matches an expression `E`` if:

* The pattern is an `integer`, `real` or `text` constant, and the
  expression has the same type and the same value. For example,
  pattern `0` matches `integer` value `0`.

* The pattern is a name that is declared in the current context, and
  the expression `P = E` is true. For example, you can define an
  optimization `sin pi is 0.0` that will enforce the mathematical
  equality despite possible rounding errors.

* The pattern is a name `N` that is not declared in the current context.
  In that case, a new binding `N is E` will be added add the beginning
  of the context. This is the case for example, in `N! is N * (N-1)!`

* The pattern is a type declaration like `Name : Type`, in which case the
  expression must have the specified `Type`. In that case too, a new
  binding `Name is E` will be added at the beginning of the
  context. This is the case for pattern `X:integer + Y:integer`.

* The pattern is a type declaration like `SubPattern as Type`, in
  which the expression must have the correct `Type`. This is the case
  for `X:integer + Y:integer as integer` matching `2+3-8`: the compiler
  can deduce that `2+3` has `integer` type and can therefore proceed
  using `X:integer - Y:integer` for the `-` infix.

* The pattern is an infix `SubPattern when Condition`, and
  `SubPattern` matches the expression, and the `Condition` is true.
  Bindings created while testing `SubPattern` are present in the
  context used to evaluate `Condition`. For example, you can match
  only even `integer` values using `N:integer when N mod 2 = 0`.

* The pattern is a block with a child `C`, and `C` matches `E`.
  This would be the case for `(X+Y) * (X-Y) is X^2 - Y^2`.
  Note that the delimiters of a block cannot be tested that way, you
  should use a conditional pattern like `B:block when  B.opening = "("`.

* The pattern is an infix with name `I`, the expression is an infix
  with the same name, and the two pattern operands match the
  respective expression operands. For example, patten `X + Y`
  matches expression `2+3*5`.

* The pattern is the a prefix or postfix, the expression is
  respectively a prefix or postfix, and pattern operator and operand
  both match expression operator and operand respectively. For
  example, `(A+B)(C+D)` matches `(1+2)(3+4)`.  If the pattern is the
  outermost prefix or postfix, and if the pattern operator is a name,
  then the expression operator must be a name too, with the
  same value. For example, `sin X` matches `sin (3.1*pi)` and `X%`
  matches `4%`.


### Overloading

There may be multiple declarations where the pattern matches the
shape. This is called _overloading_. For example, as we have seen
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
declaration matches if it matches the whole shape of the tree. For
example, `X+1` can match any of the declarations patterns below:

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

Knowing which candidate matches may be possible at compile-time, or
may require run-time tests against the values in the declaration. This
depends on the shape of the tree being evaluated and on the available
declarations in the context,

> *NOTE* The XL2 implementation does not select the first that
> matches, but the _largest and most specialized_ match. This is a
> slightly more complicated implementation, but not by far, and it has
> some benefits, notably with respect to making the code more robust
> to reorganizations. For this reason, this remains an open option.
> However, it is likely to be more complicated with the more dynamic
> semantics of XL, notably for dynamic dispatch, where the
> runtime cost of finding the proper candidate might be a bit too high
> to be practical.

For example, a definition of the
[Fibonacci sequence](https://en.wikipedia.org/wiki/Fibonacci_number)
in XL is given below:

```xl
fib 0   is 0
fib 1   is 1
fib N   is (fib(N-1) + fib(N-2))
```

> *NOTE* Parentheses are required around the
> [expressions statements](HANDBOOK_1-syntax.md#tweak-1-expression vs-statement).
> in the last declaration in order to parse this as the addition of
> `fib(N-1)` and `fib(N-2)` and not as the `fib` of `(N-1)+fib(N-2)`.

When evaluating a sub-expression like `fib(N-1)`, three candidates for
`fib` are available, and type information is not sufficient to
eliminate any of them. The generated code will therefore evaluate
`N-1`, something called [immediate evaluation](#immediate-evaluation),
in order to compare the value against the candidates. If the value is
`0`, the first definition will be selected. If the value is `1`, the
second definition will be used. Otherwise, the third definition will
be used.

The version of the declaration that is actually used depends on the
dynamic value of `N`. This is the basis for _dynamic dispatch_ in XL,
i.e. runtime selection of the code to execute depending on some values.
Dynamic dispatch is an important feature to support techniques such as
object-oritented programming.

Testing can happen for multiple parameters, and so does dynamic
dispatch in XL. This is very different from C++, for instance, where
built-in dynamic dispatch ("virtual functions") works along a single
axis at a time, and only based on the type of the value.

This makes the XL version sometimes harder to optimize, but has
interesting use cases. For example, Tao3D uses theme functions that
depend on the slide "theme", the slide "master" and the slide
"element", as in:

```xl
theme_font "WhiteChristmas", "main", "title"    is font "Alex Brush"
theme_font SlideTheme, SlideMaster, SlideItem   is font "Arial"
```


## Pattern matching

Consider the following examples:

```xl
A in B..C as boolean
A in B..C is A >= B and A <= C
```

The first type declaration tells the compiler that a pattern like
`A in B..C` is supposed to evaluate as a `boolean` value. The
definition that follows has the same pattern `A in B..C`, and its body
is `A >= B and A <= C`.



The names in that pattern are


The parse tree on the left of `:` or `as` is also called a pattern,
except in the case where it consists in a single name, in which case
it is called a _variable_. For example, in following two type
declarations, the first one declares a variable named `X`, whereas the
second one declares a pattern for the multiplication of `integer`
values:

```xl
X:integer                           // Declares an integer variable named X
X:integer + Y:integer as integer    // Declares that 2+3 is an integer
```


### Creating a functional object

Unlike in several functional languages, when you declare a "function",
you do not automatically declare an object with the function's
name.

For example, the first definition in the following code does not
create any declaration for `my_function` in the context, which means
that the last statement in that code will cause an error.

```xl
my_function X  is X + 1
apply Function, Value is Function(Value)
apply my_function, 1
```

The reason for that is that overloading means a multiplicity of
declarations must often be considered for a single
expression. Consider the standard definition of `factorial`:

```xl
factorial 0 is 1
factorial N is N * factorial(N-1)
apply Function, Value is Function(Value)
apply factorial, 6
```

In that case, there are two declarations that "constitute" `factorial`,
so it does not really make sense to pass `factorial` around. If you
really want to expose `factorial` as a "function object", you need to
explicitly do so. This is illustrated below with a definition of
`factorial` that is correct XL.


```xl
0! is 1
N! is N * (N-1)!
factorial is (N is N!)
apply Function, Value is Function(Value)
apply factorial, 6
```

Note that this is not really idiomatic XL, since you don't need to
give an explicit name to your function. Instead, you can pass a
declaration like:

```xl
0! is 1
N! is N * (N-1)!
apply Function, Value is Function(Value)
apply (N is N!), 6
```

### Immediate evaluation

In the previous examples, matching `2 * pi * Radius` against the
possible candidates for `X * Y` expressions required an evaluation of
`2 * pi` in order to check whether it was a `real` or `integer` value.

This is called _immediate evaluation_ of arguments, and is required in
XL in the following cases:

1. When the formal parameter being checked has a type declaration,
   like `Radius` in our example, and when the type does not match the
   argument. Immediate evaluation is required in such cases order to
   check if the argument type is of the expected type after evaluation.
   Evaluation is *not* required if the argument and the declared type
   for the formal parameter match. as in the following example:
   ```xl
   write X:infix   is  write X.left, " ", Xlname, " ", X.right
   write A+3
   ```
   In that case, since `A+3` is already an `infix`, so it is possible
   to bind it to `X`.

2. When the part of the pattern being checked is a constant. For
   example, consider the definition of the factorial. where the
   expression `(N-1)` must be evaluated in order to check if it
   matches the value `0`, in order to verify if `(N-1)!` matches `0!`:
   ```xl
   0! is 1
   N! is N * (N-1)!
   ```

3. When the name of the formal parameter that is not part of a type
   declaration already exists. For example, the standard definition of
   `if` tests in XL refers to pre-existing names `true` and `false`:
   ```xl
   if true  then TrueBody else FalseBody    is TrueBody
   if false then TrueBody else FalseBody    is FalseBody
   ```
   This is also the case if the name of the same formal parameter is
   used several times in the same pattern, as in:
   ```xl
   A - A    is 0
   ```
   Such a definition would require the evaluation of `X` and `2 * Y`
   in expression `X - 2 * Y` in order to check if they are equal.

A pattern may not match the expected forms if a formal parameter
happens to unintentionally use a name that already exists in the
context. For instance, the following definition and use of `select`
will fail to compile, because `true` and `false` are defined, XL is
case-insensitive, and `"Equal"` does not match `true`.

```xl
select Condition, True, False     is    if Condition then True else False
select X = Y, "Eq", "Diff"
```

This problem is called namespace pollution, and is common to most
languages in one shape or another. For example, C has a similar
problem with `#define` macros, where you can for example
`#define x y-1` if you want to really annoy your co-workers. A minimal
amount of care is normally sufficient to avoid the problem.

The simplest solution is to change the name of the formal parameters
so that they don't conflict with any visible declaration. Another,
more verbose approach is to specify the expected types, for example:

```xl
select Condition:boolean, True:anything, False:anything is
    if Condition then True else False
select X = Y, "Eq", "Diff"
```

If there are multiple candidates being considered,



### Deferred evaluation

In the cases where immediate evaluation


This is generally the case when the pattern being checked contains an
explicit

_type annotation_, i.e. an infix `:` form `Parameter : Type`.

The [XL type system](HANDBOOK_3-types.md] is described in more details
in the next chapter. For now, suffice it to say that the presence of a
type annotation for a formal parameter is one way to force the
evaluation of the arguments before executing the implementation of the declaration.


### Closures


### Scoping

## Declaration-only bodies



### Tail call optimization



## Compiled representations

Code and any data can also have one or several _compiled forms_. The
compiled forms are generally very implementation-dependent, varying
with the machine you run the program on as well as with the compiler
technology being used.
