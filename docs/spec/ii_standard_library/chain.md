
# The Chain Object

    Chain := Object clone.

A `Chain` object chains together one or more iterable objects into a
single iterable. They are most commonly constructed as a consequence
of [`Collection <>`](collection.md#collection--obj).

## Simple Slots

    Chain toString := "Chain".

## Methods

### `Chain iterator.`

Chains are iterable objects, so this method returns
a [`ChainIterator`](iterator.md#chainiterator). The iterator returned
is exactly as mutable as the underlying container being iterated.

[[up](.)]
<br/>[[prev - The Cached Value Object](cached.md)]
<br/>[[next - The Collection Mixin Object](collection.md)]
