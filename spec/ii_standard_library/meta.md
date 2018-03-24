
# The Meta Object

    meta := Object clone.

The meta object is expected to exist on all traditional objects and
all lexical scope objects. It defines certain special behavior that
the interpreter will refer to. The built-in meta object defines
appropriate default behavior. If a clone of the built-in meta object
is used as the lexical meta object, care must be taken that the
methods still behave according to this specification, as many
preconditions are not checked when the VM invokes meta methods.

Further, all of the slots listed here should either be directly
defined on a given meta object or directly defined on one of its
parents. Implementations are free to ignore `missing` when making meta
calls, for efficiency reasons, so the behavior is undefined if a
particular slot on `meta` is intended to be accessed through
`missing`.

## Simple Slots

    meta toString := "meta".
    meta sigil := Object clone.
    meta lang := Object clone.

## Methods

### `meta meta.`

This method should always return the calling meta object. If `meta` is
cloned, this method should be updated so that it returns the cloned
meta object.

### `meta sys.`

This is a subobject which contains implementation-dependent code. All
lexical meta objects are expected to have `sys`, and its value should
never be modified from the original. The contents of `sys` may change
from release to release, so users should never rely on direct calls to
methods or slots within it.

### `meta missed.`

This method is invoked as a last resort during a full retrieval if the
slot does not exist and `missing` does not exist on the object
either. This most often occurs when dealing with non-traditional
objects, which might not have a `missing` slot. The built-in meta
object has a `missed` method which throws a `SystemError` object with
a generic message.

### `meta radix.`

This object is a clone of the root object which contains several slots
to facilitate writing numerical literals of various bases. It contains
slots `x` and `X`, which parse the argument as hexadecimal; `o` and
`O`, which parse as octal; and `b` and `B`, which parse as binary.

### `meta hashParen.`

[TODO: Document exactly what this does.]

## Inner Object Methods

## `~l (method).`

The `~l` sigil constructs a `Cached` procedure and wraps it in a
method. [TODO: Finish documenting this once `Cached` is documented.]
