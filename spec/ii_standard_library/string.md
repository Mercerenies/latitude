
# The String Object

    String := Object clone.

The string object represents string values in Latitude. Strings are
always encoded in UTF-8. It is possible for a string to contain
unassigned Unicode code points but it is an error for a string to
contain a sequence of bytes that is not valid UTF-8.

## Simple Slots

## Methods

### `String intern.`

Returns a symbol whose name is `self`. Note that if `self` begins with
a tilde, an uninterned symbol will be returned, so `("~a" intern) ==
("~a" intern)` is false.

### `String stringify.`

Returns `self`. For non-string objects, `stringify` calls
`toString`. As such, it is a convenient way to convert a value which
may already be a string to a string.

### `String toString.`

Returns a string representing `self`. The resulting string will be
enclosed in quotes and will have characters escaped as necessary.

### `String pretty.`

Returns `self.`

### `String == obj.`

Returns whether the two strings contain the same sequence of bytes.

### `String < obj.`

Returns whether `self` is less than `obj` according to case-sensitive
lexicographic ordering.

[TODO: This currently compares byte-by-byte. Should we compare by
Unicode code points?]

### `String substringBytes: start, end.`

Returns a substring of the current string. The substring will start at
the `start` index and end at the `end` index. [TODO: Finish
documenting]

[TODO: The rest of `String`]

## Static Methods
