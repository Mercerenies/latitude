
# The Stream Object

    Stream := Object clone.

A `Stream` object stores a stream which can be written to or read
from. Some streams are unidirectional and are only designated for
reading or writing but not both. The direction of a stream can be
checked with `in?` and `out?`.

## Simple Slots

    Stream toString := "String".

## Methods

### `Stream in?.`

Returns whether the stream is designated for input.

### `Stream out?.`

Returns whether the stream is designated for output.

### `Stream puts (str).`

Sends `str`, which must be a string, to the stream. The stream must be
designated for output, or an `IOError` will be raised.

### `Stream putln (str).`

Sends `str`, which must be a string, to the stream, followed by a
newline. The stream must be designated for output, or an `IOError`
will be raised.

### `Stream print (obj).`

Sends `obj` to the stream. `toString` will be called on `obj` before
it is sent to the stream. The stream must be designated for output, or
an `IOError` will be raised.

### `Stream println (obj).`

Sends `obj` to the stream, followed by a newline. `toString` will be
called on `obj` before it is sent to the stream. The stream must be
designated for output, or an `IOError` will be raised.

### `Stream printf (obj, args...).`

Calls `obj` (a proc-like object) with `args...` as arguments. The
resulting string is sent to the stream. The stream must be designated
for output, or an `IOError` will be raised.

### `Stream readln.`

Reads one full line of input from the stream and returns it, as a
string. The newline is always omitted from the returned string. If the
stream has reached its end (`eof?`), the empty string is returned. If
the stream has not reached its end but is blocking, then this function
may block. If the stream is not designated for input, then an
`IOError` will be raised.

### `Stream read.`

Reads one character of input from the stream and returns it, as a
string. If the stream has reached its end (`eof?`), the empty string
is returned. If the stream has not reached its end but is blocking,
then this function may block. If the stream is not designated for
input, then an `IOError` will be raised.

Note carefully that, when calling this on the standard input stream
`stdin` in the REPL, the REPL shares the same input stream. So if
`stdin read` is called and more than one character is entered, the
remaining characters will be treated as a line of Latitude code by the
REPL.

### `Stream eof?.`

Returns whether or not the stream has reached its end.

### `Stream close.`

Closes the stream. Any further operations other than subsequent
`close` calls on the stream will have an undefined result. `close` is
idempotent, so that calling it on a closed stream will have no
effect. Streams should always be closed explicitly and *will not* be
closed by the VM.

### `Stream closeAfter (mthd).`

Calls the method `mthd`. The caller of this method will be the stream
object `self`. After the method terminates (or is exited for any
reason, such as a continuation jump), the stream will be closed with
`close`. Note that `closeAfter` uses the VM thunk system internally,
so `Kernel kill` will still bypass this method's protection.

### `Stream flush.`

Flushes the stream, forcing any buffered output to be written to the
stream's target. If the stream is not designated for output, an
`IOError` will be raised.

## Static Methods

### `Stream open (filename, mode).`

Returns a new `Stream` object which points to the file with name
`filename`. The stream will be opened with access and mode modifiers
depending on the `mode` argument, which should be a string.

| String      | Result                  |
| ----------- | ----------------------- |
| `"r"`       | Read-only               |
| `"w"`       | Write-only              |
| `"rb"`      | Read-only (binary)      |
| `"wb"`      | Write-only (binary)     |

Note that some operating systems may disregard the binary flag.

### `Stream exists? (filename).`

Returns whether or not a file with the given name exists.

[[up](.)]
<br/>[[prev - The Stack Frame Object](stackframe.md)]
<br/>[[next - The String Object](string.md)]
