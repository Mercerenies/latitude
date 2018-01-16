
# The Iterator Object and Iterators

    Iterator := Object clone.

An iterator is used to iterate over various data structures in
Latitude. The `Iterator` object is designed to be the parent to all
such iterators.

## Methods

### `Iterator iterator.`

The `iterator` method, in general, is designed to return an iterator
to a collection. On iterators, the method always returns the current
iterator, unmodified.

### `Iterator end?.`

Returns whether or not the iterator has reached its end.

### `Iterator next.`

Advances the iterator to its next element. The return value of `next`
is specific to the iterator and may not be meaningful. `next` should
only be called if `end?` is false; if the iterator has already reached
its end, `next` may not have meaningful semantics.

### `Iterator element.`

Returns the current element of the iterator. This method should only
be called if `end?` is false.

### `Iterator element = arg.`

Sets the current element of the collection being iterated over. This
method should only be called if `end?` is false. Many iterators are
read-only; if an iterator is read-only, this method should raise a
`ReadOnlyError` without modifying the iterator or the collection.

## Iterators

### `ArgIterator`

This iterator is returned by an `ArgList` and iterates over each
argument available to the list. `ArgIterator` is a mutable iterator,
and values that are mutated using `element=` are updated in the
current scope, even if the original value was defined in a higher
scope.

### `ArrayIterator`

This iterator is returned by an `Array` and iterates over each element
of the array in order. `ArrayIterator` is a mutable iterator.

### `ChainIterator.`

This iterator is returned by a `Chain` object and is used to chain
multiple iterators together. `ChainIterator` is mutable if and only if
the current underlying iterator is mutable. Note that this means
`ChainIterator` may be immutable for some of its iteration and mutable
for the rest, if one of its underlying iterators is mutable and the
other is not.

### `NilIterator.`

This iterator is returned by the nil object. The `NilIterator` is
always an empty iterator and is the identity of the chain operation on
iterators.

### `StringIterator.`

This iterator is returned by a `String` object and iterates over each
character or each byte, as determined by the semantics of the
string. In addition to the standard iterator methods, `StringIterator`
defines a `bytes?` method, which returns whether it is iterating over
bytes (true) or over characters (false).
