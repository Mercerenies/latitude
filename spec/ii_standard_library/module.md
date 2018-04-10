
# The Module Object

    Module := global clone.

A module object, which is also a scope object, encapsulates the notion
of a loaded module. When a module is loaded into Latitude, it is
provided a module object through the `$whereAmI` variable.

## Simple Slots

    Module toString := "Module".

## Methods

### `Module header.`

Returns the module's header, a [`FileHeader`](fileheader.md) object
that details certain information about the module being loaded.

### `Module import (names).`

For each name in the iterable `names`, (shallow-) copies the value at
that slot in `self` to the slot with the same name in the caller's
lexical scope.

[TODO: Should this also work with dynamic scope?]

### `Module importAll.`

Injects a lookup object into the calling lexical scope's inheritance
hierarchy, using a similar technique to that of mixins. If a name is
ever looked up in the calling lexical scope and cannot be found, then
the injected lookup object's `missing` method will look up that same
name in the `self` module's scope. If the name is found there, then
that value is returned. If the name is not found, the injected
object's `missing` will delegate to the parent's `missing`.
