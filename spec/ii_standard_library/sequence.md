
# The Sequence Module

    sequence.lat

The sequence module defines methods for interacting with (finite or
infinite) forward sequences of data, akin to forward-linked lists in
many languages.

## The Sequence Object

    Sequence := Object clone.

A sequence is a forward-linked list of data. Like an array, a sequence
is an iterable collection, so it implements
the [`Collection`](collection.md) mixin. Sequence objects are most
commonly constructed using `fromArray`.

### Methods

#### `Sequence toString.`

Returns a string representation of the sequence.

#### `Sequence list.`

Sequences are implemented in terms of [`Cons`](cons.md) cells. This
method returns the cons cell representing the start of the sequence.
If the sequence is empty, `Nil` is returned.

#### `Sequence iterator.`

Returns an iterator over the sequence. The returned iterator will be a
`SequenceIterator`.

#### `Sequence head.`

Returns the first element of the sequence, or `Nil` if the sequence is
empty.

#### `Sequence tail.`

Returns a clone of the current sequence object which holds all but the
first element of the current sequence. The two sequences share cons
cells, so changes to one will be reflected in the other. If the
sequence is empty, another empty sequence is returned.

#### `Sequence lazy.`

Coerces the sequence object into a `LazySequence`, returning a new
`LazySequence` object over the same data.

### Static Methods

#### `Sequence fromArray (arr).`

Returns a sequence containing the same elements, in the same order, as
the array.

## The Lazy Sequence Object

    LazySequence := Sequence clone.

A lazy sequence is a specialized type of sequence which does not
compute its head or tail until explicitly required to do so. These
sequences are usually constructed using `Proc iterate`.

### Methods

#### `LazySequence toString.`

Forces the first element (if it exists) of the sequence and prints a
representation of the string, which includes the stringified first
element. The tail is not forced.

[TODO: The rest of this]

## `SequenceIterator`

A sequence iterator is an iterator over a sequence object. A sequence
iterator is lazy if the underlying iterator is lazy, and a sequence
iterator is always a mutable iterator.
