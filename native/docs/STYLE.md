# XL coding style

XL is a very simple, yet powerful language.

Since most of it is defined by the XL library, readability relies
heavily on a number of conventions.

## Capitalization

The core XL language is insensitive to capitalization and ignores
underscores. In other words, `JoeDalton`, `JOE_DALTON` and
`joe_dalton` all represent the same entity, much as they would in
real-life.

However, the XL code consistently follows a convention that helps
identify the purpose or intent of a given symbol.

1. Symbols in `lowercase` normally denote data types, or functions
   that return a data type, for example `integer` or `address`. By
   extension, prefix operators (functions) that either take or return
   a type will often also be spelled in lowercase, for example
   `new integer`, `like datagram`, `byte_size integer`.

2. Symbols in `UPPERCASE` normally denote modules or other
   compile-time constants. For example, `INTEGER` is the module
   defining among other things the `integer` type.

   * Internally, modules are nothing else than constant records

   * Note that the same name being often used for a module and types
     defined in the module. This works despite XL being insensitive to
     case because the compiler uses the context to understand if you
     talk about the module or the type.

3. Symbols in `CamelCase` normally denote either prefix forms
   (functions) or variables. For example, `CircleRadius := 5.3` uses
   a variable named `CircleRadius`. A function computing the radius
   from a circle could be spelled as `Radius C:circle as radius`.

4. Infix and postfix operators are usually spelled in lower case. For
   example, a bitwise and for unsigned values would be written as
   `X:unsigned and Y:unsigned as unsigned`.

5. Most structural words are spelled in lower case. For example, words
   such as `if`, `then`, `else`, `while`, `loop` are generally spelled
   in lowercase.

6. When there is a specific capitalization rule, notably for acronyms,
   that capitalization should prevail. For example, XL spells `ASCII`
   because this is an acronym. Similarly, there is a meaningful
   difference (for humans) between `Next` and `NeXT`.


## Use of block operators

In XL, there are by default four kinds of block operators:

1. Parentheses `( )`
2. Curly braces `{ }`
3. Angle brackets `[ ]`
4. Indentation blocks, marked by the off-side rule

Blocks are generally used to group items. Other than syntactic
differences, blocks are essentially equivalent. There is no deep
semantic distinction between `(A+B)` and `[A+B]` for example.
In a prefix usage, there is little semantic difference between
`A B`, `A(B)`, `A[B]` or `A{B}`, or even `[A]{B}`.

There are syntactic differences between blocks, however, most notably
different priorities leading to different parsing. Indentation and
curly-braces `{ }` blocks have low priority, and are normally used to
contain statements. Parentheses and square brackets `[ ]` have higher
priorities and are normally used to contain expressions.

For example `(sin X + Y)` will be parsed as a block containing
the *expression* `sin X + Y`, which is an infix `+` operator separating
a prefix `sin X` and a name `Y`.

By contrast, `{write X + Y}`, while it has the same sequence of
tokens, will be parsed as a block containing the *statement*
`write X + Y`, and this time, because it is parsed as a statement,
it will be interpreted as meaning a `write` prefix with `X + Y` as its
argument.

Only parsing is impacted. There is no semantic or type difference.

Conventionally, blocks are used as follows:

1. Parentheses are used to group expressions, for example to change
   the order of evaluation for expressions, as in `(2+3)*5`, which
   computes `25`, as opposed to `2+3*5` which computes `17`.

2. Curly braces are used to group sequences of statements when
   indentation would not be convenient, notably to put statements on a
   single line, as in `loop { Eat; Pray; Love }`

3. Angle brackets are generally used for indexing or generic
   parameters. For example, the `N`-th element in array `A` would
   generally be written as `A[N]`, although `A N` or `A(N)` would work
   just as well. Similarly, the `RANGE` module is generic, depending
   on a type argument. This is written `RANGE[value:type]`, although
   it could just as well be written `RANGE value:type`.

4. Indentation blocks are normally used for large sections of code on
   multiple lines. The previous loop could also be written as
   ```
   loop
       Eat
       Pray
       Love
   ```
