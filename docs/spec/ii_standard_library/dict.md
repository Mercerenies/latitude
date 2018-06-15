
# The Dictionary Object

    Dict := Object clone.

A dictionary stores key-value pairs, where the keys are symbols and
the values are Latitude objects. The same behavior can be approximated
with non-traditional objects, with the caveat that the `parent` slot
can then not be used as a key. With dedicated dictionary objects, any
symbol can be used as a key.

Dictionaries are iterable in three ways. The standard `iterator`
method returns an iterator with elements cons cells containing
key-value pairs. `keys` returns an iterable construct over the
collection of keys in the dictionary. `values` returns an iterable
construct over the collection of values in the dictionary. All three
of these iterators are read-only. The first two are invalidated if
keys are added to or removed from the dictionary, but `values` is
invalidated if any key *or* value is changed.

## Simple Slots

## Methods

### `Dict toString.`

Returns a string representation of the dictionary, approximating a
string which would, if evaluated, produce an equivalent dictionary
object.

### `Dict clone.`

Returns a clone of the current dictionary object.

### `Dict dup.`

Returns a duplicate of the current dictionary object.

### `Dict get (key).`

Returns the value at the given key in the dictionary, without
evaluating methods. If `key` is not a symbol, `TypeError` is
raised. If `key` does not exist in the dictionary, `SlotError` is
raised.

### `Dict get (key) = value.`

Sets the value associated with the given key in the dictionary. If the
key does not exist, it is created and the value is
assigned. Otherwise, the value overwrites the previous one at that
key.

### `Dict has? (key).`

Returns whether or not the dictionary contains a value at the given
key.

### `Dict delete (key).`

If `key` exists as a key in the dictionary, it is removed from the
dictionary.

### `Dict iterator.`

Returns a [`DictIterator`](iterator.md#dictiterator) iterating over
the dictionary. The `DictIterator` is invalidated if keys are added to
or removed from the dictionary but is not invalidated if values are
modified.

### `Dict keys.`

Returns a collection containing all of the keys currently in the
dictionary.

### `Dict values.`

Returns a collection containing all of the values currently in the
dictionary.

### `Dict map (block).`

Maps a method over the dictionary. This behaves equivalently to the
corresponding `Collection` method but is specialized to dictionaries.

### `Dict == other.`

Returns true if and only if the dictionaries have the same keys and
each key corresponds to an equivalent (by `==`) value in both
dictionaries.

## Static Methods

### `Dict empty.`

Returns a fresh empty dictionary.

[[up](.)]
<br/>[[prev - The Continuation Object](cont.md)]
<br/>[[next - The Ellipsis Object](ellipsis.md)]
