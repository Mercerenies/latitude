
# The String Object

    String := Object clone.

The string object represents string values in Latitude. Strings are
always encoded in UTF-8. It is possible for a string to contain
unassigned Unicode code points but it is an error for a string to
contain a sequence of bytes that is not valid UTF-8.

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
the `start` index and end at the `end` index, using
the [standard indexing](../appendix/terms.md#indexing) rules. This
method, unlike `substring`, counts only bytes of the string and not
characters, so care must be taken that a string is only split at
character boundaries.

### `String byteCount.`

Returns the number of bytes in the string. If the string contains only
ASCII characters, this is equal to its length. `byteCount` is
significantly faster than `length`, especially for long strings, as it
does not have to walk the string.

### `String radix (rad).`

Parses the string as an integer in radix `rad`, which must be an
integer from 2 to 36. The string is parsed in a language using the
digits `0` to `9` for the first ten values, then the letters `a` to
`z` as needed. The string is parsed in a case-insensitive manner, so
`A` to `Z` can be used in place of `a` to `z`. If the radix is
invalid, an `ArgError` is raised. If the string cannot be parsed in
the given radix, an `InputError` is raised. Otherwise, the integer
value is returned.

### `String findBytes (substr, index).`

Finds the first occurence of `substr` within `self`, starting at
`index`. If a match is found, the index where it starts is
returned. Otherwise, `Nil` is returned. The index is counted in bytes.

### `String find (substr, index).`

Finds the first occurence of `substr` within `self`, starting at
`index`. If a match is found, the index where it starts is
returned. Otherwise, `Nil` is returned. The index is counted in
characters.

### `String findFirst (substr).`

Finds the first occurrence of `substr`  within `self`. If a  match is
found,  the index  where it  starts is  returned. Otherwise,  `Nil` is
returned. The index is counted in characters.

### `String findAll (substr).`

Finds every occurrence of `substr` within `self`, returning an array
of indices for each start position. If no matches are found, an empty
array is returned.

### `String bytes?.`

Returns whether the string object is a byte string object. A byte
string object will always consider bytes and characters to be
equivalent, so any method which counts in characters will count in
bytes. String objects constructed from string literals are never byte
strings but can be converted to byte strings using the `bytes` method.

### `String bytes.`

This method returns a clone of the current string object which is
identical in every way except that it is a byte string object. A byte
string object always counts bytes, even in methods which would
normally count in characters. Additionally, iterators constructed on a
byte string will count forward in bytes as well. Methods which always
count in bytes are unaffected.

### `String substring (start, end).`

Returns a substring of the current string. The substring will start at
the `start` index and end at the `end` index, using
the [standard indexing](../appendix/terms.md#indexing) rules. This
method counts in characters.

### `String split (delim).`

Returns an array consisting of substrings of `self`, as delimited by
the string `delim`. Multiple consecutive instances of `delim` in
`self` will result in empty strings being added to the array.

### `String replace (substr, index, mthd).`

Returns a new string, acquired by replacing the first match of
`substr` in `self`, starting the search at `index` using
the [standard indexing](../appendix/terms.md#indexing) rules. The
matched string will be replaced with the result of `mthd
stringify`. The index is counted in characters.

### `String replaceFirst (substr, mthd).`

Returns a new string, acquired by replacing the first match of
`substr` in `self`. The matched string will be replaced with the
result of `mthd stringify`.

### `String replaceAll (substr, mthd).`

Returns a new string, acquired by replacing every match of `substr` in
`self`. The matched string will be replaced with the result of `mthd
stringify`. The method will be called once *for each* match, so it is
possible to construct a method which returns different results each
time to perform different replacements.

### `String padLeft (ch, n).`

Prepends `ch` to the string until the length is at least `n`. Returns
the new string.

### `String padRight (ch, n).`

Appends `ch` to the string until the length is at least `n`. Returns
the new string.

### `String asciiOrd.`

Returns the ASCII value of the first character of `self`. If `self` is
empty or its first character is not ASCII, an `ArgError` is raised.

### `String ord.`

Returns the Unicode value of the first character of `self`. If `self`
is empty, an `ArgError` is raised. Unlike many of the `String`
methods, `ord` is unaffected by whether the string is a byte string or
not; it will always return the Unicode value of the first UTF-8
character. [TODO: Is this behavior desired?]

### `String iterator.`

Returns a [`StringIterator`](./iterator.md#stringiterator) to the string which will
iterate over each character (or each byte, if the string is a byte
string).

### `String map (mthd).`

Applies the method to each character of `self`, returning a new string
containing the concatenated results. The results are concatenated with
`++`, which means `stringify` will be called to convert them to
strings.
