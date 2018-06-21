
# Advanced Flow Control

The basic control structures are used most of the time. However, in
those infrequent situations where you need a bit more control than a
simple if-statement or while-loop can provide, Latitude provides a
more powerful construct.

## Continuations

A continuation is a first-class representation of a specific point in
the code, which can then be later jumped to. Continuations are
constructed using `callCC`, which takes a block and passes the current
continuation as its argument.

    foo := callCC {
      putln "This will print".
      $1 call ("Result of callCC").
      putln "This will not print".
    }.
    putln (foo). ; Prints "Result of callCC"

Using continuations in this way allows us to implement early returns
from functions, the equivalent of return statements in other
programming languages.

    doSomeWork := {
      takes '[arg].
      callCC {
        exit := $1.
        arg nil? ifTrue {
          ;; If the argument is nil then do nothing
          exit call (Nil).
        }.
        ;; ... Some complicated work ...
      }.
    }.

This pattern is fairly common in Latitude, so it is implemented as a
method in its own right. Calling `escapable` within a `callCC` block
will enable `return` statements to exit that block.

    doSomeWork := {
      takes '[arg].
      callCC {
        escapable.
        arg nil? ifTrue {
          ;; If the argument is nil then do nothing
          return (Nil).
        }.
        ;; ... Some complicated work ...
      }.
    }.

However, this is not the only capability of continuations. Latitude
continuations are first-class, meaning that they can be invoked from
anywhere, even after the original block has exited. Here is a
rudimentary infinite loop implemented using only continuations.

    start := callCC { $1. }.
    putln: "This will never stop printing".
    start call (start).

From within the `callCC` block, we return the continuation object
itself and store it in `start`. Then, later, we call `start`, passing
itself as an argument, so that when we restart our makeshift loop,
`start` will retain its own value.

Obviously, this form of looping is not terribly efficient. Loops
written using `loop` or `while` use more primitive looping
instructions and are much more efficient, so the code above is
primarily for demonstrative purposes.

## Exceptions

Like most other scripting languages, Latitude supports exception
handling. Any object can be thrown as an exception object, although by
convention we usually only throw objects which are children of the
`Exception` object.

    % Exception clone throw.
    ***** EXCEPTION *****
    Exception - Exception!
    <stack trace omitted>
    % "A" throw.
    ***** EXCEPTION *****
    A

`Exception` objects respond to the `throw` message differently than
other Latitude objects, by attaching the current stack trace to the
object before throwing. This is why we get a significantly more
helpful message when throwing an `Exception`. We can attach a message
to the exception with `throwWith`.

    % Exception clone throwWith "Something really bad happened!".
    ***** EXCEPTION *****
    Exception - Something really bad happened!
    <stack trace omitted>

To handle exceptions, write a handler block. Handler blocks take the
form

    {
      blockBody.
    } handle {
      handlerBody.
    }.

The `blockBody` will be executed. If it terminates normally, the
handler is never called. If an exception (any exception at all) is
thrown, `handlerBody` will be run, with that exception as the sole
argument. If there are multiple handlers, they will be run in the
reverse order they were established.

    {
      {
        Exception clone throw.
      } handle {
        putln: "This will print first".
      }.
    } handle {
        putln: "This will print second".
    }.

After all the handlers have executed, a system exception handler will
run, which prints out the stack trace (if available) and then
terminates the program.

However, more than likely, we want to do more than print a message
when an exception occurs; we want to handle the problem and resume
execution. This is where continuations come in handy.

Accessing a variable which doesn't exist throws a `SlotError`, since
we're trying to access a nonexistent slot on the (lexical) scope
object. Let's write a function that tries to access a variable `value`
but doesn't crash if the variable doesn't exist.

    ;; Returns the value, or zero if it doesn't exist
    valueOrZero := {
      callCC {
        escapable.
        {
          value.
        } handle {
          return 0.
        }.
      }.
    }.

    ;; Value doesn't exist yet
    println: valueOrZero. ; ==> 0

    ;; Now let's make the variable
    value := 99.
    println: valueOrZero. ; ==> 99

You might recognize this pattern of "handle the exception then
resume", as it is the same pattern seen in try-catch statements of
other languages. Latitude provides a `catch` method, implemented using
the exact technique shown above. The following code will behave the
same way.

    ;; Returns the value, or zero if it doesn't exist
    valueOrZero := {
      {
        value.
      } catch (err SlotError) do {
        0.
      }.
    }.

    ;; Value doesn't exist yet
    println: valueOrZero. ; ==> 0

    ;; Now let's make the variable
    value := 99.
    println: valueOrZero. ; ==> 99

`handle` is primitive in the sense that it is implemented using
special VM instructions. `catch` (and its slightly more general cousin
`resolve`) are higher-level exception-handling techniques implemented
*in terms of* `handle`. In most common cases, `catch` will suffice and
should be used rather than calling `handle` directly.

There are [several](/spec/ii_standard_library/exception.md) built-in
exception types. Most of them are not available as global names but
are instead accessed within the `err` object, to avoid polluting the
global namespace. Constructing your own exception types is no
different from constructing any other object "type"; simply clone the
`Exception` object and make any modifications you please.

    MyException := Exception clone.
    MyException toString := "MyException".
    MyException message := "Some useful error message".

## Thunks

With the threat of continuations jumping around the code, it can be
difficult to enforce invariants. We haven't explicitly learned about
file objects yet, but like in any language, files must be closed when
the programmer is finished with them. A naive approach may look
something like this.

    local 'file = Stream open ("myfile.txt", "r").
    ;; Some work ...
    file close.

But if the "Some work ..." portion were to throw an exception or
perform a continuation jump, it is very likely that `file close` would
never executed. Latitude allows the programmer to construct thunks to
enforce invariants about the execution order of code.

    local 'file.
    thunk: {
      file = Stream open ("myfile.txt", "r").
    }, {
      ;; Some work ...
    }, {
      file close.
    }.

The global `thunk` method takes three arguments: a "before" method, a
"during" method, and an "after" method. It executes each of the three
of them in order. However, if a continuation jump or other control
flow technique causes the "during" method to be entered to exited
abnormally, the VM will guarantee that the "before" and "after"
methods are called as appropriate. So even if "Some work ..." throws
an exception, the file will be closed.

In the specific case of file handling, the `Stream` object provides
this behavior in a succinct way.

    Stream open ("myfile.txt", "r") closeAfter {
      ;; Here, self is the file object
      ;; Do some work on self ...
    }.

Finally, if only the "after" method is needed, `protect` can be used.

    {
      someMethodBody.
    } protect {
      putln: "Will be run unconditionally after the body".
    }.

## Summary

Now we have explored continuations and the two accompanying features:
handlers and thunks. You have all of the major control flow tools you
need to implement your own control structures now. In the next
chapter, we'll discuss input/output and files.

[[up](.)]
<br/>[[prev - Collections](arrays.md)]
<br/>[[next - Input and Output](io.md)]
