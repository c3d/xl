# XL syntax

For programmers familiar with other programming language, the syntax
of XL may not seem very innovative at first, and that is
intentional. Most programmers should be able to read and write correct
XL code in a matter of minutes.

The first noticable thing is a disturbing lack of all these nice
semi-random punctuation characters that have decorated programs since
the dawn of computing and make most source code look like an ornate
form of line noise to the uninitiated. Where are all the parentheses
gone? Why this horrible lack of curly braces? How can you make sense
of a program without a semi-colon to terminate (or separate) statements?

In reality, however, the difference between XL syntax and earlier
programming languages is much more than skin deep. The syntax of XL is
actually one of its most unique characteristics. It is essential to
understand both the philosophy and implementation of the language.


## Homoiconic representation of programs

XL is a [homoiconic language](https://en.wikipedia.org/wiki/Homoiconicity),
meaning that all XL programs are data and conversely. This makes it
particularly easy for programs to manipulate programs, an approach
sometimes referred to as _metaprogramming_.  Metaprogramming is the
foundation upon which the touted extensibility of XL is built.


### Why Lisp remains strong to this day

In that respect, XL is very much inspired by one of the earliest
and most enduring high-level programming language,
[Lisp](https://en.wikipedia.org/wiki/Lisp_(programming_language))
The earliest implementations of Lisp date back to 1958, yet that
language remains surprisingly modern and flourishing today, unlike
languages of that same era like [Cobol](https://en.wikipedia.org/wiki/COBOL)
or [Fortran](https://en.wikipedia.org/wiki/Fortran).

One reason for Lisp's endurance is the meta-programming capabilities
deriving from homoiconicity. If you want to add a feature to Lisp, all
you need is to write a program that translates Lisp programs with the
new feature into previous-generation Lisp programs. This kind of
capability made it much easier to add object-oriented programming
[to Lisp](https://en.wikipedia.org/wiki/Common_Lisp_Object_System)
than to languages like C. Neither [C++](https://en.wikipedia.org/wiki/C++)
nor [Objective C](https://en.wikipedia.org/wiki/Objective-C) could
have possibly be implemented as a regulard C library.

Despite its strengths, Lisp remains confined to specific markets, in
large part because to most programmers, the language remains
surprisingly alien to this day, even garnering such infamous nicknames
as "_Lots of Insipid and Stupid Parentheses_". As seen from a
[concept programming](HISTORY.md#xl-gets-a-theoretical-foundation-concept-programming)
point of view, the underlying problem is that the Lisp syntax departs
from the usual notations as used by human beings. For example, adding
1 and 2 is written `1+2` in XL, like in most programming languages,
but `(+ 1 2)` in Lisp.

XL addresses this problem by putting human accessibility first. XL is
quite a bit more than Lisp with a new fancy and programmer-friendly
syntax, however.


### The XL parse tree

The XL syntax is  much _simpler_ than that of languages such
as C, and arguably not much more complicated than that of Lisp.
A key to keeping things simple is that the XL syntax is _dynamic_.
Available operators and their precedence are _configured_ primarily
through a [syntax file](../src/xl.syntax). There are no keywords or
special operators in XL.

XL programs can be represented with a very simple tree structure,
called a _parse tree_. The XL parse tree contains four leaf node types
(integer, real, text and name), and four inner node types (infix,
prefix, postfix and block).

Leaf nodes contain values that are atomic as far as XL is concerned:

1. `integer` nodes represent integer values like `1234`, `2#1001` or
   `16#FFFE_FFFF`.

2. `real` nodes represent floating-point values like `1.234`,
   `1.5e-10` or `2#1.0001_0001#e24`.

3. `text` nodes represent text values like `"Hello world"` or `'A'`.

4. `name` node represent names like `ABC_DEF` or symbols like `<=>`.


Inner nodes contains combinations of other XL nodes:

1. `infix` nodes represent two operands separated by a name or symbol,
   like `A+B` or `X and Y`. Infix nodes with a "new line" name are
   used for separate program lines.

2. `prefix` nodes represent two nodes where the operand follows the
   operator, like `+A` or `sin X`.

3. `postfix` nodes represent two nodes where the operator follows the
   operand, like `3%` or `45km`.

4. `block` nodes represent a node surrounded by two delimiters, like
   `[a]`, `(a)`, `{a}`. Blocks are also used to represent indentation.


> *NOTE* This list of node types is what the current implementations
> of XL offer. Some changes may happen, notably:
> * Adding a "binary object" node type, which could be used to store
>   binary data in the program. A possible syntax would be to prefix
>   `bits` before a large integer value or file name:
>   ```xl
>       bits 16#FF_00_FF_00_FF_FF_00_FF_00
>       bits "image.png"
>   ```
> * Finding a more efficient representation for large sequences of
>   items. So far, attempts at finding such a representation came with
>   an unacceptable cost, notably with respect to the generated code.

For example, let's consider the following code:

```xl
if X < 0 then
   write_line "The value of ", X, " is negative"
   X := -X
```

Assuming that this program is stored in a file called `program.xl`,
the XL parse tree for this program can be obtained by using the
following command:

```
% xl -parse program.xl -style debug -show
(infixthen
 (prefix
  if
  (infix<
   X
   0))
 (block indent
  (infix CR
   (prefix
    write_line
    (infix,
     "The value of "
     (infix,
      X
      " is negative"
     )))
   (infix:=
    X
    (prefix
     -
     X
    )))))
```

All of XL is built on this very simple data structure. Some choices,
like having distinct `integer` and `real` node, were guided primarily
by considerations beyond syntax, for example the need to be able to
precisely define [program evaluation](XL-2-evaluation.md) or to
represent distinct machine types.

> *NOTE* Empty blocks are represented as a block with an "empty name" as
> a child. This is not very satisfactory. Alternatives such as
> representing blocks as possibly empty sequences of items have proven
> even more complicated, since the representation of [A,B,C] becomes
> ambiguous, and possibly more difficult to process in a generic way.


## Syntax for XL nodes

The leaf nodes in XL all have a uniquely identifable syntax.

### Number syntax

Numbers begin with a digit, i.e. one of `0123456789`.

A single underscore `_` character can be used to separate digits, as
in `1_000_000`. The following are not valid XL numbers: `_1` (leading
underscore), `2_` (trailing underscore), `3__0` (two underscores).

Based numbers can be written by following the base with the `#` sign.
The base can be any decimal value between 2 and 36, or 64.

* For bases between 11 and 36, letters `A` through `Z`
  or `a` through `z` represent digit values larger than 10. For
  example, `A` is 10, `F` is 15, `z` is 35.
* For base 64, [Base64](https://en.wikipedia.org/wiki/Base64)
  encoding is used, and case matters.

There is an implementation-dependent limit for the maximum `integer`
value. This limit cannot be less than the maximum value for a
2-complement 64-bit signed integer.

For real numbers, a dot `.` is used as decimal separator, and must
separate digits. For example, `0.2` and `2.0` are valid but, unlike in
C, `.2` and`2.` are not real numbers but a prefix and postifix dot
respectively. Also note that there is a frequent notation for ranges
using two dots, so `2..3` is an infix `..` with `2` and `3` as
operands, and usually indicates the range between 2 and 3.

Both integer and real numbers can contain an exponent, specified by
the letter `e` or `E`. If the exponent is negative, then the number is
parsed as a real number. Therefore, `1e3` is integer value 1000, but
`1e-3` is real value `0.001`. The exponent is always given in base 10,
and it indicates an exponentiation in the given base, so that `2#1e8`
is decimal value 256. For based numbers, the exponent may be precede
by a `#` sign, which is mandatory if `e` or `E` are valid digits in
the base, as in `16#FF#e2` which represents decimal value 65280.

If a sign precedes a number, like `+3` or `-5.3`, it is parsed by the
compiler as a prefix `-` and not as part of the number. It is
possible, however, for an `integer` or `real` node to contain negative
values as a result of program evaluation.

The various syntactic possibilities to give a number are only for
convenience, and are all strictly equivalent as far as program
execution is concerned. In other words, a program may not behave
differently if a constant is given as `16#FF_FF` or as `65535`.

> *NOTE* One unsatisfactory aspect of XL number syntax is that it does
> not offer an obvious path to correctly represent "semantic" version
> numbers in the code.  For example, a notation like `2.3.1` will
> parse as an infix `.` between real number `2.3` and integer `1`,
> making it indistinguishable from `2.30.1`.


### Name and symbol syntax

Names in XL begin with an ASCII letter or Unicode symbol, followed by
ASCII letters, Unicode symbols or digits. For example, `MyName` and
`A22` are valid XL names.

A single underscore `_` can be used to separate two valid characters
in a name. Therefore, `A_2` is a valid XL name, but `A__2` and `_A`
are not.

The choice to include Unicode symbols makes the assumption that most
Unicode symbols can be interpreted as a letter in at least one
language. In many cases, it works. For example, `𝝿_2` or `étalon` are
valid XL names.

> *NOTE* Unfortunately, the XL parser is not smart enough to
> recognize what humans would see as symbols in the Unicode space, so
> that `⇒A2` is presently a valid name in XL. It is highty recommended
> to not depend on this behavior, which is considered a bug.

Case and delimiters are not significant in XL, so that `JOE_DALTON`
and `JoeDalton` are treated identically.

> *NOTE* For historical reasons, the current implementations are quite
> lacking in that respect.

Symbols begin with one of the ASCII punctuation characters:

```
    ! # $ % & ( ) * + , - . / : ; < = > ? @ [ \ ] ^ _ ` { | } ~
```

Symbols longer than one character must be specified in the XL syntax
file. For example, the XL syntax file defines a `<=` operator, but no
`<=>` operator. Consequently, the sequence `1 <=> 2` will be parsed as
`(1 <= (> 2))`. It is of course possible to add the `<=>` operator in
the syntax file if one wants to add the "spaceship operator" to the
language.

Except for the characters they contain, names and symbols are
otherwise treated interchangeably by XL.


### Text syntax

Text in XL is delimited with a single or double quote, `'` or `"`, and
can contain any printable character. Foe example, `"Hello World"` or
`'ABC'` are valid text in XL.

Additionally, the XL syntax file can specify delimiters for "long"
text. Long text can include line-terminating characters, and only
terminates when the matching delimiter is reached. By default, `<<`
and `>>` are long-text delimiters, so that the following is valid
text:

```xl
MyLongText is <<
   This is a multi-line text
   that contains several lines
>>
```

Additional separators can be configured, and can be used to define
specific types of text. For example, a program that often has to
manipulate HTML data could allow `HTML` and `END_HTML` as delimiters,
so that you could write:

```xl
MyHTML is HTML
    <p>This is some HTML text here</p>
END_HTML
```


### Indentation and off-side rule

Indentation in XL is significant, and is parsed as a special kind of
block.

Individual program line are parsed as infix nodes with the first line
as the left operand, and the second line as the right operand.

Consequently, the two `loop` instructions below have exactly the same
structure, except for the block delimiters (curly braces or
indentation) and for the line-separating infix names (semi-colon or
line terminator):

```xl
loop { Eat; Pray; Love }
loop
    Eat
    Pray
    Love
```


Indentation must use the same indentation character within a single
file, either tab or space. In other words, either your whole file is
indented with tabs, or it is indented with spaces, but it is a syntax
error to mix both.

Indentation within a block must be consistent. For example, the
following code will cause a syntax error because of the incorrect
indentation of `Pray`:

```xl
loop
    Eat
   Pray
    Love
```


## Making the syntax easy for humans

XL contains a couple of tweaks designed specifically to make code
easier to read or write by humans. When the human logic is subtle, so
is the XL compiler parsing...


### Tweak #1: expression vs. statement

This first tweak is intended to put in XL an implicit grammatical
grouping that humans apparently do. Consider for example the following:

```xl
write sin X, cos Y
```

Most human beings parse this as `write (sin(X),cos(Y))`, i.e. we call
`write` with two values resulting from evaluating `sin X` and
`cos Y`.

This is, however, not entirely logical. If `write` takes
comma-separated arguments, why wouldn't `sin` also take
comma-separated arguments? In other words, why doesn't this parse as
`write_line(sin(X, cos(Y))`?

This shows that humans have a notion of *expressions*
vs. *statements*. Expressions such as `sin X` have higher priority
than commas and require parentheses if you want multiple arguments. By
contrast, statements such as `write_line` have lower priority, and
will take comma-separated argument lists. An indent or `{ }` block
begins a statement, whereas parentheses `()` or square brackets `[]`
begin an expression.

There are rare cases where the default rule will not achieve the
desired objective, and you will need additional parentheses.


### Tweak #2: infix vs. prefix

Another special rule is that XL will use the presence of a space on
only one side of an operator to disambiguate between an infix or a
prefix. For example:

```xl
write -A    // write (-A)
B - A       // (B - A)
```

-------------------------------------------------------------------------------

Next: [Program evaluation](XL-2-evaluation.md)
Previous: [Introduction](XL-0-introduction.md)
