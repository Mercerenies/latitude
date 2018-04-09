
# The Chain Object

    Chain := Object clone.

A `Chain` object chains together two iterable objects into a single
iterable. They are most commonly constructed as a consequence
of [`Collection <>`](collection.md#collection--obj).

## Simple Slots

    Chain toString := "Chain".

## Methods

### `Chain first.`

This slot contains the first iterable, which should not be an
evaluating object.

### `Chain second.`

This slot contains the second iterable, which should not be an
evaluating object.

### `Chain iterator.`

Chains are iterable objects, so this method returns
a [`ChainIterator`](iterator.md#chainiterator). The iterator returned
is exactly as mutable as the underlying container being iterated.
