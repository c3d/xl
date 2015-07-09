# ELIoT
### Extensible Language for the Internet of Things

ELIoT is a very simple and small programming language specifcally
designed to facilitate the configuration and control of swarms of
small devices such as sensors or actuators. It can also be used as a
powerful, remotely-accessible extension language for larger
applications.


## Example: Measuring temperature

Consider a sensor named `sensor.corp.net` running ELIoT and featuring
a temperature measurement through a `temperature` function.

ELIoT lets you evaluate programs on this sensor remotely to do all
kinds of interesting temperature measurements. By deferring
computations to the sensor, we minimize network traffic and energy
consumption. Examples similar to the ones below can be found in the
[demo directory](https://github.com/c3d/eliot/tree/master/demo).

### Simple temperature measurement (polling)

To measure the temperature on a remote node called "sensor.corp.net",
use the following code:

    temperature_on_sensor -> ask "sensor.corp.net", { temperature }
    writeln "Temperature is ", temperature_on_sensor

The `->` rewrite operator reads "transforms into" and is used in ELIoT
to define variables, functions, macros, and so on. Look into
[builtins.eliot](https://github.com/c3d/eliot/blob/master/src/builtins.eliot)
for examples of its use.

The `ask` function sends a program on a remote node, waits for its
completion, and returns the result. So each time we call
`temperature_on_sensor`, we send a program containing `temperature` on
the remote node called `sensor.corp.net`, and wait for the measured
value.


### Reporting sudden changes in temperatures

An application may be interested in sudden changes in temperatures,
e.g. if the sensor suddenly warms up. With ELIoT, without changing
anything to the temperature API, you can check the temperature every
second and report if it changed by more than 1 degree since last time
it was measured with the following program:

    invoke "sensor.corp.net",
        last_temperature := temperature
        every 1s,
            check_temperature temperature
        check_temperature T:real ->
            writeln "Measuring temperature ", T, " from process ", process_id
            if abs(T - last_temperature) >= 1.0 then
                reply
                    temperature_changed T, last_temperature
            last_temperature := T
    temperature_changed new_temp, last_temp ->
        writeln "Temperature changed from ", last_temp, " to ", new_temp

The `invoke` function sends a program to the remote node and opens a
bi-directional connexion, which allows the sensor to `reply` when it
feels it has useful data to report. In that case, the sensor replies
with a call to `temperature_changed`, sending back the old and new
temperature, and the controlling node can display a message using
`writeln`.



### Reporting changes in temperatures since last report

Another application may be interested in how temperature changes over
time, even if the change is gradual. In that case, you want to report
temperature if it changes by more than one degree since last time it
was *reported* (instead of measured). You can do that with a slight
variation in the code above, so that you update `last_temperature`
only after having transmitted the new value, not after having measured it:

    invoke "sensor.corp.net",
        last_temperature := temperature
        every 1s,
            check_temperature temperature
        check_temperature T:real ->
            writeln "Measuring temperature ", T, " from process ", process_id
            if abs(T - last_temperature) >= 1.0 then
                reply
                    temperature_changed T, last_temperature
                last_temperature := T
    temperature_changed new_temp, last_temp ->
        writeln "Temperature changed from ", last_temp, " to ", new_temp

In ELIoT, indentation is significant, and defined "blocks" of
code. Other ways to delimit a block of code include brackets
`{ code }` (which we used in the first example for `{ temperature }`,
as well as parentheses `( code )` or square brackets `[ code ]`. The
latter two are used for expressions.


### Computing average, min and max temperatures

Again using the same sensor, and again without any code or API change
on the sensor, you can also have it compute min, max and average
temperatures from samples taken every 2.5 seconds:
             
    invoke "sensor.corp.net",
        min   -> 100.0
        max   -> 0.0
        sum   -> 0.0
        count -> 0

        compute_stats T:real ->
            min   := min(T, min)
            max   := max(T, max)
            sum   := sum + T
            count := count + 1
            reply
                report_stats count, T, min, max, sum/count

        every 2.5s,
            compute_stats temperature

    report_stats Count, T, Min, Max, Avg ->
        writeln "Sample ", Count, " T=", T,
                " Min=", Min, " Max=", Max, " Avg=", Avg

Notice how the first parameter of `compute_stats`, `T`, has a type
declaration `T:real`. This tells ELIoT that a `real` value is expected
here. But it also forces ELIoT to actually compute the value, in order
to check that it is indeed a real number.

As a result, `temperature` is evaluated only once (to bind it to
`T`). If we had done the computation by replacing `T` with
`temperature`, we might have gotten inconsistent readings between two
subsequent evaluations of `temperature`, yielding possibly incorrect
results.


### Computing the difference between two sensors

Imagine now that you have two temperature sensors called
`sensor1.corp.net` and `sensor2.corp.net`, located in Sydney,
Australia, while your controlling application is located in Sydney,
Canada. If you need the difference in temperature between the two
sensors, wouldn't it make sense to minimize the traffic between Canada
and Australia, and have the two sensors talk to one another locally in
Australia?

This is very easy with ELIoT. The following program will only send a
traffic across the ocean if the temperature between the two sensors
differs by more than 2 degrees, otherwise all network traffic will
remain local:

    invoke "sensor2.corp.net",
       every 1.1s,
            sensor1_temp ->
                ask "sensor1.corp.net",
                    temperature
            send_temps sensor1_temp, temperature

       send_temps T1:real, T2:real ->
           if abs(T1-T2) > 2.0 then
               reply
                   show_temps T1, T2

    show_temps T1:real, T2:real ->
        write "Temperature on sensor1 is ", T1, " and on sensor2 ", T2, ". "
        if T1>T2 then
            writeln "Sensor1 is hotter by ", T1-T2, " degrees"
        else
            writeln "Sensor2 is hotter by ", T2-T1, " degrees"

This program looks and behaves like a single program, but will
actually be executing on three different machines that may possibly
located hundreds of miles from one another.


### A very powerful, yet simple API

With these examples, we have demonstrated that using ELIoT, we can
answer queries from applications that have very different requirements.
An application will get exactly the data it needs, when it needs it,
minimizing network traffic and optimizing energy utilization.

Yet the API on the sensors and on the controlling computer is
extremely simple. On the sensor, we only have a single function
returning the temperature. And on the controlling computer, we have a
single language that deals with data collection, timing, exchange
between nodes, computations, and more.


## How do we measure the temperature?

It is very simple to add your own functions to ELIoT, and to call any
C or C++ function of your choosing. The `temperature` function is
implemented in a file called
[temperature.tbl](https://github.com/c3d/eliot/blob/master/src/temperature.tbl).
It looks like this:

    NAME_FN(Temperature,            // Unique internal name
            real,                   // Return value
            "temperature",          // Name for ELIOT programmers

            // C++ code to compute the temperature and return it
            std::ifstream is("/sys/class/thermal/thermal_zone0/temp");
            int tval;
            is >> tval;
            R_REAL(tval * 0.001));

In that code, we read core temperature data as reported by Linux on
Raspberry Pi by reading the system file `/sys/class/thermal/thermal_zone0/temp`.
This file returns values in 1/1000th of a Celsius, so we multiply the
value we read by 0.001 to get the actual temperature.

To add the `temperature` module to ELIoT, we just need to add it to
the list of modules in the
[Makefile](https://github.com/c3d/eliot/blob/master/src/Makefile#L32):

    # List of modules to build
    MODULES=basics io math text remote time-functions temperature

This will build at least `temperature.tbl`. That file contains the
interface between ELIoT and your code. In simple cases like our
temperature measurement, it may be sufficient. However, if you have
files called `temperature.h` or `temperature.cpp`, they will be
integrated in your `temperature` module. This lets you add supporting
classes or functions.

The `tell`, `ask`, `invoke` and `reply` words are implemented in the
module called `remote`, which consists of
[remote.tbl](https://github.com/c3d/eliot/blob/master/src/remote.tbl),
[remote.h](https://github.com/c3d/eliot/blob/master/src/remote.h) and
[remote.cpp](https://github.com/c3d/eliot/blob/master/src/remote.cpp).
This may not be the easiest module to study, however. You may find
[io.tbl](https://github.com/c3d/eliot/blob/master/src/io.tbl) easier
to understand.


## Compiling ELIoT

To build ELIoT, just use `make`. On a Raspberry Pi, a `make -j3`
should run in about 10 minutes if you start from scratch. On a version
2, it's about one minute. On a modern PC, it's may be as low as 3 to 5
seconds. If `make` works (and it should), then use `sudo make install`
to install the target. In summary:

      % make
      % sudo make install

The default location is `/usr/local/bin/eliot`, but you can install in
`/opt/local/` for example by building with `make PREFIX=/opt/local/`.
Don't forget the trailing `/`.

If you are paranoid, you can, from the top-level, run `make check` to
validate that your installation is running correctly. It is possible
for a test named `04-basic-operators-in-function` to fail on some
machines, because C arithmetic for `<<` and `>>` operators is not
portable. I will fix that one day. If other tests fail, look into
file `tests/failures-default.out` for details of what went wrong.


### Running an ELIoT server

To start an ELIoT server on a node, simply run `eliot -l`.

    pi% eliot -l

By default, ELIoT listens on port 1205. You can change that by using
the `-listen` option:

    pi% eliot -listen 42105

Now, let's try a first test program. On `boss`, edit a file called
`hello.eliot`, and write into it the following code:

    tell "pi",
        writeln "Hello World"

Replace `"pi"` with the actual Internet name of your target
machine. Then execute that little program with:

    boss% eliot hello.eliot

Normally, the console output on `pi` should now look like this:

    pi% eliot -l
    Hello World

What happens behind the scene is that ELIoT on `boss` sent the program
given as an argument to `tell` to the machine named `pi` (which must
be running an ELIoT in listen mode, i.e. have `eliot -l`
running). Then, that program executes on the slave. The `tell` command
is totally asynchronous, it does not wait for completion on the target.

If this example does not work as intended, and if no obvious error
appears on the console of either system, you can debug things by
adding `-tremote` (`-t` stands for "trace", and enables specific debug
traces, in that case any code conditioned by `IFTRACE(remote)` in the
ELioT source code).


### The magic behind the scene

There are three key functions to send programs across nodes:

 * `tell` sends a program asynchronously
 * `ask` sends a program synchronously and waits for the result
 * `invoke` sends a program and opens a bi-directional channel.
   The node can then use `reply` to execute code back in the caller's
   program

ELIoT sends not just the program segments you give it, but also the
necessary data, notably the symbols required for correct evaluation. 
This is the reason why things appear to work as a single program.



## Basics of ELIoT syntax and semantics

ELIoT derives from [XLR](http://xlr.sourceforge.net). It is a
specially trimmed-down version that does not require LLVM and can work
in full interpreted mode, making it easier to compiler and use, but
also safer, since you cannot call arbitrary C functions.

### Semantics: One operator to rule them all

ELIoT has one fundamental operator, `->`, the "rewrite operator",
which reads as *transforms into*. It is used to declare variables:

    X -> 0

It can be used to declare functions:

    add_one X -> X + 1

The rewrite operator  can be used to declare other operators:

    X + Y -> writeln "Adding ", X, " and ", Y; X - (-Y)

But it is a more general tool than the operator overloading found in
most other languages, in particular since it allows you to easily
overload combinations of operators, or special cases:

    A in B..C -> A >= B and A <= C
    X * 1 -> X

Rewrites are considered in program order, and pattern matching finds
the first one that applies. For example, factorial is defined as follows:

    0! -> 1
    N! -> N * (N-1)!

Many basic program structures are defined that way in
[builtins.eliot](https://github.com/c3d/eliot/blob/master/src/builtins.eliot).
For example, if-then-else and infinite loops are defined as follows:

    if true  then X else Y      -> X
    if false then X else Y      -> Y
    loop Body                   -> Body; loop Body


### Syntax: Look, Ma, no keywords!

ELIoT has no keywords. Instead, the syntax relies on a rather simple
[recursive descent](https://en.wikipedia.org/wiki/Recursive_descent_parser)
[parser](https://github.com/c3d/eliot/blob/master/src/parser.cpp).

The parser has no keywords, and generates a parse tree made of * node
types. The first four node types are leaf nodes:

 * `Integer` is for integer numbers such as `2` or `16#FFFF_FFFF`.
 * `Real` is for real numbers such as `2.5` or `2#1.001_001_001#e-3`
 * `Text` is for text values such as `"Hello" or `'World'`. Text can
   be encoded using UTF-8
 * `Name` is for names and symbols such as `ABC` or `**`

The last four node types are inner nodes:

 * `Infix` are nodes where a named operator separates the operands,
   e.g. `A+B` or `A and B`.
 * `Prefix` are nodes where the operator precedes the operand, e.g.
   `+X` or `sin X`. By default, functions are prefix.
 * `Postfix` are nodes where the operator follows the operand, e.g.
   `3%` or `5km`.
 * `Block` are nodes with only one child surrounded by delimiters,
   such as `(A)`, `[A]` or `{A}`.

Of note, the line separator is an infix that separates statements,
much like the semi-colon `;`. The comma `,` infix is traditionally
used to build lists or to separate the argument of
functions. Indentation forms a special kind of block.

For example, the following code:

    tell "foo",
        if A < B+C then
            hello
        world

parses as a prefix `tell`, with an infix `,` as its right argument. On
the left of the `,` there is the text `"foo"`. On the right, there is
an indentation block with a child that is an infix line separator. On
the left of the line separator is the `if` statement. On the right is
the name `world`.

This parser is dynamically configurable, with the default priorities
being defined by the
[eliot.syntax](https://github.com/c3d/eliot/blob/master/src/eliot.syntax) file.

Parse trees are the fundamendal data structure in ELIoT. Any data or
program can be represented as a parse tree.


### ELIoT as a functional language

ELIoT can be seen as a functional language, where functions are
first-class entities, i.e. you can manipulate them, pass them around,
etc:

    adder X:integer -> (Y -> Y + X)

    add3 := adder 3
    add5 := adder 5

    writeln "3+2=", add3 2
    writeln "5+17=", add5 17
    writeln "8+2=", (adder 8) 2

However, it is a bit different in the sense that the core data
structure is the parse tree. Some specific parse trees, for example
`A+B`, are not naturally reduced to a function call, although they are
subject to the same evaluation rules based on tree rewrites.


### Subtlety #1: expression vs. statement

The ELIoT parse tree is designed to represent programs in a way that
is relatively natural for human beings. In that sense, it departs from
languages such as Lisp or SmallTalk.

However, being readable for humans requires a few special rules to
match the way we read expressions. Consider for example the following:

    write sin X, cos Y

Most human being parse this as meaning `write (sin(X),cos(Y))`,
i.e. we call `write` with two values resulting from evaluating `sin X`
and `cos Y`. This is not entirely logical. If `write` takes
comma-separated arguments, why wouldn't `sin` also take
comma-separated arguments? In other words, why doesn't this parse as
`write(sin(X, cos(Y))`?

This shows that humans have a notion of *expressions*
vs. *statements*. Expressions such as `sin X` have higher priority
than commas and require parentheses if you want multiple arguments. By
contrast, statements such as `write` have lower priority, and will
take comma-separated argument lists. An indent or `{ }` block begins a
statement, whereas parentheses `()` or square brackets `[]` begin an
expression.

There are rare cases where the default rule will not achieve the
desired objective, and you will need additional parentheses.

### Subtlety #2: infix vs. prefix

Another special rule is that ELIoT will use the presence of space on
only one side of an operator to disambiguate between an infix or a
prefix. For example:

    write -A    // write (-A)
    B - A       // (B - A)

### Subtlety #3: Delayed evaluation

When you pass an argument to a function, evaluation happens only when
necessary. Deferred evaluation may happen multiple times, which is
necessary in many cases, but awful for performance if you do it by
mistake.

Consider the following definition of `every`:

    every Duration, Body ->
        loop
            Body
            sleep Duration

In that case, we want the `Body` to be evaluated every iteration,
since this is typically an operation that we want to execute at each
loop. Is the same true for `Duration`?

One way to force evaluation is to give a type to the argument. If you
want to force early evaluation of the argument, and to check that it
is a real value, you can do it as follows:

    every Duration:real, Body ->
        loop
            Body
            sleep Duration
    
### Subtlelty #4: Closures and remote transport

Like many functional languages, ELIoT ensures that the value of
variables is preserved for the evaluation of a given body. Consider
for example:

    adder X:integer -> (Y -> Y + X)
    add3 := adder 3

In that case, `adder 3` will bind `X` to value `3`, but then the
returned value outlives the scope where `X` was declared. However, `X`
is referred to in the code. So the returned value is a *closure* which
integrates the binding `X->3`.

At this point, such closures cannot be sent across a `tell`, `ask`,
`invoke` or `reply`. Make sure data that is sent over to a remote node
has been evaluated before being sent.



