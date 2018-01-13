
# The Kernel Object

    Kernel

The kernel object defines several language-level operators and
reflection functions, as well as replicating some of the behavior in
the root object so that it can be applied to non-traditional objects
as well.

## Simple Slots

    Kernel toString := "Kernel".
    Kernel GC := Object clone.
    Kernel GC toString := "GC".
    Kernel Slots := Object clone.
    Kernel Slots toString := "Slots".
    Kernel Parents := Object clone.
    Kernel Parents toString := "Parents".

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

While the lexical and dynamic scope arguments are not required to be
paired, it is generally recommended for callers to ensure that they
are. This is for the simple reason that many methods, especially those
that perform metaprogramming or reflection tasks, make the assumption
that scopes in the call stack are paired and may behave strangely if
this is untrue.

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

### `Kernel readHeader (filename).`

This method reads the first several lines of the Latitude script with
the given filename. These lines are parsed using the Latitude file
header rules, and a `FileHeader` object is returned. These rules are
described in detail in [TODO: `FileHeader` information]. If the file
does not exist or does not have a header of the appropriate format, a
`FileHeader` object containing nil slots is returned.

### `Kernel executablePath.`

This method returns the full pathname of the Latitude executable file,
including the filename. The directory portion of this pathname

### `Kernel protect (object, slotName).`

Adds assignment and delete protection to the given slot on the
supplied object. If the slot does not exist, [TODO: This is currently
a bug in the interpreter; correct and fill in the behavior here].

### `Kernel protected? (object, slotName).`

Returns whether the given slot on the supplied object has any
protections applied to it.

### `Kernel evaluating? (object).`

Returns whether the argument is an evaluating object or not. That is,
returns true if and only if the object has a primitive field
containing a method.

### `Kernel keys (object).`

Returns a list, as though allocated using the built-in list syntax, of
all of the slot names available on the object and its direct and
indirect parents. This method does not access any of the slots and
does not take `missing` results into consideration; it only returns
slots that actually exist in the inheritance hierarchy.

### Kernel eq (object1, object2).

Returns whether the two objects are in fact the same object. This
method behaves like `===` on `Object` but works on non-traditional
objects as well.

### Kernel id (object).

Returns a numerical identifier that uniquely represents the given
object. As long as the object is retained in memory, the identifier
will be unique to it. Once the object is garbage collected, its
identifier may be reused.

### `Kernel invokeOn (object, method).`

This method returns a procedure object. When invoked, the procedure
will call `method` with the arguments supplied to the procedure. The
caller at this time will be `object`. This method works like `Object
invoke` but also behaves correctly on non-traditional objects.

### `Kernel invokeOnSpecial (object, method, modifier).`

This method returns a procedure object. When invoked, the procedure
will call `method` with the arguments supplied to the procedure. The
caller at this time will be `object`.

This method acts like `Object invokeOn` but also behaves correctly on
non-traditional objects. As such, the behavior of `modifier` is the
same as if `Object invokeOn` were called.

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

## Subobject Methods

### `Kernel GC traced.`

Returns whether the garbage collector is currently being
traced. Tracing is initially disabled for the garbage collector but
can be enabled with the `trace` method.

### `Kernel GC run.`

Runs the garbage collector, cleaning up unreachable objects. If the
garbage collector is being traced, a message will be printed to the
standard error stream.

### `Kernel GC total.`

Returns the number of objects which are currently allocated for the
current VM.

### `Kernel GC trace.`

Enables tracing for the garbage collector. When the garbage collector
runs, a message will be printed to the screen. If tracing is already
enabled, this method does nothing.

### `Kernel GC untrace.`

Disables tracing for the garbage collector, so that messages will not
be printed when the garbage collector is invokved. If tracing is
already disabled, this method does nothing.

### `Kernel Slots hold (object, symbol).`

Returns the value of the given slot on `object`. If the result is a
method, it is *not* called. Note that this method may result in a call
to `missing` if the slot is not found through the standard full
retrieval.

### `Kernel Slots get (object, symbol).`

Returns the value of the given slot on `object`. If the result is a
method, it is called with `object` as the caller. As such, if `symbol`
is a literal, this is equivalent to simply invoking the method with no
arguments using the usual syntax.

### `Kernel Slots put (object, symbol, value).`

Sets the value of the given slot on `object`. If the symbol is a
literal value, this is equivalent to simply using the colon-equals
assignment.

### `Kernel Slots delete (object, symbol).`

If a slot with name `symbol` exists on `object` directly, then this
method deletes that slot. Note that this only removes direct slots and
does not interfere with slots from a parent class. This method always
returns the nil object.

### `Kernel Slots has? (object, symbol).`

Returns whether or not the slot exists on the object. Specifically,
this method attempts to access the slot using `Kernel Slots hold`. If
the access is successful, the return value is true. If the access
attempt throws a `SlotError`, the return value is false. Any other
exceptions or thrown objects are propogated.

### `Kernel Parents origin (object, symbol).`

Locates the object in the argument's inheritance hierarchy where the
slot with the given name is defined. If the slot was defined directly
on the argument, the argument object is returned. Otherwise, if it was
defined on a parent and inherited, then the relevant parent object is
returned. If the slot does not exist on the object, then a `SlotError`
is raised. The `missing` method of the object is not used in this
case, as only slots that actually exist in the hierarchy are checked.

### `Kernel Parents above (object, symbol).`

This method identifies the origin of the given slot on the argument
object, as though through `Kernel Parents origin`. It then returns the
value of the slot on the origin's parent. That is, it ignores the
current value of the slot and attempts to access the inherited
value. Effectively, this method emulates the `super` construct
available in other languages. If the slot's value is a method, it is
returned without calling.

### `Kernel Parents hierarchy (object).`

Returns an array containing, in order, all of the objects in the
argument's inheritance hierarchy.
