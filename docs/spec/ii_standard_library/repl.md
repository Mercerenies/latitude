
# The REPL Module

    repl.lat

The REPL module defines the appropriate object methods for running a
read-eval-print loop. If the Latitude VM was started in REPL mode,
this module is imported as part of the standard load process.
Otherwise, the module can be imported explicitly in order to simulate
a REPL within Latitude.

## Load Effects

`$except` and `$it` are defined globally, which delegate to,
respectively, `REPL exception` and `REPL lastResult`.

## The REPL Object

    REPL := Object clone.

### Simple Slots

    REPL toString := "REPL".

### Static Methods

#### `REPL quit.`

Exits the VM and terminates the program. Unlike `Kernel kill`, `REPL
quit` will correctly run any handlers which were put into place
*after* the REPL module was loaded.

#### `REPL exception.`

Returns the last uncaught exception triggered within code evaluated by
the REPL's loop. If no exception has occurred, `Nil` is returned.

#### `REPL lastResult.`

Returns the last value successfully returned by code evaluated in the
REPL's loop. If no value has been successfully returned, `Nil` is
returned.

#### `REPL eof?.`

Returns whether the standard input stream has reached its end.
Equivalent to `stdin eof?`.

#### `REPL read.`

Prints an appropriate prompt to `stdout` and reads a line of input
from `stdin`. The string entered by the user is returned.

#### `REPL eval (text).`

Evaluates the text, in the REPL's appropriate scope. The result is
returned. If an exception is raised, the exception is propogated
outward.

#### `REPL print (arg).`

Prints the argument. Equivalent to `stdout println (arg)`.

#### `REPL loop.`

Runs the REPL loop itself. In an infinite loop, `read` is called,
followed by `eval`, then `print`. Additional handling is enabled
within the loop, so that if standard input reaches its end, the
program terminates, and any exceptions (or other objects) thrown will
be caught and handled by the loop. As such, barring an incredibly
long-range continuation jump, this method will never exit until it is
time for the VM process to end.

[[up](.)]
<br/>[[prev - The Random Module](random.md)]
<br/>[[next - The Sequence Module](sequence.md)]
