
# Simple Flow Control

Like every other nontrivial language, Latitude provides methods for
conditional branching, looping, and the like.

## Do Blocks

The simplest form of flow control, arguably not even flow control, is
the `do` block, which takes a single method and runs it,
unconditionally once.

    do {
      putln "This will be printed once".
    }.

This sort of block is only really useful in situations where some
intermediate local variables are needed, but you don't want those
variables polluting the enclosing scope.

    foo := do {
      a := intermediateValue1.
      b := intermediateValue2.
      ;; ... Some complicated expression involving a and b
    }.
    ;; Now a and b only exist inside the do-block and
    ;; don't pollute the outer scope

... (Discuss booleans and nil next)

[[up](.)]
<br/>[[prev - Variables](vars.md)]
