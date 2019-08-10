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

Executing an XL program is the result of two phases, a _declaration phase_,
where all the declarations are stored in the context, followed by an
_evaluation phase_, where statements other than declarations are
processed in order.

In the following description, `CONTEXT` will be the name of a
pseudo-variable that describes the execution context. Where necessary,
`CONTEXT0` will describe the context before execution.


### Sequences

Both declaration and evaluation phases will process _sequences_, which
are one of:

* A block, in which case processing the sequence is processing the
  block's child
* An infix "newline" or semi-colon, i.e. statements on separate lines
  or separated by a semi-colon, in which case the left and right
  operands of the infix are processed in that order
* An infix `is` is called a _declaration_
* Anything else is called a _statement_

For example, consider the following code:

```xl
pi is 3.14
circumference 5.3; circumference 2.5
circumference Radius:real is 2 * pi * Radius
```

THe first and last line are each representing a declaration of `pi` and
`circumference Radius:real` respectively. The second line is made of two
statements that compute `circumference 5.3` and `circumference 2.5`.
There are two declarations and two statements in this code.


### Declaration phase

During the declaration phase, all declarations are stored in order in
the context, so that they appear before any declaration that was
already in the context.

For example, in the example above, the declaration phase would result
in having something like:

```xl
CONTEXT is
    pi is 3.14
    circumference Radius:real is 2 * pi * Radius
    CONTEXT0 // Previous context
```

The actual implementation is likely to store declarations is a more
easily accessible way, for example using some hashing or in a binary
tree. Whatever algorithm is used must preserve the order of
declarations, since the execution phase depends on it.

Note that since the declaration phase occurs first, all declarations
in the program will be visible during the execution phase.

### Execution phase

The execution phase then evaluates each statement in the order they
appear in the program. For each statement, the context is looked up
for matching declarations. There is a match if the shape of the tree
being evaluated matches the shape of the tree in the declaration.

For example, `circumference 5.3` will not match the declation of `pi`,
but it will match the declatation of `circumference Radius:real` since
the value `5.3` is indeed a real number.

When this happens, a new context is created with _bindings_ for the
formal parameters to the value passed as an argument in the
statement. This new context is called a _local context_ and will be
used to evaluat the definition associated with the declaration.

For example, the local context to evaluate the definition of
`circumference Radius:real` would be:

```xl
CONTEXT is
    Radius is 5.3
    // Earlier context begins here
    pi is 3.14
    circumference Radius:real is 2 * pi * Radius
    CONTEXT0 // Previous context
```

The execution then resumes by executing the definition, in that case
`2 * pi * Radius`, in the local context. The result of that execution
replaces the original statement.

The process can then resume with the next statement, `circumference 2.5`.
The result of evaluating the whole program will be the result of that
last statement.


### Expression evaluation

Executing the implementation for `circumference Radius:real` involves
the evaluation of expression `2 * pi * Radius`. This follows almost
exactly the same process as for `circumference 5.3`, with the
difference that now the same process needs to be repeated multiple
time.

If we apply the evaluation process with `2 * pi * Radius`, assuming
the operator declarations in the [standard library](../src/builtins.xl),
we will find a declaration for `X:real * Y:real as real` as well as a
declaratoin for `X:integer * Y:integer`. There is nothing that
directly matches something like `X * Y * Z`.

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
and not a `real`. It does not match `X:integer * Y:integer` because
`3.14` is a `real` and not an `integer`. However, there is a
declaration of an implicit conversion that looks like this:

```xl
X:integer   as real is ...
```

This means that the compiler is allowed to call that implicit
conversion to get a `real` from `integer` value `2`. This form is
evaluated in a context that looks like:

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

The result of that implicit conversion is `2.0`, and evaluation can
then resume with:

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

The result of the multiplication is a `real` with value `33.284`. This
is the result of evaluating `circumference 5.3`.


### Deferred evaluation


### Pattern matching



## Compiled representations

Code and any data can also have one or several _compiled forms_. The
compiled forms are generally very implementation-dependent, varying
with the machine you run the program on as well as with the compiler
technology being used.
