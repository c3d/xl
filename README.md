The XL programming language
===========================

Xl is a super-flexible language based entirely on parse tree rewrites.

In XL, you don't define functions or variables. You define "rewrites" of the code.
This is done with the rewrite operator `->` which reads as "transforms into".

For example, `X -> 0` means that you should transform `X` into `0`.
Below is the definition of a factorial in XL:

```
0! -> 1
N! -> N * (N-1)!
```

Internally, XL parses source input into a parse tree containing only 8 node types.

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

XL is used as the basis for Tao, a functional, reactive, dynamic document description language for 3D animations.
See http://www.taodyne.com for more information on Tao.
