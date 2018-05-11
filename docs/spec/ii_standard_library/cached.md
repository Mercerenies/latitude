
# The Cached Value Object

    Cached := Proc clone.

A `Cached` object is a procedure which stores a lazy-evaluated
value. The first time the procedure is invoked, the value is computed,
as per a stored procedure. The result is stored, so that later calls
to the procedure can simply return the value. `Cached` objects are
most commonly created using the [`memo`](global.md#global-memo-block)
global method.

## Simple Slots

    Cached toString := "Cached".

## Methods

### `Cached value.`

Returns the value computed from the stored procedure. If the value has
not been computed yet, this method returns `Nil`.

### `Cached done?.`

Returns `True` if and only if the stored procedure has been invoked
and the value set.

### `Cached procedure.`

This is the stored procedure itself. Its default value is the global
`Proc` object, but it is frequently overriden in subobjects to a
correct implementation of the stored procedure work. This should *not*
be an evaluating object and should also *not* return an evaluating
object as its result. It should also work correctly when passed no
arguments.

### `Cached call.`

If the stored value has already been computed (`done?`), returns the
stored value (`value`). Otherwise, this method simply returns the
stored value.



[[up](.)]
<br/>[[prev - Booleans and the Nil Object](boolnil.md)]
<br/>[[next - The Chain Object](chain.md)]
