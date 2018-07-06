
# The Slots Object

    Slots := Object clone.

The slots object is a singleton object,
like [the parents object](parents.md), which allows runtime access to
an object's slots, using either predetermined or runtime symbol names.

## Simple Slots

    Slots toString := "Slots".

## Static Methods

### `Slots hold (object, symbol).`

Returns the value of the given slot on `object`. If the result is a
method, it is *not* called. Note that this method may result in a call
to `missing` if the slot is not found through the standard full
retrieval.

### `Slots get (object, symbol).`

Returns the value of the given slot on `object`. If the result is a
method, it is called with `object` as the caller. As such, if `symbol`
is a literal, this is equivalent to simply invoking the method with no
arguments using the usual syntax.

### `Slots put (object, symbol, value).`

Sets the value of the given slot on `object`. If the symbol is a
literal value, this is equivalent to simply using the colon-equals
assignment.

### `Slots delete (object, symbol).`

If a slot with name `symbol` exists on `object` directly, then this
method deletes that slot. Note that this only removes direct slots and
does not interfere with slots from a parent class. This method always
returns the nil object.

### `Slots has? (object, symbol).`

Returns whether or not the slot exists on the object. Specifically,
this method attempts to access the slot using `Slots hold`. If
the access is successful, the return value is true. If the access
attempt throws a `SlotError`, the return value is false. Any other
exceptions or thrown objects are propogated.

### `Slots protect (object, slotName, prot).`

Adds protection to the given slot on the supplied object. `prot`
should be a list of `Protection` instances specifying the
protection(s) to apply. If the slot does not exist, a `SlotError` is
raised.

### `Slots protected? (object, slotName).`

Returns whether the given slot on the supplied object has any
protections applied to it.

### `Slots hasProtection? (object, slotName, prot).`

Returns whether the given slot on the supplied object has the given
protection(s) applied to it, where `prot` is a list of `Protection`
instances.

## The Protection Object

    Protection := Enumeration clone.

Used in the protection interface is the `Protection` enumeration
object, defining the values `Assign` and `Delete`.

[[up](.)]
<br/>[[prev - The Range Object](range.md)]
<br/>[[next - The Stack Frame Object](stackframe.md)]
