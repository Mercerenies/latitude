
# The Kernel Object

    Kernel := Object clone.

The kernel object defines several language-level operators and
reflection functions, as well as replicating some of the behavior in
the root object so that it can be applied to non-traditional objects
as well.

## Simple Slots

    Kernel toString := "Kernel".
    Kernel GC := Object clone.
    Kernel GC toString := "GC".

## Static Methods

### `Kernel kill.`

This method performs a hard kill of the interpreter, immediately
terminating the program.

### `Kernel eval (lexical, dynamic, string).`

Evaluates the string as a Latitude script, which can consist of zero
or more statements. The string is evaluated in the given lexical and
dynamic scopes. Note that a method call is *not* simulated; the code
is evaluated in the scopes that are provided. `eval` returns the
result of the execution and propogates any thrown objects.

### `Kernel evalFile (filename, lexical).`

Reads the entire contents of the given file and evaluates them as a
Latitude script. The script is evaluated in the provided lexical
scope, paired with a clone of the current dynamic scope. Note that,
unlike with `eval`, a method call is simulated in this case, so while
the provided lexical scope is used without cloning, the usual
variables
bound
[in a method call](../i_syntax_and_semantics/ch4_evaluation.md#method-calls) are
bound in this case. `evalFile` returns the result of the execution and
propogates any thrown objects.

What constitutes a valid filename depends on the underlying operating
system. On most commonly-used systems, `evalFile` will accept either a
relative or absolute pathname.

### `Kernel compileFile (filename).`

Compiles the file into Latitude bytecode, storing the compiled result
in a new file in the same directory as `filename`, with the same name,
except that the letter "c" is appended to the end.

What constitutes a valid filename depends on the underlying operating
system. On most commonly-used systems, `compileFile` will accept either a
relative or absolute pathname.

### `Kernel load (filename).`

Reads and evaluates the given file in a clone of the global lexical
scope. This is equivalent to

    Kernel evalFile (filename, global clone).

### `Kernel loadRaw (filename).`

Reads and evaluates the given file in a clone of the global lexical
scope. Unlike with `load`, `loadRaw` always takes a relative path and
assumes the path to be relative to the directory containing the
Latitude executable, *not* the current working directory. This method
is primarily intended for internal use and should rarely need to be
invoked explicitly by the user.

### `Kernel cloneObject (object).`

This method returns a clone of the argument object.

### `Kernel dupObject (object).`

Returns a new object identical to `object` but distinct from it. This
is different from cloning an object, which returns a new object whose
*parent* is the object. This copies the actual slots of the object
into a new object which shares the original's parent.

### `Kernel readHeader (filename).`

This method reads the first several lines of the Latitude script with
the given filename. These lines are parsed using the Latitude file
header rules, and a `FileHeader` object is returned. These rules are
described in detail in [`FileHeader`](fileheader.md). If the file does
not exist or does not have a header of the appropriate format, a
`FileHeader` object containing nil slots is returned.

### `Kernel executablePath.`

This method returns the full pathname of the Latitude executable file,
including the filename.

### `Kernel cwd.`

This method returns the full pathname of the current working directory
from which the Latitude executable is being run.

### `Kernel evaluating? (object).`

Returns whether the argument is an evaluating object or not. That is,
returns true if and only if the object has a primitive field
containing a method.

### `Kernel directKeys (object).`

Returns a list, as though allocated using the built-in list syntax, of
all of the slot names available *directly* on the object. This
excludes any slots belonging to parent objects and does not take into
account `missing` results. As such, it returns exactly the keys which
can be deleted from `object` using `Slots delete`.

### `Kernel keys (object).`

Returns a list, as though allocated using the built-in list syntax, of
all of the slot names available on the object and its direct and
indirect parents. This method does not access any of the slots and
does not take `missing` results into consideration; it only returns
slots that actually exist in the inheritance hierarchy.

### `Kernel eq (object1, object2).`

Returns whether the two objects are in fact the same object. This
method behaves like `===` on `Object` but works on non-traditional
objects as well.

### `Kernel id (object).`

Returns a numerical identifier that uniquely represents the given
object. As long as the object is retained in memory, the identifier
will be unique to it. Once the object is garbage collected, its
identifier may be reused.

### `Kernel invoke (method) on (object).`

`invoke` returns a procedure object. On this procedure object, an `on`
method is defined, which sets the target object. When invoked, the
procedure will call `method` with the arguments supplied to the
procedure. The caller at this time will be the target object. If the
target object has not been set, it defaults to `Nil`. The `on` method
returns the procedure object.

Additionally, the returned procedure has a method `by` defined on it.
If `by` is called with a method argument, then the procedure object is
modified to have a handler method. This adds an additional step to the
call. When the procedure is invoked, after the new lexical and dynamic
scopes are constructed, the handler will be called with two arguments:
the lexical and dynamic scopes. The handler method is free to modify
these scopes for metaprogramming purposes. `by` can be called multiple
times, and if multiple handlers are supplied then they will each be
executed in the order they were added.

Example use:

    Kernel invoke (method) on (object) call.
    Kernel invoke (method) on (object) by (handler) call.

### `Kernel env (name).`

Returns the value of the shell environment variable with the given
name. If the variable exists, its value as a string is returned. If
the variable does not exist, the nil object is returned.

### `Kernel env (name) = value.`

Sets the shell environment variable with the given name. The value
should either be a string or nil. If the value is nil, the environment
variable with the given name is unset. If the value is a string, the
environment variable is set to the given value. This method always
returns nil.

## Inner Object Methods

### `Kernel GC run.`

Runs the garbage collector, cleaning up unreachable objects. If the
garbage collector is being traced, a message will be printed to the
standard error stream.

### `Kernel GC total.`

Returns the number of objects which are currently allocated for the
current VM.

### `Kernel GC limit.`

Returns the number of objects which will trigger the garbage
collector.

### `Kernel GC trace.`

Enables tracing for the garbage collector. When the garbage collector
runs, a message will be printed to the screen. If tracing is already
enabled, this method does nothing.

### `Kernel GC untrace.`

Disables tracing for the garbage collector, so that messages will not
be printed when the garbage collector is invokved. If tracing is
already disabled, this method does nothing.

[[up](.)]
<br/>[[prev - The Iterator Object and Iterators](iterator.md)]
<br/>[[next - The Method Object](method.md)]
