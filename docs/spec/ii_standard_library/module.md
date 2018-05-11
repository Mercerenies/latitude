
# The Module Object

    Module := global clone.

A module object, which is also a scope object, encapsulates the notion
of a loaded module. When a module is loaded into Latitude, it is
provided a module object through the `$whereAmI` variable.

## Methods

### `Module header.`

Returns the module's header, a [`FileHeader`](fileheader.md) object
that details certain information about the module being loaded.

### `Module sigils.`

Returns an object whose keys are sigils that the module exports and
whose values are the corresponding implementations of those
sigils. These can be imported into the current scope with
`importAllSigils`.

### `Module toString.`

If called on the `Module` object itself, returns `"Module"`.
Otherwise, returns the module's name. In the latter case, equivalent
to `self header moduleName`.

### `Module import (names).`

For each name in the iterable `names`, (shallow-) copies the value at
that slot in `self` to the slot with the same name in the caller's
lexical scope. Returns `self`.

[TODO: Should this also work with dynamic scope?]

### `Module importAll.`

Injects a lookup object into the calling lexical scope's inheritance
hierarchy, using a similar technique to that of mixins. If a name is
ever looked up in the calling lexical scope and cannot be found, then
the injected lookup object's `missing` method will look up that same
name in the `self` module's scope. If the name is found there, then
that value is returned. If the name is not found, the injected
object's `missing` will delegate to the parent's `missing`. Returns
`self`.

### `Module importAllSigils.`

Imports all of the sigils in `sigils` into the current lexical scope's
meta object. Returns `self`.

[[up](.)]
