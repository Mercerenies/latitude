
# The Exception Object and Built-in Exceptions

    Exception := Object clone.

An exception is an error condition, usually raised with the `throw`
method. The details of the exception-handling mechanism in Latitude
can be found
at
[Exceptions](../i_syntax_and_semantics/ch6_controlflow.md#exceptions).

## Simple Slots

    Exception message := "Exception!".
    Exception toString := "Exception".
    Exception stack := Nil.

## Methods

### `Exception throw.`

Throws the current object, triggering any exception handlers that are
in scope. Unlike [`Object throw`](object.md#object-throw), this method
assigns a `stack` slot to the curent object, which contains the
current stack trace at the time the exception was thrown, as per
`currentStackTrace` [TODO: Link].

### `Exception pretty.`

Produces a string represent of the exception which consists of its
name (as per `toString`), followed by a delimiter (usually a dash),
followed by the exception's message (as per `message`).

### `Exception printStackTrace.`

Prints the stack trace of the exception object. This call is
equivalent to `self stack dump.`.

## Exceptions

The `err` singleton object contains several built-in exceptions, which
are thrown by standard library functions in the case of
failure. Unless otherwise stated, all of the below exceptions are only
found in slots on `err`, not in the global scope. Additionally, each
exception listed here has a `toString` slot which evaluates to its own
name and, unless otherwise stated, is a subobject of `SystemError`.

### `Exception`

The root exception object, which is defined in `err` in addition to
being available as a globally-scoped name.

### `SystemError`

    err SystemError message := "System Exception!".

This is the parent object of all exceptions thrown by the standard
library. Unlike many of the exceptions available here, it is available
as a globally scoped name *and* within `err`. `SystemError` is a
direct subobject of `Exception`.

### `ArgError`

    err ArgError message := "Argument error".

This exception is thrown when arguments to a method are invalid and
none of the more specific exceptions make sense for the situation.

### `BoundsError`

    err BoundsError message := "Bounds error".

This exception is thrown when an invalid index is provided to a
container-like structure.

### `ContError`

    err ContError message := "Continuation error".

[TODO: Is this used?]

### `IOError`

    err IOError message := "IO error".

This exception is thrown if an error occurs while performing an I/O
operation.

### `InputError`

    err InputError message := "Input error".

This exception is thrown when a string parse is attempted, and the
string fails to be recognizable as a valid value of the target type.

### `IntegrityError`

    err IntegrityError message := "Integrity error".

This exception is thrown when a string of a specific format is
expected, and the string does not satisfy the given format.

### `UTF8IntegrityError`

    err UTF8IntegrityError message := "UTF-8 Integrity error".

A subobject of `IntegrityError`, this exception is thrown if a string
which is not a valid UTF-8 string is constructed.

### `LangError`

    err LangError message := "Error reading from external language".

This exception is thrown when a language parse (using the `#()`
syntax) is attempted and fails.

### `ModuleError`

    err ModuleError message := "Module error".

This exception is thrown when a module load is attempted and fails,
usually because a module with the given name cannot be found.

### `NotSupportedError`

    err NotSupportedError message := "Not supported".

This exception is thrown when a platform-specific method is invoked
and is not supported on the current platform.

### `ParseError`

    err ParseError message := "Parse error".

This exception is thrown by the Latitude parser in the case of a
syntax error within Latitude code.

### `ReadOnlyError`

    err ReadOnlyError message := "Write access prohibited".

This exception is thrown when a write is attempted on data which is
intended as read-only, such as attempting to modify the elements of a
read-only iterator.

### `ProtectedError`

    err ProtectionError message := "Protected variable".

This exception, a subobject of `ReadOnlyError`, is thrown by the
Latitude VM if an operation is attempted which violates the slot
protection of a slot.

### `SlotError`

    err SlotError slotName := Nil.
    err SlotError objectInstance := Nil.

This exception is thrown if a slot which does not exist is accessed on
an object. The `message` slot of this object is a method which returns
an appropriate message, containing the slot name.

### `SystemArgError`

    err SystemArgError message := "Wrong number of arguments to system call".

As the message name indicates, this exception is thrown when a
system-level call is made with incorrect arguments.

### `SystemCallError`

[TODO: Is this one used?]

### `TypeError`

    err TypeError message := "Type error".

This exception is thrown if an object whose type or characteristics
are not what are expected is passed to a method.
