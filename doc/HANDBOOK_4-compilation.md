# Compiled XL

## Compiled representations

Code and any data can also have one or several _compiled forms_. The
compiled forms are generally very implementation-dependent, varying
with the machine you run the program on as well as with the compiler
technology being used.


Types also determine properties such as the size and binary representation
of values. For example, on most machines, `integer` will be
represented as a 64-bit 2-complement binary value, and `real` using
the IEEE-754 64-bit representation.


## Data


## Lifetime


## Closures


## Caller lookup

Whenever code contains `caller.X`, an implicit `X` argument is added
to the enclosing function, wich needs to be passed by all callers.

For example, the following code:

```xl
example X:integer as integer is X + caller.Base
Base : integer := 25
example 3
```

is transformed into:

```xl
example X:integer, Base:integer as integer is X + Base
Base : integer := 25
example 3, Base
```
