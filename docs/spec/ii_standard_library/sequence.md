
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

### Static Methods

#### `Sequence fromArray (arr).`

Returns a sequence containing the same elements, in the same order, as
the array.

## The Lazy Sequence Object

    LazySequence := Sequence clone.

A lazy sequence is a specialized type of sequence which does not
compute its head or tail until explicitly required to do so. These
sequences are usually constructed using `make` or `iterate`.

### Methods

#### `LazySequence toString.`

Forces the first element (if it exists) of the sequence and prints a
representation of the string, which includes the stringified first
element. The tail is not forced.

#### `LazySequence list.`

Returns the cons cell (or `Nil`) representing the start of the
sequence. The cons cell objects are specialized so that their `car`
and `cdr` will not force until necessary. Additionally, the `car` of
such cells is mutable but the `cdr` is immutable.

#### `LazySequence map! (block).`

Mutates the sequence by mapping the block over each element. This
method is overridden from `Collection map!` to be lazy so that
infinite sequences will behave correctly when this method is called.

### Static Methods

#### `LazySequence make (block).`

Given a procedure, this method produces an infinite lazy sequence.
Each element of the sequence is determined by calling (lazily,
on-demand) the procedure `block` with no arguments. The procedure is
guaranteed to be called in order, so the first call should return the
first element, the second should return the second, and so on. As
such, for any nontrivial uses of this method, `block` should be
stateful.

#### `LazySequence iterate (initial, procd).`

Produces an infinite lazy sequence. The first element of the sequence
will be `initial`. Each subsequent element is obtained by calling
`procd` and passing the previous element as the argument.

This method behaves in a similar way to the Haskell `iterate`
function.

## `SequenceIterator`

A sequence iterator is an iterator over a sequence object. A sequence
iterator is lazy if the underlying iterator is lazy, and a sequence
iterator is always a mutable iterator.
