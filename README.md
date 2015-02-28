The XL programming language
===========================

Xl is a super-flexible language based entirely on parse tree rewrites.

In XL, you don't define functions or variables. You define "rewrites"
of the code. This is done with the rewrite operator `->` which reads
as "transforms into".

For example, `X -> 0` means that you should transform `X` into `0`.
Below is the definition of a factorial in XL:

```
0! -> 1
N! -> N * (N-1)!
```

Internally, XL parses source input into a parse tree containing only 8
node types.

Four node types represent leafs in the parse tree:

* Integer nodes represents integral numbers such as `123`.
* Real nodes represent floating-point numbers such as `3.1415`.
* Text nodes represent textual data such as `"ABC"`
* Name nodes represent names and symbols such as `ABC` or `<<`

Four other node types represent inner nodes in the parse tree:

* Infix nodes represent notations such as `A+B` or `A and B`
* Prefix nodes represent notations such as `+A` or `sin A`
* Postfix nodes represent notations such as `3%` or `5 km`
* Block nodes represent notations such as `(A)`, `[A]` or `{A}`.

Block nodes are used to represent indentation in the source code.
Infix nodes are used to represent line breaks in the source code.

XL is used as the basis for Tao, a functional, reactive, dynamic
document description language for 3D animations.
See http://www.taodyne.com for more information on Tao.
In action: http://www.youtube.com/embed/Fvi29XAo4SI.


The xl-symbols branch
=====================

This branch is a major refactoring of the compiler with the following
objectives:

* Store the symbol tables using XL data structures.
* Generate better code, notably using type inference where applicable
* Generally speaking, cleanup all the crud that accumulated over time

The compiler has already been significantly reduced compared to
'master'. It currently runs at less than 25000 lines of code. A minor
issue is that it does not work at all. Yet.

The symbol table uses a data structure that can almost be evaluated to
what it represents. Specifically:

* Scopes are represented by `Prefix` nodes. If scope `A` is inside
  scope `B`, the representation is `B A`, e.g. `(X->1) (Y->2; Z->3)`
* Declarations are represented by `Infix` nodes, where a `\n` infix
  represents a declaration, its left node being something like `A->B`,
  and its right node being an `;` infix, where the left and right node
  are hash-balanced children declarations.



XL2 vs. XLR
===========

The current work happens in the XLR directory, which originally was
the runtime subset of XL. It was supposed to be simpler and more
lightweight variant of the language, used only as a back-end language
for dynamic code generation, basically a way to connect the front-end
with LLVM. It turns out that even very simple and lightweight makes
for a quite useful language.

So at this point in time, the "higher-level" version, XL2, has been
put largely on hold. It may be revived one day for those who like a
more Algolish/Adaesque syntax. It did offer a number of interesting
features, notably the notion of "true generic type". But at this
point, I believe that none of the interesting features in XL2 is hard
to replicate dynamically in XLR. Time will tell.
