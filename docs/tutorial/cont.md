
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

[[up](.)]
<br/>[[prev - Collections](arrays.md)]
