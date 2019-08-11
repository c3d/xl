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


### Execution context

The execution of XL programs is defined by describing the evolution of
a particular data structure called the _execution context_, which
stores all values accessible to the program at any given time.

That data structure is only intended to explain the effect of
evaluating the program. It is not intended to be a model of how things
are actually implemented. As a matter of fact, care was taken in the
design of XL to allow standard compilation and optimization techniques
to remain applicable, and to leave a lot of freedom regarding actual
evaluation techniques.

In the examples below, `CONTEXT` will be a pseudo-variable that
describes the execution context. Where necessary, `CONTEXT0` will
represent the previous context.


## Execution phases

Executing an XL program is the result of three phases,

1. A _parsing phase_ where program source text is converted to a parse tree,
2. A _declaration phase_, where all the declarations are stored in the
   context,
3. An _evaluation phase_, where statements other than declarations are
   processed in order.

The execution phases are designed so that in a very large number of
cases, it is at least conceptually possible to do both the parsing and
declaration phases ahead of time, and to generate machine code that can
perform the evaluation phase using only representations of code and
data optimized for the specific machine running the program.

In short, it should be possible to create an efficient ahead-of-time
compiler for XL. Work is currently in progress to build one. This kind
of optimizations are described in more details in the
[Compiled representations](#compiled-representations) section below.


### Parsing phase

The parsing phase reads source text and turns it into a parse tree
using operator spelling and precedence information given in the
[syntax file](../src/xl.syntax). This results in either a parse-time
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
   because `syntax` is designed to modify the syntax used during the
   parsing phase.

3. Processing `import` statements: Since imported modules can contain
   `syntax` statements, they must at least partially be processed
   during parsing.

4. Identifying words that switch to a
   [child syntax](HANDBOOK_1-syntax.md#comment-block-text-and-syntax-separators):
   symbols that activate a child syntax are recognized during parsing.
   This is the case for example with the `extern` name in the
   [default syntax](../src/xl.syntax#L62).

5. Identifying binary data: The word `bits` is treated specially
   during parsing, to generate parse tree nodes representing binary data.
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
* An infix `is` is called a _declaration_, and is processed during the
  [declaration phase](#declaration-phase).
* Anything else is called a _statement_ and is processed during the
  [execution phase](#execution-phase).

For example, consider the following code:

```xl
pi is 3.14
circumference 5.3
circumference Radius:real is 2 * pi * Radius
```

THe first and last line are each representing a declaration of `pi` and
`circumference Radius:real` respectively. The second line is made of one
statements that computes `circumference 5.3`. There are two
declarations and one statement in this code. The _result_ of executing
the code is the value of its last statement, in that case the value
computed by `circumference 5.3`.


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
in the program will be visible during the execution phase. In our
example, it is possible to use `circumference` before it has been
declared. This is necessary because declarations may refer to one
another in a circular way. Some other languages require "forward
declarations" in such cases, XL does not.


### Execution phase

The execution phase evaluates each statement in the order they appear
in the program. For each statement, the context is looked up for
matching declarations. There is a match if the shape of the tree being
evaluated matches the shape of the tree on the left of `is` in the
declaration.

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

The process can then resume with the next statement if there is
one. In our example, there isn't one, so the execution is complete.


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
