
# The Procedure Object

    Proc := Object clone.

The procedure object is the root object of callable constructs, which
includes evaluating objects, continuations, and any other built-in
object with a `call` method.

## Simple Slots

    Proc toString := "Proc".

## Methods

### `Proc call (args ...).`

Calling the root `Proc` object has no effect and returns
`Nil`. However, this method is frequently overriden in subobjects to
have more specific effects.

### `Proc <| arg.`

This method implements right-to-left function composition. A new
`Proc` object is returned which, when invoked, calls `arg` with its
arguments and then calls `self` with the result.

### `Proc |> arg.`

This method implements left-to-right function composition. A new
`Proc` object is returned which, when invoked, calls `self` with its
arguments and then calls `arg` with the result.

### `Proc shield.`

Returns the procedure. This method is overriden
in [`Method`](method.md) to return a non-evaluating form of the
method.
