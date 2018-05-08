
# Booleans and the Nil Object

    Boolean := Object clone.
    True := Boolean clone.
    False := Boolean clone.
    Nil := Object clone.

The Boolean object models values which represent truth-hood or
falsehood. Its two subobjects, the true object and the false object,
represent these two concepts, respectively, of truth-hood and
falsehood. They are primarily used in Boolean statements such
as [if-statements](global.md#global-if-obj-then-block1-else-block2)
and [loop conditionals](global.md#global-while-cond-do-block).

The nil object models the concept of emptiness. The nil object is
also [iterable](iterator.md) and will always be viewed as an empty
collection.

## Simple Slots

    Boolean toString := "Boolean".

    False toString := "False".
    False toBool := False.
    False false? := True.

    True toString := "True".
    True true? := True.

    Nil toString := "Nil".
    Nil toBool := False.
    Nil nil? := True.

## Methods

### `Nil iterator.`

Returns a [`NilIterator`](iterator.md#niliterator) which iterates over
the empty collection.
