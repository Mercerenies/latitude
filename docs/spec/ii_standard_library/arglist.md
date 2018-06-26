
# The Argument List Object

    ArgList := Object clone.

An argument list object encapsulates an argument-like object as an
iterable collection. Any object which has fields of the form `$1`,
`$2`, ... can be encapsulated in this way, so that those fields can be
iterated over. Argument list objects are most frequently constructed
using the global [`$*`](global.md#global-) method.

## Simple Slots

    ArgList toString := "ArgList".

## Methods

### `ArgList clone.`

Returns a copy of the argument list object. The internal data is
shallow-copied when an argument list is cloned.

### `ArgList fill.`

Stores the caller's dynamic scope in `self`'s internal memory,
effectively filling the argument list object with the arguments of the
caller's scope. Returns `self`.

### `ArgList fillWith (arg).`

Stores the given argument-like object in `self`'s internal memory. An
argument-like object is an object which has slots of the form `$1`,
`$2`, .... The longest unbroken prefix of such slots will be taken to
be the argument list's elements. In particular, any dynamic scope
object is an argument-like object,
and [`$argv`](global.md#global-argv) is also an argument-like object.
Returns `self`.

### `ArgList shift.`

Rotates the argument list object by taking the first argument, placing
it at the end of the argument list, and shifting all the other
arguments forward by one. The argument that was moved to the end is
returned. If the argument list was empty, no modifications are
performed, and `Nil` is returned.

### `ArgList unshift.`

Rotates the argument list object by taking the final argument, placing
it at the beginning of the argument list, and shifting all the other
arguments backward by one. The argument that was moved to the
beginning is returned. If the argument list was empty, no
modifications are performed, and `Nil` is returned.

### `ArgList unshiftOnto (arg).`

Similar to `unshift`, this method rotates the argument list by
shifting all the elements backward by one and placing `arg` onto the
front of the argument list. `arg` is returned. Note that, unlike
`shift` and `unshift`, this method increases the size of the argument
list by one.

### `ArgList iterator.`

Returns an [`ArgIterator`](iterator.md#argiterator) object which
iterates over the given `ArgList` object.

### `ArgList dropIn.`

Drops the current argument values into the caller's scope. Equivalent
to `ArgList dropInto ($dynamic)`.

### `ArgList dropInto (arg).`

`dropInto` is the inverse operation of `fillWith`. This method inserts
arguments from the argument list into the slots `$1`, `$2`, ... of the
`arg` object.

[[up](.)]
<br/>[[next - The Array Object](array.md)]
