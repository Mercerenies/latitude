
# The Process Object

    Process := Object clone.

A `Process` object represents a subprocess of the Latitude process and
stores references to its pipes, as well as providing the ability to
query the process for basic information about its runtime.

## Simple Slots

    Process toString := "Process".

## Methods

### `Process stdin.`

Returns the standard input stream of the process, as a `Stream`
object. Note that, from Latitude's perspective, this is an *output
stream* that will forward any output sent to it into the process'
input.

### `Process stdout.`

Returns the standard output stream of the process, as a `Stream`
object. Note that, from Latitude's perspective, this is an *input
stream* that will read input from the process' standard output stream.

### `Process stderr.`

Returns the standard error stream of the process, as a `Stream`
object. Note that, from Latitude's perspective, this is an *input
stream* that will read input from the process' standard error stream.

### `Process finished?.`

Returns true if the process has been run and has terminated.

### `Process running?.`

Returns true if the process has been started and is currently running.

### `Process exitCode.`

Returns the numerical exit code of the process. If the process is not
finished, returns `-1`.

### `Process execute.`

Runs the process. If the process has already started or has finished,
the behavior is undefined.

## Static Methods

### `Process spawn (cmd).`

Constructs a new process object whose command is `cmd`, which must be
a string. This does not start the process but merely constructs the
resources necessary to start it in the future.

[[up](.)]
<br/>[[prev - The Procedure Object](proc.md)]
<br/>[[next - The Range Object](range.md)]
