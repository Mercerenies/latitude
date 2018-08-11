
# The Symbol Object

    Symbol := Object clone.

The symbol object models symbols. A symbol is an index to an immutable
string stored in the global symbol table. Since symbols are internally
simple indices, they have constnat-time comparisons for equality and
ordering, which makes them excellent as keys in a tree-like data
structure.

Note carefully that, unlike in many Lisp dialects, in Latitude symbols
are case-sensitive, so `'a` is not the same as `'A`.

More information on symbols as a data type can be found
in [Symbol Table](../i_syntax_and_semantics/ch3_object.md#symbol-table)

## Simple Slots

## Methods

### `Symbol toProc.`

Returns a [`Proc`](proc.md) object which, when called, invokes the
method at the given slot on the first argument, with all remaining
arguments being passed to the invoked method. More concisely, the
following two calls are equivalent in behavior.

    foo bar: 1, 2, 3.
    'bar toProc call: foo, 1, 2, 3.

### `Symbol asText.`

Returns the name of the symbol, as a string. Remember that the tilde
at the start of a generated symbol is a part of its name but the
single quote at the start of a standard or natural symbol is not. So
`'foo asText` returns `"foo"` but `~foo asText` returns `"~foo"`.

### `Symbol toString.`

Returns a string representation of the symbol. The string
representation shall be a symbol literal which could be parsed in
Latitude syntax to produce a symbol with the same name. Thus, the
string representation will begin with either a single quote or a
tilde.

### `Symbol pretty.`

Returns the name of the symbol as a string. This method is equivalent
to `Symbol asText`.

### `Symbol == arg.`

Returns whether the two symbols are the same. This is a constant-time
operation and thus does not depend on the length of the symbol's
name. Note that two symbol literals may allocate distinct Latitude
objects which point to the same symbol, so while it is certainly true
that `'a == 'a`, it may or may not be true that `'a === 'a`.

### `Symbol < arg.`

Returns whether `self` is less than `arg`. The ordering used is not
necessarily the lexicographic ordering of the names; it is an
arbitrary but consistent total ordering of the symbols in the symbol
table.

## Static Methods

### `Symbol gensym.`

Returns a new generated symbol. This method is a useful way to
generate new unique symbols for use as keys in data structures. The
name of the generated symbol shall begin with `~G`, followed by an
arbitrary numeral. An invocation of this method is equivalent to
`Symbol gensymOf "G"`.

### `Symbol gensymOf (prefix).`

Returns a new generated symbol, like `gensym`. The name of the
generated symbol shall begin with `~`, followed by the prefix,
followed by an arbitrary numeral. The prefix value must be a string.

[[up](.)]
<br/>[[prev - The String Object](string.md)]
<br/>[[next - The Version Object](version.md)]
