
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

[[up](.)]
<br/>[[prev - Simple Flow Control](flow.md)]
