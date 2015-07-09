# ELIoT
### Extensible Language for the Internet of Things

ELIoT is a very simple programming language specifcally designed to
facilitate the configuration and control of swarms of small
devices such as sensors or actuators. It can also be used as a
powerful, remotely-accessible extension language for larger
applications.


## Example: Measuring temperature from remote sensors

Imagine you have a sensor named `sensor.corp.net` running ELIoT,
which features a temperature measurement through a `temperature`
function. We can evaluate programs on this sensor remotely to do all
kind of interesting temperature measurements. By deferring
computations to the sensor, we minimize network traffic and energy
consumption. Examples similar to the ones below can be found in the
[demo directory]

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

Imagine that you are interested in sudden changes in temperatures,
e.g. if the sensor suddenly warms up. You can check the temperature
every second and report if it changed by more than 1 degree since last
time it was measured, you can use:

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
with a call to `temperature_changed` with the old and new temperature,
and the controlling node can display a message using `writeln`.



### Reporting changes in temperatures since last report

Maybe what really interests you is change over time, even if it's
gradual. In that case, you want to report temperature if it changes by
more than one degree since last time it was *reported* (instead of
measured). You can do that with a slight variation in the code above,
simply changing when you store `last_temperature`:

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

With the same sensor, and without changing anything on the target, you
can also have it compute min, max and average temperatures from
samples taken every 2.5 seconds:
             
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
and Australia, and have the two sensors talk to one another across the
room?

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
minimizing network traffic and optimizing energy utiliation.

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
temperature measurement, it may be sufficient. However, if you have a
file called `temperature.h`, it will be read too. That's where you
would add your own headers, classes or helper declarations. If you
have a file called `temperature.cpp`, it will also be added to the
module. That's where you can implement your helper functions or
classes.

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



## Basics of ELIoT syntax

ELIoT has one fundamental operator, `->`, the "rewrite operator",
which reads as *transforms into*. It is used to declare variables:

    X -> 0

It can be used to declare functions:

    add_one X -> X + 1

ELIoT is a functional language, where functions are first-class
entities, i.e. you can manipulate them, pass them around, etc:

    adder X:integer -> (Y -> Y + X)

    add3 := adder 3
    add5 := adder 5

    writeln "3+2=", add3 2
    writeln "5+17=", add5 17
    writeln "8+2=", (adder 8) 2

The rewrite operator can also be used for more powerful
transformations. For example, you can define operators, as in the
following factorial definition:

    0! -> 1
    N! -> N * (N-1)!

You can also define more complex structures. For instance,
if-then-else is define in `builtins.eliot` as follows:

    if true  then X else Y      -> X
    if false then X else Y      -> Y

Both in the case of factorial or if-then-else, we use pattern
matching, and we will select the first pattern that matches.
