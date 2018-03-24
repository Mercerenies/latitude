
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

Returns a `Proc` [TODO: Link] object which, when called, invokes the
method at the given slot on the first argument, with all remaining
arguments being passed to the invoked method. More concisely, the
following two calls are equivalent in behavior.

    foo bar: 1, 2, 3.
    'bar toProc call: foo, 1, 2, 3.

### `Symbol asText.`

Returns the name of the symbol, as a string. Remember that the tilde
at the start of an uninterned symbol is a part of its name but the
single quote at the start of an interned symbol is not. So `'foo
asText` returns `"foo"` but `~foo asText` returns `"~foo"`.

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

### `Symbol assign = mthd.`

When called, this method defines a method in the current scope which
performs an assignment of the variable with the name `self`. The newly
constructed method will be assigned in either the current lexical or
current dynamic scope depending on
the
[name of the symbol](../i_syntax_and_semantics/ch4_evaluation.md). The
newly constructed method's name will be the name of the caller with an
equal sign `=` appended, so that it can be called using the assignment
syntax. When called with one argument, the new method will invoke
`mthd` with the argument and assign the returned value to the symbol
named by the caller of `assign=`. For example,

    var := 1.
    'var assign = { $1. }.
    while { var < 10. } do {
      var printObject.
      var = var + 1.
    }.

Without the `assign=` call, it would be necessary to use `parent var
:= var + 1` to increment the variable, which can become a nuisance
when methods are nested several scopes deep.

## Static Methods

### `Symbol gensym.`

Returns a new uninterned symbol. This method is a useful way to
generate new unique symbols for use as keys in data structures. The
name of the generated symbol shall begin with `~G`, followed by an
arbitrary numeral. An invocation of this method is equivalent to
`Symbol gensymOf "G"`.

### `Symbol gensymOf (prefix).`

Returns a new uninterned symbol, like `gensym`. The name of the
generated symbol shall begin with `~`, followed by the prefix,
followed by an arbitrary numeral. The prefix value must be a string.
