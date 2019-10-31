# History of XL

The status of the current XL compiler is [a bit messy](../#compiler-status).
There is a rationale to this madness. I attempt to give it here.

There is also a
[blog version](https://grenouillebouillie.wordpress.com/2017/12/10/from-ada-to-xl-in-25-years/)
if you prefer reading on the web (but it's not exactly identical).
In both cases, the article is a bit long, but it's worth understanding
how XL evolved, and why the XL compiler is still work in progress.


## It started as an experimental language

Initially, XL was called LX, "Langage experimental" in French, or as
you guessed it, an experimental language. Well, the very first
codename for it was "WASHB" (What Ada Should Have Been). But that did
not sound very nice. I started working on it in the early 1990s, after
a training period working on the Alsys Ada compiler.

What did I dislike about Ada? I never liked magic in a language. To
me, keywords demonstrate a weakness in the language, since they
indicated something that you could not build in the library using the
language itself. Ada had plenty of keywords and magic
constructs. Modern XL has no keyword whatsoever, and it's a Good Thing
(TM).

Let me elaborate a bit on some specific frustrations with Ada:

* Tasks in Ada were built-in language constructs. This was
  inflexible. Developers were already hitting limits of the Ada-83
  tasking model. My desire was to put any tasking facility in a
  library, while retaining an Ada-style syntax and semantics.

* Similarly, arrays were defined by the language. I wanted to build
  them (or, at least, describe their interface) using standard
  language features such as generics. Remember that this was years
  before the STL made it to C++, but I was thinking along similar
  lines. Use cases I had in mind included:

  * interfacing with languages that had different array layouts such
    as Fortran and C,
  * using an array-style interface to access on-disk records. Back
    then, `mmap` was unavailable on most platforms,
  * smart pointers that would also work with on-disk data structures,
  * text handling data structures (often called “strings”) that did
    not expose the underlying implementation (e.g. “pointer to char”
    or “character array”), …

* Ada text I/O facilities were uncomfortable. But at that time, there
  was no good choice. You had to pick your poison:

  * In Pascal, `WriteLn` could take as many arguments as you needed
    and was type safe, but it was a magic procedure, that you could
    not write yourself using the standard language features, nor
    extend or modify to suit your needs.

  * Ada I/O functions only took one argument at a time, which made
    writing the simplest I/O statement quite tedious relative to C or
    Pascal.

  * C’s `printf` statement had multiple arguments, but was neither
    type safe nor extensible, and the formatting string was horrid.

* I also did not like Ada pragmas, which I found too ad-hoc, with a
  verbose syntax. I saw pragmas as indicative that some kind of
  generic “language extension” facility was needed, although it took
  me a while to turn that idea into a reality.

I don't have much left of that era, but that first compiler was
relatively classical, generating 68K assembly language. I reached the
point where the compiler could correctly compile a "Hello World"
style program using an I/O library written in the language. I was
doing that work at home on Atari-ST class machines, but also gave
demos to my HP colleagues running XL code on VME 68030 boards.

From memory, some of the objectives of the language at the time included:

* Giving up on superfluous syntactic markers such as terminating semi-colon.

* Using generics to write standard library component such as arrays or
  I/O facilities.

* Making the compiler an integral part of the language, which led to...

* Having a normalised abstract syntax tree, and...

* Considering “pragmas” as a way to invoke compiler extensions.
  Pragmas in XL were written using the `{pragma}` notation, which
  would indirectly invoke some arbitrary code through a table.

Thus, via pragmas, the language became extensible. That led me to...


## LX, an extensible language

I wanted to have a relatively simple way to extend the language.
Hence, circa 1992, the project was renamed from "experimental" to
"extensible", and it has kept that name since then.

One example of thing I wanted to be able to do was to put tasking in a
library in a way that would "feel" similar to Ada tasking, with the
declaration of task objects, rendez-vous points that looked like
procedures with parameter passing, and so on.

I figured that my `{annotations}` would be a neat way to do this, if
only I made the parse tree public, in the sense that it would become a
public API. The idea was that putting `{annotation}` before a piece of
code would cause the compiler to pass the parse tree to whatever
function was associated with `annotation` in a table of annotation
processors. That table, when pointing to procedures written in XL,
would make writing new language extensions really easy. Or so I thought.

Ultimately, I would make it work. If you are curious, you can see the
[grand-child of that idea](../xl2/native/xl.semantics.instructions.xl#L629)
in the `translation` statements under `xl2/`. But that was way beyond
what I had initially envisionned, and the approach in the first XL
compiler did not quite work. I will explain why soon below.

The first experiment I ran with this, which became a staple of XL
since then, was the `{derivation}` annotation. It should have been
`{differentiation}`, but at that time, my English was quite crappy,
and in French, the word for "differentiation" is "derivation". The
idea is that if you prefixed some code, like a function, with a
`{derivation}` annotation, the parse tree for that function would be
passed to the `derivation` pragma handler, and that would replace
expressions that looked like differential expressions with their
expanded value. For example, `{derivation} d(X+sin(X))/dX` would
generate code that looked like `1 + cos(X)`.

If you are curious what this may look like, there are still
[tests in the XL2 test suite](../xl2/native/TESTS/07.Plugins)
 using a very similar feature and syntax.


## LX, meet Xroma

That initial development period for LX lasted between 1990, the year
of my training period at Alsys, and 1998, when I jointed the HP
California Language Lab in Cupertino (CLL). I moved to the United
States to work on the HP C++ compiler and, I expected, my own
programming language. That nice plan did not happen exactly as
planned, though…

One of the very first things I did after arriving in the US was to
translate the language name to English. So LX turned into XL. This was
a massive rename in my source code, but everything else remained the
same.

As soon as I joined the CLL, I started talking about my language and
the ideas within. One CLL engineer who immediately “got it” is Daveed
Vandevoorde. Daveed immediately understood what I was doing, in large
part because he was thinkering along the same lines. He pointed out
that my approach had a name: meta-programming, i.e. programs that deal
with programs. I was doing meta-programming without knowing about the
word, and I felt really stupid at the time, convinced that everybody
in the compilers community knew about that technique but me.

Daveed was very excited about my work, because he was himself working
on his own pet language named Xroma (pronounced like Chroma). At the
time, Xroma was, I believe, not as far along as XL, since Daveed had
not really worked on a compiler. However, it had annotations similar
to my pragmas, and some kind of public representation for the abstract
syntax tree as well.

Also, the Xroma name was quite Xool, along with all the puns we could
build using a capital-X pronounced as “K” (Xolor, Xameleon, Xode, ...)
or not (Xform, Xelerate, ...) As a side note, I later called
“Xmogrification” the VM context switch in
[HPVM](https://en.wikipedia.org/wiki/HP_Integrity_Virtual_Machines),
probably in part as a residual effect of the Xroma naming conventions.

In any case, Daveed and I joined forces. The combined effort was named
Xroma. I came up with the early version of the lightbulb logo still
currently used for XL, using FrameMaker drawing tools, of all
things. Daveed later did a nice 3D rendering of the same using the
Persistence of Vision ray tracer. I don't recall when the current logo
was redesigned.


## XL moves to the off-side rule

Another major visual change that happened around that time was
switching to the off-side rule, i.e. using indentation to mark the
syntax. Python, which made this approach popular, was at the time a
really young language (release 1.0 was in early 1994).

Alain Miniussi, who made a brief stint at the CLL, convinced me to
give up the Ada-style `begin` and `end` keywords, using an solid
argumentation that was more or less along the lines of “I like your
language, but there’s no way I will use a language with `begin` and
`end` ever again”. Those were the times where many had lived the
transition of Pascal to C, some still wondering how C won.

I was initially quite skeptical, and reluctantly tried an
indentation-based syntax on a fragment of the XL standard library. As
soon as I tried it, however, the benefits immediately became
apparent. It was totally consistent with a core tenet of concept
programming that I was in the process of developing (see below),
namely that the code should look like your concepts. Enforcing
indentation made sure that the code did look like what it meant.

It took some effort to convert existing code, but I’ve never looked
back since then. Based on the time when Alain Miniussi was at the CLL,
I believe this happened around 1999.


## XL gets a theoretical foundation: Concept programming

The discussions around our respective languages, including the
meta-programming egg-face moment, led me to solidify the theoretical
underpinning of what I was doing with XL. My ideas actually did go
quite a bit beyond mere meta-programming, which was really only a
technique being used, but not the end goal. I called my approach
_Concept Programming_. I tried to explain what it is about in
[this presentation](http://xlr.sourceforge.net/Concept%20Programming%20Presentation.pdf).

Concept programming deals with the way we transform concepts that
reside in our brain into code that resides in the computer. That
conversion is lossy, and concept programming explores various
techniques to limit the losses. It introduces pseudo-metrics inspired
by signal processing such as syntactic noise, semantic noise,
bandwidth and signal/noise ratio. These tools, as simple as they were,
powerfully demonstrated limitations of existing languages and
techniques.

Since then, Concept Programming has consistently guided what I am
doing with XL. Note that Concept Programming in the XL sense has
little do do with C++ concepts (although there may be a connection,
see blog referenced above for details).


## Mozart and Moka: Adding Java support to XL

At the time, Java was all the rage, and dealing with multiple
languages within a compiler was seen as a good idea. GCC being renamed
from "GNU C Compiler" to the "GNU Compiler Collection" is an example
of this trend.

So with Daveed, we had started working on what we called a "universal
program database", which was basically a way to store and access
program data independently of the language being used. In other words,
we were trying to create an API that would make it possible to
manipulate programs in a portable way, whether the program was written
in C, C++ or Java. That proved somewhat complicated in practice.

Worse, Daveed Vandevoord left the HP CLL to join the Edison Design
Group, where he's still working to this date. Xroma instantly lost
quite a bit of traction within the CLL. Also, Daveed wanted to keep the
Xroma name for his own experiments. So we agreed to rename "my" side
of the project as "Mozart". For various reasons, including a debate
regarding ownership of the XL code under California law, the project
was open-sourced. The [web site](http://mozart-dev.sourceforge.net)
still exists to this day, but is not quite functional since CVS
support was de-commissioned from SourceForge.

Part of the work was to define a complete description of the source
code that could be used for different language. Like for Xroma, we
stayed on bad puns and convoluted ideas for naming. In Mozart that
representation was called `Coda`. It included individual source
elements called `Notes` and the serialized representation was called a
`Tune`. Transformation on Notes, i.e. the operations of compiler
plug-ins, were done by `Performer` instances. A couple of years later,
I would realize that this made the code totally obfuscated for the
non-initiated, and I vowed to never make that mistake again.

Mozart included [Moka](http://mozart-dev.sourceforge.net/moka.html), a
Java to Java compiler using Mozart as its intermediate representation.
I published an [article in Dr Dobb's journal](http://www.drdobbs.com/jvm/what-is-moka/184404696),
 a popular developers journal at the time.

But my heart was never with Java anymore than with C++, as evidenced
by the much more extensive documentation about XL on the Mozart web
site. As a language, Java had very little interest for me. My
management at HP had no interest in supporting my pet language, and
that was one of the many reasons for me to leave the CLL to start
working on virtualization and initiate what would become HPVM.


## Innovations in 2000-vintage XL

By that time, XL was already quite far away from the original Ada,
even if it was still a statically typed, ahead-of-time language. Here
are some of the key features that went quite a bit beyond Ada:

* The syntax was quite clean, with very few unnecessary
  characters. There were no semi-colons at the end of statement, and
  parentheses were not necessary in function or procedure calls, for
  example. The off-side rule I talked about earlier allowed me to get
  rid of any `begin` or `end` keyword, without resorting to C-style curly
  braces to delimit blocks.

* Pragmas extended the language by invoking
  [arbitrary compiler plug-ins](http://mozart-dev.sourceforge.net/tools.html#pragma).
  I suspect that attributes in C++11 are distant (and less powerful)
  descendants of this kind of annotation, if only because their syntax
  matches my recollection of the annotation syntax in Xroma, and
  because Daveed has been a regular and innovative contributor to the
  C++ standard for two decades...

* [Expression reduction](http://mozart-dev.sourceforge.net/xl_style.html#expred)
  was a generalisation of operator overloading that works with
  expressions of any complexity, and could be used to name types. To
  this day, expression reduction still has no real equivalent in any
  other language that I know of, although expression templates can be
  used in C++ to achieve similar effect in a very convoluted and less
  powerful way. Expression templates will not allow you to <em>add</em>
  operators, for example. In other words, you can redefine what
  `X+Y*Z` means, but you cannot create `X in Y..Z` in C++.

* [True generic types](http://mozart-dev.sourceforge.net/xl_style.html#truegen)
   were a way to make generic programming much easier by declaring
   generic types that behaved like regular types. Validated generic
   types extended the idea by adding a validation to the type, and
   they also have no real equivalent in other languages that I am
   aware of, although C++ concepts bring a similar kind of validation
   to C++ templates.

* [Type-safe variable argument lists](http://mozart-dev.sourceforge.net/xl_style.html#vararg)
   made it possible to write type-safe variadic functions. They solved
   the `WriteLn` problem I referred to earlier, i.e. they made it
   possible to write a function in a library that behaved exactly like
   the Pascal `WriteLn`. I see them as a distant ancestor of variadic
   templates in C++11, although like for concepts, it is hard to tell
   if variadic templates are a later reinvention of the idea, or if
   something of my e-mails influenced members of the C++ committee.

* A powerful standard library was in the making. Not quite there yet,
  but the key foundations were there, and I felt it was mostly a
  matter of spending the time writing it.
  [My implementation of complex numbers](../xl2/native/library/xl.math.complex.xl),
  for example, was
  [70% faster than C++ on](http://mozart-dev.sourceforge.net/news.html#complex)
  simple examples, because it allowed everything to be in registers
  instead of memory.  There were a few things that I believe also date
  from that era, like getting rid of any trace of a main function,
  top-level statements being executed as in most scripting languages.


## XL0 and XL2: Reinventing the parse tree

One thing did not work well with Mozart, however, and it was the parse
tree representation. That representation, called `Notes`, was quite
complicated. It was some kind of object-oriented representation with many
classes. For example, there was a class for `IfThenElse` statements, a
`Declaration` class, and so on.

This was all very complicated and fragile, and made it extremely
difficult to write thin tools (i.e. compiler plug-ins acting on small
sections of code), in particular thin tools that respected subtle
semantic differences between languages. By 2003, I was really hitting
a wall with XL development, and that was mostly because I was also
trying to support the Java language which I did not like much.

One of the final nails in the Mozart coffin was a meeting with Alan
Kay, of Smalltalk fame, during an HP technical conference. Kay was an
HP Fellow at the time. I tried to show him how my language was solving
some of the issues he had talked about during his presentation. He did
not even bother looking. He simply asked: “_Does your language
self-compile?_“. When I answered that the compiler was written in
C++, Alan Kay replied that he was not interested.

That gave me a desire to consider a true bootstrap of XL. That meant
rewriting the compiler from scratch. But at that time, I had already
decided that the internal parse tree representation needed to be
changed. So that became my topic of interest.

The new implementation was called XL2, not just as a version number,
but because I was seeing things as a three-layer construction:

* `XL0` was just a very simple parse tree format with only eight node
  types. I sometimes refer to that level of XL as "_XML without the M_",
  i.e. an extensble language without markup.
* `XL1` was the core language evaluation rules, not taking any library
  into account.
* `XL2` was the full language, including its standard library. At the
  time, the goal was to reconstruct a language that would be as close
  as possible at the version of XL written using the Mozart framework.

This language is still available today, and while it’s not been
maintained in quite a while, it seems to still pass most of its test
suite. More importantly, the `XL0` format has remained essentially
unchanged since then.

The XL0 parse tree format is something that I believe makes XL
absolutely unique among high-level programming languages. It is
designed so that code that can look and feel like an Ada derivative
can be represented and manipulated in a very simple way, much like
Lisp lists are used to represent all Lisp programs. XL0, however, is
not some minor addition on top of S-expressions, but rather the
definition of an alternative of S-expressions designed to match the
way humans parse code.

The parse tree format consists of only eight node types, four leaf
node types (integer, real, text and symbol), four inner node types
(infix, prefix, postfix and block).

* `Integer` represents integer numbers, like `123` or
  `16#FFFF_FFFF`. As the latter example shows, the XL syntax includes
  support for based numbers and digit grouping.

* `Real` represents floating-point numbers, like `123.456` or
  `2#1.001_001#e-3`. Like for `Integer`, XL supports based
  floating-point numbers and digit grouping.

* `Text` represents textual constants like `"Hello"` or `'A'`.

* `Name` represents names like `ABC` or symbols like `<=`.

* `Infix` represents operations where a name is between two operands,
  like `A+B` or `A and B`.

* `Prefix` represents operations where an operator precedes its
  operand, like `sin X` or `-4`.

* `Postfix` represents operations where an operator follows its
  operand, like `3 km` or `5%`.

* `Block` represents operations where an operand is surrounded by two
  names, like `[A]`, `(3)` or `{write}`.

Individual program lines are seen as the leaves of an infix “newline”
operator. There are no keywords at all, the precedence of all
operators being given dynamically by a syntax file.

## Bootstrapping XL

The initial translator converts a simplified form of XL into C++ using
a very basic transcoding that involves practically no semantic
analysis. The limited XL2 acceptable as input for this translation
phase is only used in the bootstrap compiler. It already looks a bit
like the final XL2, but error checking and syntax analysis are
practically nonexistent.

The bootstrap compiler can then be used to translate the native XL
compiler. The native compiler performs much more extended semantic
checks, for example to deal with generics or to implement a true
module system. It emits code using a configurable "byte-code" that is
converted to a variety of runtime languages. For example, the C
bytecode file will generate a C program, turning the native compiler
into a transcoder from XL to C.

That native compiler can translate itself, which leads to a true
bootstrap where the actual compiler is written in XL, even if a C
compiler is still used for the final machine code generation. Using a
Java or Ada runtime, it would theoretically be possible to use a Java
or Ada compiler for final code generation.

The XL2 compiler advanced to the point where it could pass a fairly
large number of complex tests, including practically all the things
that I wanted to address in Ada:

* Pragmas implemented as [compiler plug-ins](../xl2/native/TESTS/07.Plugins).
* Expression reduction [generalising operator overloading](../xl2/native/TESTS/05.Expressions/multi-reduction.xl).
* An I/O library that was [as usable as in Pascal](xl2/native/TESTS/12.Library/hello_world.xl),
 but [written in the language](../xl2/native/library/xl.text_io.xl)
 and [user-extensible](../xl2/native/TESTS/12.Library/instantiation_of_complex.xl#L8).
* A language powerful enough to define its own
  [arrays](../xl2/native/library/xl.array.xs) or
  [pointers](../xl2/native/library/xl.pointer.address.xs),
  while keeping them exactly
  [as usable as built-in types](../xl2/native/TESTS/08.Aggregates/basic-array.xl).


## XL2 compiler plugins

XL2 has [full support for compiler plug-ins](../xl2/native/TESTS/07.Plugins),
 in a way similar to what had been done with Mozart. However, plug-ins
were much simpler to develop and maintain, since they had to deal with
a very simple parse tree structure.

For example, the
[differentiation plugin](../xl2/native/xl.plugin.differentiation.xl)
implements symbolic differentiation for common functions. It is tested
[here](../xl2/native/TESTS/07.Plugins/differentiation.xl#L33).
The generated code after applying the plugin would
[look like this](../xl2/native/TESTS/07.Plugins/differentiation_cmd_line.ref).
 The plugin itself is quite simple. It simply applies basic
mathematical rules on parse trees. For example, to perform symbolic
differentiation on multiplications, the code looks like this:

```xl
function Differentiate (expr : PT.tree; dv : text) return PT.tree is
    translate expr
        when ('X' * 'Y') then
            dX : PT.tree := Differentiate(X, dv)
            dY : PT.tree := Differentiate(Y, dv)
            return parse_tree('dX' * 'Y' + 'X' * 'dY')
```

Meta-programming became almost entirely transparent here. The
`translate` statement, itself provided by a compiler plug-in (see
below), matches the input tree against a number of shapes. When the
tree looks like `X*Y`, the code behind the matching `then` is
evaluated. That code reconstructs a new parse tree using the
`parse_tree` function.

Also notice the symmetric use of quotes in the `when` clause and in
the `parse_tree` function, in both cases to represent variables as
opposed to names in the parse tree. Writing `parse_tree(X)` generates a
parse tree with the name `X` in it, whereas `parse_tree('X')` generates a
parse tree from the `X` variable in the source code (which must be a
parse tree itself).


## XL2 internal use of plugins: the `translation` extension

The compiler uses this plug-in mechanism quite extensively internally.
A particularly important compiler extension provides the `translation`
and `translate` instructions. Both were used extensively to rewrite XL0
parse trees easily.

We saw above an example of `translate`, which translated a specific tree
given as input. It simply acted as a way to compare a parse tree
against a number of forms, evaluating the code corresponding to the
first match.

The `translation` declaration is even more interesting, in that it is
a non-local function declaration. All the `translation X` from all
modules are accumulated in a single `X` function. Functions
corresponding to `translation X` and `translation Y` will be used to
represent  distinct phases in the compiler, and can be used a regular
functions taking a tree as input and returning the modified tree.

This approach made it possible to distribute `translation
XLDeclaration` statements
[throughout the compiler](../xl2/native/xl.semantics.functions.xl#L371),
 dealing with
declaration of various entities, with matching `translation XLSemantics`
took care of
[the later semantics analysis phase](../xl2/native/xl.semantics.functions.xl#L557).

 Writing code this way made it quite easy to maintain the compiler
over time. It also showed how concept programming addressed what is
sometimes called
[aspect-oriented programming](https://en.wikipedia.org/wiki/Aspect-oriented_programming).
This was yet another proof of the "extensible" nature of the language.


## Switching to  dynamic code generation

One issue I had with the original XL2 approach is that it was strictly
a static compiler. The bytecode files made it possible to generate
practically any language as output. I considered generating LLVM
bitcode, but thought that it would be more interesting to use an XL0
input instead. One reason to do that was to be able to pass XL0 trees
around in memory without having to re-parse them. Hence XLR, the XL
runtime, was born. This happened around 2009.

For various reasons, I wanted XLR to be dynamic, and I wanted it to be
purely functional.  My motivations were:

* a long-time interest in functional languages.
* a desire to check that the XL0 representation could also comfortably
  represent a functional languages, as a proof of how general XL0 was.
* an intuition that sophisticated
  [type inference](https://en.wikipedia.org/wiki/Type_inference),
  Haskell-style, could make programs both shorter and more solid than
  the declarative type systems of Ada.

While exploring functional languages, I came across
[Pure](https://en.wikipedia.org/wiki/Pure_(programming_language)),
 and that was the second big inspiration for XL. Pure prompted me to
use LLVM as a final code generator, and to keep XLR extremely simple.


## Translating using only tree rewrites

I sometimes describe XLR as a language with a single operator, `is`,
which reads as _transforms into_. Thus, `X is 0` declares that `X` has
value `0`.

Until very recently, that operator was spelled using an arrow, as
`->`, which I thought expressed the _transforms into_ quite
well. Around 2018, I decided that this was unreadable for the novice,
and switched to using `is` as this _definition operator_. This `->`
operator is still what you will find for example on the
[Tao3D web site](http://tao3d.sourceforge.net).


This notation can be used to declare basic
operators:

```xl
x:integer - y:integer as integer    is opcode Sub
```

It makes a declaration of `writeln` even shorter than it was in XL2:

```xl
write x:text as boolean             is C xl_write_text
write x:integer as boolean          is C xl_write_integer
write x:real as boolean             is C xl_write_real
write x:character as boolean        is C xl_write_character
write A, B                          is write A; write B
writeln as boolean                  is C xl_write_cr
writeln X as boolean                is write X; writeln

```

More interestingly, even if-then-else can be described that way:

```xl
if true  then TrueBody else FalseBody   is TrueBody
if false then TrueBody else FalseBody   is FalseBody
if true  then TrueBody                  is TrueBody
if false then TrueBody                  is false
```

> **NOTE** the above code now requires a
> [metabox](HANDBOOK_2-evaluation#metabox) for true in the version of
> XL described in this document, i.e. `true` must be replaced with
> `[[true]]` in order to avoid being interpreted as a formal parameter.

Similarly for basic loops, provided your translation mechanism
implements tail recursion properly:

```xl
while Condition loop Body is
    if Condition then
        Body
    while Condition loop Body

until Condition loop Body is while not Condition loop Body

loop Body is Body; loop Body

for Var in Low..High loop Body is
    Var := Low
    while Var < High loop
        Body
        Var := Var + 1
```


> **NOTE** The fact that such structures can be implemented in the
> library does not mean that they have to. It is simply a proof that
> basic amenities can be constructed that way, and to provide a
> reference definition of the expected behaviour.


## Tao3D, interactive 3D graphics with XL

When I decided to leave HP, I thought that XLR was flexible enough to
be used as a dynamic document language. I quickly whipped together a
prototype using XLR to drive an OpenGL 3D rendering engine. That
proved quite interesting.

Over time, that prototype morphed into [Tao3D](http://tao3d.sf.net).
 As far as the XLR language itself is concerned, there wasn’t as much
evolution as previously. A few significant changes related to
usability popped up after actively using the language:

* Implicit conversions of integer to real were not in the original
  XLR, but it was quite annoying in practice when providing object
  coordinates.

* The XL version in Tao3D also became sensitive to spacing around
  operators, so as to distinguish `Write -A` from `X - Y`. Earlier
  versions forced you to use parentheses in the first case, as in
  `Write (-A)`, which was quite against the ideas of concept
  programming that your code must match your ideas.

* The more important change was the integration in the language of
  reactivity to transparently deal with events such as mouse, keyboard
  or time. Thus, the Tao3D language a fully functional-reactive
  language, without changing the core translation technology at all.

Precisely because the changes were so minor, Tao3D largely proved the
point that XL was really extensible. For example, a `slide` function
(that takes code as its second argument) makes it easy to describe a
great-looking bullet points slide:


```xl
import WhiteChristmasTheme
theme "WhiteChristmas"

slide "An example slide",
    * "Functional reactive programming is great"
    color_hsv mouse_x, 100%, 100%
    * "This text color changes with the mouse"
    color_hsv time * 20, 100%, 100%
    * "This text color changes with time"
```

and get an animated slide that looks like this:

![Tao3D slide](https://grenouillebouillie.files.wordpress.com/2017/12/tao3dtheme1.png?w=1424)

The same technique goes well beyond mere bullet points:

[![Tao3D animation](http://img.youtube.com/vi/4wTQcKvhReo/0.jpg)](http://www.youtube.com/watch?v=4wTQcKvhReo "Tao3D animation")

Tao3D developed a relatively large set of specialised modules, dealing
with things such as stereoscopy or lens flares. As a product, however,
it was never very successful, and Taodyne shut down in 2015, even if
the open-source version lives on.

Unfortunately, Tao3D was built on a relatively weak implementation of
XL, where the type system in particular was not well thought out
(it was really a hack that only supported parse tree types).
This made a few things really awkward. Notably, all values are passed
by reference, which was mostlyi an implementation hack to enable the
user-interface to "retrofit" values into the code when you move shapes
on the screen. Unfortunately, this made the language brittle, and
forced many modules to rely on poor hacks when updating values. To
make a long story short, `X := Y` in Tao3D is a joke, and I'm
rightfully ashamed of it.


## ELFE, distributed programming with XL

[ELFE](https://github.com/c3d/elfe) was another experiment with XL,
that took advantage of XL’s extensibility to explore yet another
application domain, namely distributed software, with an eye on the
Internet of Things. The idea was to take advantage of the existence of
the XL0 standard parse tree to communicate programs and data across
machines.

An ELFE program looks as as if it was running on a single machine, but
actively exchanges program segments and their associated data between
distant nodes:

```xl
invoke "pi2.local",
   every 1.1s,
        rasp1_temp ->
            ask "pi.local",
                temperature
        send_temps rasp1_temp, temperature

   send_temps T1:real, T2:real ->
       if abs(T1-T2) > 2.0 then
           reply
               show_temps T1, T2

show_temps T1:real, T2:real ->
    write "Temperature on pi is ", T1, " and on pi2 ", T2, ". "
    if T1>T2 then
        writeln "Pi is hotter by ", T1-T2, " degrees"
    else
        writeln "Pi2 is hotter by ", T2-T1, " degrees"
```


ELFE only adds a very small number of features to the standard XL:

* The `ask` statement sends a program, and returns the result of
  evaluating that program as if it has been evaluated locally. It
  works like a remote function call.

* An `invoke` statement sends a program to a remote node. It’s a “fire
  and forget” operation, but leaves a reply channel open while it’s
  executing.

* Finally, the `reply` statement allows a remote node to respond to
  whoever invoke‘d it, by evaluating one of the available functions in
  the caller’s context.

A few very simple [ELFE demos](../demo) illustrate these
remote-control capabilities. For example, it’s easy to [monitor
temperature](../demo/7-two-hops.xl) on two remote sensor nodes, and to
ask them to report if their temperatures differ by more than some
specified amount. The code is very short and looks like this:

```xl
invoke "pi2.local",
   every 1.1s,
        rasp1_temp ->
            ask "pi.local",
                temperature
        send_temps rasp1_temp, temperature

   send_temps T1:real, T2:real ->
       if abs(T1-T2) > 2.0 then
           reply
               show_temps T1, T2

show_temps T1:real, T2:real ->
    write "Temperature on pi is ", T1, " and on pi2 ", T2, ". "
    if T1>T2 then
        writeln "Pi is hotter by ", T1-T2, " degrees"
    else
        writeln "Pi2 is hotter by ", T2-T1, " degrees"
```

ELFE was designed to run with a small memory footprint, so it provides
a complete interpreter that does not require any LLVM. As the names in
the example above suggest, it was tested on Raspberry Pi. On the other
hand, the LLVM support in that “branch” of the XL family tree fell
into a bad state of disrepair.


## XL gets a type system

Until that point, XL lacked a real type system. What was there was
mostly quick-and-dirty solutions for the most basic type checks. Over
a Christmas vacation, I spent quite a bit of time thinking about what
a good type system would be for XL. I was notably stumped by what the
type of `if-then-else` statements should be.

The illumination came when I realized that I was blocked in my
thinking by the false constraint that each value had to have a single
type. Instead, the type system that seems natural in XL is that a type
indicates the shape of a parse tree. For example, `integer` is the
type of integer constants in the code, `real` the type of real constants,
and `type(X+Y)` would be the type of all additions.

Obviously, that means that in XL, a value can belong to multiple
types. `2+3*5` belongs to `type(X+Y)`, to `type(X:integer+Y:integer)`
or to `infix`. This makes the XL type system extremely powerful. For
example. a type for even numbers is `type(X:integer when X mod 2 = 0)`.

ELFE also gave me a chance to implement a relatively crude version of
this idea and validate that it's basically sane. Bringing that idea to
the optimizing compiler was an entirely different affair, though, and
is still ongoing.


## The LLVM catastrophy

For a while, there were multiple subtly distinct variants of XL which
all shared the same XL0, but had very different run-time constraints.

* Tao3D had the most advanced library, and a lot of code written for
  it. But that code often depends on undesirable behaviours in the
  language, such as implicit by reference argument passing.

* ELFE had the most advanced type system of all XLR variants, being
  able to perform overloading based on the shape of parse trees, and
  having a rather complete set of control structures implemented in
  the library. It also has an interesting modular structure, and a
  rather full-featured interpreter.

* XLR has the most advanced type inference system, allowing it to
  produce machine-level instructions for simple cases to get
  performance that was on a par with C. Unfortunately, due to lack of
  time, it fell behind with respect to LLVM support, LLVM not being
  particularly careful about release-to-release source
  compatibility. And the type inference was never solid enough to
  retrofit it in Tao3D, which still uses a much simpler code
  generation.

* XL2 was the only self-compiling variant of XL ever written, and
  still had by far the most sophisticated handling of generics and
  most advanced library. But it  has been left aside for a few
  years. As an imperative language, it is more verbose and feels
  heavier to program. Yet it is not obsolete, as the discussion above
  demonstrates. It’s type system, with its support for generics or
  validation, is much more robust than whatever was ever implemented
  in all XLR variants. It would need quite a bit of love to make it
  really usable, for example improving the standard library and
  actually connect XLR as a bytecode back-end as initially envisioned.

In addition to this divergence, another problem arose externally to
the XL project. The LLVM library, while immensely useful, proved a
nightmare to use, because they purposely don't care about source cdoe
compatibility between releases. XLR was initially coded against LLVM
2.5, and the majority of Tao3D development occured in the LLVM 2.7
time frame.

Around release 3.5, LLVM started switching to a completely different
code generation model. Being able to support that new model proved
extremely challenging, in particular for something as complex as
Tao3D. The problem is not unique to Tao3D: the LLVM pipe in the Mesa
project has similar issues. But in Tao3D, it was made much worse
precisely because Tao3D uses both OpenGL and XL, and the Mesa
implementation of OpenGL commonly used on Linux also uses LLVM. If the
variants of LLVM used by the XL runtime and by OpenGL don't match,
mysterious crashes are almost guaranteed.

From 2015 to 2018, all development of XL and Tao3D was practically
stuck on this problem. It did not help that my job during that time
was especially challenging time-wise. In practice, the development of
Tao3D and XLR was put on hold for a while.


## Repairing and reconverging

A project that lasted several months, called `bigmerge` allowed me to
repair a lot of the issues:

* The XL2 compiler was brought back into the main tree
* The ELFE interpreter was merged with the main tree, and its modular
  approach (designed to allow the use of XL as an extension language)
  was incorporated in XL. As a result, ELFE is dead, but it lives on
  in the main XL tree. XL was also turned into a library, with a very
  small front-end calling that library to evaluate the code.
* The switch from `->` to `is` as the definition operator was implemented.
* The LLVM "Compatibility Restoration Adaptive Protocol" (LLVM-CRAP)
  component of XL was completely redesigned, giving up pre-3.5
  versions of LLVM, but supporting all the recent ones (from 3.7 to
  9.0).
* The Tao3D branch of the compiler was forward-ported to this updated
  compiler, under the name `FastCompiler`. That work is not complete,
  howver, because some of the changes required by the two previous
  steps are incompatible with the way Tao3D was interfacing with XL.

This is the current state of the XL tree you are looking at. Not
pretty, but still much better than two years ago.


## Language redefinition

During all that time, the language definition had been a very vaguely
defined [TeXMacs document](XL_Reference_Manual.pdf). This document had
fallen somewhat behind with respect to the actual language
implementation or design. Notably, the type system was quickly
retrofitted in the document. Also, the TexMacs document was
monolithic, and not easy to convert to a web format.

So after a while, I decided to
[rewrite the documentation in markdown](HANDBOOK.md). This led me to
crystalize decisions about a few things that have annoyed me in the previous
definition, in particular:

* The ambiguity about formal parameters in patterns, exhibited by the
  definition of `if-then-else`. The XL language had long defined
  `if-then-else` as follows:
  ```xl
  if true  then TrueClause      is TrueClause
  if false then TrueClause      is false
  ```
  There is an obvious problem in that definition. Why should `true` be
  treated like a constant while `TrueClause` a formal parameter?

  The solution proposed so far so far was that if a name already
  existed in the context, then we were talking about this name. In
  other words, `true` was supposed to be defined elsewhere and not
  `TrueClause`.

  This also dealt with patterns such as `A - A is 0`. However, the
  cost was very high. In particular, a formal parameter name could not
  be any name used in the enclosing context, which was a true nuisance
  in practice.

  More recently, I came across another problem, which was how to
  properly insert a computed value like the square root of two in a
  pattern? I came up with an idea inspired parameters in `translate`
  statements in XL2, which I called a "metabox". The notation `[[X]]`
  in a pattern will evaluate `X`. To match the square root of 2, you
  would insert the metabox `[[sqrt 2]]`. To match `true` instead of
  defining a name `true`, you would insert `[[true]]` instead of
  `true`.

  Downside: fix all the places in the documentation that had it backwards.

* The addition of opaque binary data in parse trees, for example to
  put an image from a PNG file in an XL program. I had long been
  thinking about a syntax like `binary "image.png".

* Adding a `lambda` syntax for anonymous functions. Earlier versions
  of XL would use a catch-all pattern like `(X is X + 1)` to define a
  lambda function, so that `(X is X + 1) 3` would be `4`. That pattern
  was only recognized in some specific contexts, and in other
  contexts, this would be a definition of a variable named `X`. It is
  now mandatory to add `lambda` for a catch-all pattern, as in
  `lambda X is X + 1`, but then this works in any context.



## Future work

The work that remains to make XL usable again (in the sense of being
as stable as it was for Tao3D in the 2010-2015 period) includes:

* Complete the work on an Haskell-style type inference system, in
  order to make the "O3" compiler work well.

* Repair the Tao3D interface in order to be able to run Tao3D again
  with modern LLVM and OpenGL variants.

* Re-connect the XL2 front-end for those who prefer an imperative programming
  style, ideally connecting it to XLR as a runtime.

* Sufficient library-level support to make the language usable for
  real projects.

* Bootstrapping XLR as a native compiler, to validate that the
  XLR-level language is good enough for a compiler. Some of the
  preparatory work for this is happening in the `native` directory.

* Implement a Rust-style borrow checker, ideally entirely from the
  library, and see if it makes it possible to get rid of the garbage
  collector. That would be especially nice for Tao3D, where GC pause,
  while generally quite small, are annoying.

* Some reworking of XL0, notably to make it easier to add terminal
  node types. An example of use case is version numnbers like `1.0.1`,
  which are hard to deal with currently. The distinction between
  `Integer` and `Real` is indispensable for evaluation, but it may not
  be indispensable at parse time.

* Replace blocks with arrays. Currently, blocks without a content,
  such as `( )` or `{ }`, have a blank name inside, which I find ugly. It
  would make more sense to consider them as arrays with zero length.
  Furthermore, blocks are often used to hold sequences, for example
  sequences of instructions. It would be easier to deal with a block
  containing a sequence of instructions than with the current block
  containing an instruction or a chain of infix nodes.

* Adding a “binary object” node type, which could be used to either
  describe data or load it from files. I have been considering a syntax
  like:

  ```xl
  bits 16#0001_0002_0003_0004_0005_0006_0007_0008_0009
  bits "image.jpg"
  ```

It is unclear if I will be able to do that while at the same time
working on my job at Red Hat and on various other little projects
such as `make-it-quick` or `recorder` (which are themselves off-shoots
of XL development).
