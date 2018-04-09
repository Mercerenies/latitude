
# The Cons Object

    Cons := Object clone.

A `Cons` object, usually constructed with `cons` [TODO: Link] is a
simple pair of elements. A `Cons` object consists of a `car` and a
`cdr`.

## Simple Slots

    Cons toString := "Cons".

## Methods

### `Cons car.`

Retrieves the value in the `car` cell, without evaluating it if it is
a method.

### `Cons cdr.`

Retrieves the value in the `cdr` cell, without evaluating it if it is
a method.

### `Cons car = (arg).`

Stores `arg` in the `car` cell of the cons object.

### `Cons cdr = (arg).`

Stores `arg` in the `cdr` cell of the cons object.

### `Cons pretty.`

Produces a user-friendly string representation of the cons cell, using
the standard Lisp rules. If the `cdr` is nil, then the `car` is merely
enclosed in parentheses. If the `cdr` is another cons cell, then the
`car` is listed first, followed by the remainder of the "list". If the
`cdr` is any other object, then the cell will be printed as a dotted
list.
